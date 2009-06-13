/*----------------------------------------------------------------------
 *	Purpose:
 *		Driver Connection Registry.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <wdm.h>
#include <hashtable.h>
#include "cfixkrp.h"

typedef struct _CFIXKRP_DRVCONN_REGISTRATION
{
	union
	{
		//
		// Cannot be ULONGLONG, but ULONG_PTR is sufficient.
		//
		ULONG_PTR LoadAddress;
		JPHT_HASHTABLE_ENTRY HashtableEntry;
	} Key;

	//
	// Weak reference!
	//
	PCFIXKRP_DRIVER_CONNECTION Connection;
} CFIXKRP_DRVCONN_REGISTRATION, *PCFIXKRP_DRVCONN_REGISTRATION;

C_ASSERT( FIELD_OFFSET( CFIXKRP_DRVCONN_REGISTRATION, Key.LoadAddress ) ==
		  FIELD_OFFSET( CFIXKRP_DRVCONN_REGISTRATION, Key.HashtableEntry.Key ) );
		  
/*++
	Structure Description:
		Connection Registry Object.

		Allocated from paged pool.
--*/
typedef struct _CFIXKRP_DRVCONN_REGISTRY
{
	//
	// Hashtable of connections.
	//	Key:	ULONG_PTR Load Address
	//	Value:	PCFIXKRP_DRVCONN_REGISTRATION
	//
	JPHT_HASHTABLE Connections;

	//
	// Lock guarding the hashtable.
	//
	ERESOURCE Lock;

	//
	// If TRUE, the hashtable has already been torn down.
	//
	// Background: It is possible that the driver's unload routine (and thus
	// CfixkrpTeardownDriverConnectionRegistry) has been called *before*
	// the last driver unregisters its connection.
	//
	volatile BOOLEAN TornDown;
} CFIXKRP_DRVCONN_REGISTRY, *PCFIXKRP_DRVCONN_REGISTRY;

static CFIXKRP_DRVCONN_REGISTRY CfixkrsDriverConnectionRegistry;

typedef struct _CFIXKRP_GET_CONNECTIONS_CONTEXT
{
	ULONG Capacity;
	ULONG Count;
	ULONG TotalAvailable;
	ULONGLONG *DriverLoadAddresses;
	BOOLEAN Overflow;
} CFIXKRP_GET_CONNECTIONS_CONTEXT, *PCFIXKRP_GET_CONNECTIONS_CONTEXT;

/*----------------------------------------------------------------------
 *
 * Hashtable Callbacks.
 *
 */
static ULONG CfixkrsHashRegistration(
	__in ULONG_PTR Key
	)
{
	//
	// N.B. Key is the LoadAddress.
	//
	return ( ULONG ) Key;
}


static BOOLEAN CfixkrsEqualsRegistration(
	__in ULONG_PTR KeyLhs,
	__in ULONG_PTR KeyRhs
	)
{
	//
	// N.B. Keys are the LoadAddresses, which are the primkeys.
	//
	return ( BOOLEAN ) ( KeyLhs == KeyRhs );
}

static VOID CfixkrsDeleteRegistrationHashtableCallback(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID Unused
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;
	PCFIXKRP_DRVCONN_REGISTRATION Registration;
	
	UNREFERENCED_PARAMETER( Unused );

	JphtRemoveEntryHashtable(
		Hashtable,
		Entry->Key,
		&OldEntry );

	ASSERT( Entry == OldEntry );

	Registration = CONTAINING_RECORD(
		Entry,
		CFIXKRP_DRVCONN_REGISTRATION,
		Key.HashtableEntry );

	CfixkrpFreeHashtableMemory( Registration );
}

static VOID CfixkrsEnlistConnectionHashtableCallback(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID ContextPtr
	)
{
	PCFIXKRP_GET_CONNECTIONS_CONTEXT Context;
	PCFIXKRP_DRVCONN_REGISTRATION Registration;
	
	UNREFERENCED_PARAMETER( Hashtable );

	Context = ( PCFIXKRP_GET_CONNECTIONS_CONTEXT ) ContextPtr;
	if ( ! Context ) return;

	Registration = CONTAINING_RECORD(
		Entry,
		CFIXKRP_DRVCONN_REGISTRATION,
		Key.HashtableEntry );

	Context->TotalAvailable++;
	if ( Context->Count < Context->Capacity )
	{
		Context->DriverLoadAddresses[ ( Context->Count )++ ] = 
			Registration->Key.LoadAddress;
	}
	else
	{
		Context->Overflow = TRUE;
	}
}

/*----------------------------------------------------------------------
 *
 * Internals.
 *
 */
NTSTATUS CfixkrpInitializeDriverConnectionRegistry()
{
	NTSTATUS Status;

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

	Status = ExInitializeResourceLite(
		&CfixkrsDriverConnectionRegistry.Lock );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	//
	// Initialize hashtable.
	//
	if ( ! JphtInitializeHashtable(
		&CfixkrsDriverConnectionRegistry.Connections,
		CfixkrpAllocatePagedHashtableMemory,
		CfixkrpFreeHashtableMemory,
		CfixkrsHashRegistration,
		CfixkrsEqualsRegistration,
		31 ) )
	{
		( VOID ) ExDeleteResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
		return STATUS_NO_MEMORY;
	}

	CfixkrsDriverConnectionRegistry.TornDown = FALSE;

	return STATUS_SUCCESS;
}

VOID CfixkrpTeardownDriverConnectionRegistry()
{
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( ! CfixkrsDriverConnectionRegistry.TornDown );

	//
	// Mark as being torn down. Do that under a lock in order to
	// wait for other threads to get their hands off the structure.
	//
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(
		&CfixkrsDriverConnectionRegistry.Lock, TRUE );

	CfixkrsDriverConnectionRegistry.TornDown = TRUE;
	
	ExReleaseResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
	KeLeaveCriticalRegion();

	//
	// Unregister all and delete hashtable.
	//
	JphtEnumerateEntries(
		&CfixkrsDriverConnectionRegistry.Connections,
		CfixkrsDeleteRegistrationHashtableCallback,
		NULL );

	JphtDeleteHashtable( &CfixkrsDriverConnectionRegistry.Connections );
	( VOID ) ExDeleteResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
}

NTSTATUS CfixkrpRegisterDriverConnection(
	__in ULONGLONG LoadAddress,
	__in PCFIXKRP_DRIVER_CONNECTION Connection
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;
	PCFIXKRP_DRVCONN_REGISTRATION Registration;

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( LoadAddress != 0 );
	ASSERT( Connection );
	ASSERT( ! CfixkrsDriverConnectionRegistry.TornDown );

	Registration = ExAllocatePoolWithTag( 
		PagedPool, 
		sizeof( CFIXKRP_DRVCONN_REGISTRATION ), 
		CFIXKR_POOL_TAG );
	if ( ! Registration )
	{
		return STATUS_NO_MEMORY;
	}

	//
	// N.B. No InterfaceReference call on Connection here.
	//
	// N.B. ULONG_PTR cast is safe.
	//
	Registration->Key.LoadAddress	= ( ULONG_PTR ) LoadAddress;
	Registration->Connection		= Connection;

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(
		&CfixkrsDriverConnectionRegistry.Lock, TRUE );

	JphtPutEntryHashtable(
		&CfixkrsDriverConnectionRegistry.Connections,
		&Registration->Key.HashtableEntry,
		&OldEntry );

	ASSERT( OldEntry == NULL );

	ExReleaseResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
	KeLeaveCriticalRegion();

	return STATUS_SUCCESS;
}

NTSTATUS CfixkrpUnregisterDriverConnection(
	__in ULONGLONG LoadAddress
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	PCFIXKRP_DRVCONN_REGISTRATION Registration;

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( LoadAddress != 0 );

	if ( CfixkrsDriverConnectionRegistry.TornDown )
	{
		//
		// This is possible and is ok.
		//
		return STATUS_SUCCESS;
	}

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(
		&CfixkrsDriverConnectionRegistry.Lock, TRUE );

	JphtRemoveEntryHashtable( 
		&CfixkrsDriverConnectionRegistry.Connections,
		( ULONG_PTR ) LoadAddress,
		&Entry );
	if ( Entry != NULL )
	{
		Registration = CONTAINING_RECORD(
			Entry,
			CFIXKRP_DRVCONN_REGISTRATION,
			Key.HashtableEntry );

		//
		// N.B. Connection pointer is weak, so no need to dereference here.
		//
		CfixkrpFreeHashtableMemory( Registration );
	}

	ExReleaseResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
	KeLeaveCriticalRegion();

	return STATUS_SUCCESS;
}

NTSTATUS CfixkrpLookupDriverConnection(
	__in ULONGLONG DriverLoadAddress,
	__out PCFIXKRP_DRIVER_CONNECTION *Connection
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	PCFIXKRP_DRVCONN_REGISTRATION Registration;
	NTSTATUS Status;

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( DriverLoadAddress != 0 );
	ASSERT( Connection );
	ASSERT( ! CfixkrsDriverConnectionRegistry.TornDown );

	*Connection = NULL;

	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(
		&CfixkrsDriverConnectionRegistry.Lock, TRUE );
	Entry = JphtGetEntryHashtable( 
		&CfixkrsDriverConnectionRegistry.Connections,
		( ULONG_PTR ) DriverLoadAddress );
	if ( Entry != NULL )
	{
		Registration = CONTAINING_RECORD(
			Entry,
			CFIXKRP_DRVCONN_REGISTRATION,
			Key.HashtableEntry );

		*Connection = Registration->Connection;

		CfixkrpReferenceConnection( *Connection );

		Status = STATUS_SUCCESS;
	}
	else
	{
		Status = STATUS_NOT_FOUND;
	}

	ExReleaseResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
	KeLeaveCriticalRegion();

	return Status;
}

NTSTATUS CfixkrpGetDriverConnections(
	__in ULONG LoadAddressesCapacity,
	__out_ecount(LoadAddressesCapacity) ULONGLONG *DriverLoadAddresses,
	__out PULONG ElementsWritten,
	__out PULONG ElementsAvailable 
	)
{
	CFIXKRP_GET_CONNECTIONS_CONTEXT Ctx;

	ASSERT( DriverLoadAddresses );
	ASSERT( ElementsWritten );
	ASSERT( ElementsAvailable );
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

	Ctx.Capacity			= LoadAddressesCapacity;
	Ctx.Count				= 0;
	Ctx.TotalAvailable		= 0;
	Ctx.DriverLoadAddresses = DriverLoadAddresses;
	Ctx.Overflow			= FALSE;

	//
	// Enumerate registrations and collect load addresses.
	//
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(
		&CfixkrsDriverConnectionRegistry.Lock, TRUE );
	
	JphtEnumerateEntries(
		&CfixkrsDriverConnectionRegistry.Connections,
		CfixkrsEnlistConnectionHashtableCallback,
		&Ctx );

	ExReleaseResourceLite( &CfixkrsDriverConnectionRegistry.Lock );
	KeLeaveCriticalRegion();

	ASSERT( Ctx.DriverLoadAddresses == DriverLoadAddresses );
	ASSERT( Ctx.Count <= Ctx.Capacity );
	ASSERT( ! Ctx.Overflow || ( Ctx.Count == Ctx.Capacity ) );

	*ElementsAvailable = Ctx.TotalAvailable;
	*ElementsWritten = Ctx.Count;

	if ( Ctx.Overflow )
	{
		ASSERT( *ElementsAvailable > *ElementsWritten );

		//
		// N.B. This is a warning -- output parameters are filled.
		//
		return STATUS_BUFFER_OVERFLOW;
	}
	else
	{
		ASSERT( *ElementsAvailable == *ElementsWritten );

		return STATUS_SUCCESS;
	}
}