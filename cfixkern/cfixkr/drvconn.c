/*----------------------------------------------------------------------
 *	Purpose:
 *		Represents a connection between cfixkr and a test driver.
 *
 *		N.B. INTERFACE::Context refers to a PCFIXKRP_DRIVER_CONNECTION.
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

//
// Not defined in wdm.h, but holds for i386 and amd64.
//
#define SYNCH_LEVEL (IPI_LEVEL-2)

#define CFIXKRP_DRIVER_CONNECTION_SIGNATURE 'nnoC'

typedef struct _CFIXKRP_THREAD_CHANNEL
{
	union
	{
		PRKTHREAD Thread;
		JPHT_HASHTABLE_ENTRY HashtableEntry;
	} Key;

	PCFIXKRP_REPORT_CHANNEL Channel;
} CFIXKRP_THREAD_CHANNEL, *PCFIXKRP_THREAD_CHANNEL;

C_ASSERT( FIELD_OFFSET( CFIXKRP_THREAD_CHANNEL, Key.Thread ) ==
		  FIELD_OFFSET( CFIXKRP_THREAD_CHANNEL, Key.HashtableEntry.Key ) );

/*++
	Structure Description:
		Driver Connection Registry Object.

		Allocated from nonpaged pool.
--*/
typedef struct _CFIXKRP_DRIVER_CONNECTION
{
	ULONG Signature;

	ULONGLONG DriverLoadAddress;

	//
	// External references are the references by the test driver. At
	// most one reference may be held. As soon as the driver releases
	// its reference, we must assume that the driver is unloading
	// and we may not touch it any more (i.e. call exports).
	//
	// N.B. The test driver must hold a reference on our device object
	// to avoid this driver from being unloaded *before* the test
	// driver unloads.
	//
	// Requires exclusive ownership of DriverUnloadLock (Holding this lock
	// when decrementing ExternalReferenceCount is the key for making 
	// unloading safe).
	//
	volatile LONG ExternalReferenceCount;

	//
	// References within this driver. Includes external references.
	//
	volatile LONG ReferenceCount;

	//
	// Lock protecting against premature driver unload.
	//
	ERESOURCE DriverUnloadLock;

	//
	// Adapter to the test driver.
	//
	PCFIXKRP_TEST_ADAPTER Adapter;

	//
	// Hashtable containing the mapping between threads and channels,
	// i.e. which channel is to be used for any events occuring
	// on the current thread.
	//
	struct
	{
		KSPIN_LOCK Lock;

		//
		// Lock. Use CfixkrsAcquireChannelLock to acquire ownership.
		// IRQL is raised to SYNCH_LEVEL i.o. to allow reports calls
		// at DIRQL.
		//
		JPHT_HASHTABLE Table;
	} Channels;
} CFIXKRP_DRIVER_CONNECTION, *PCFIXKRP_DRIVER_CONNECTION;

/*----------------------------------------------------------------------
 *
 * Hashtable Callbacks.
 *
 */
static ULONG CfixkrsHashThreadChannel(
	__in ULONG_PTR Key
	)
{
	//
	// N.B. Key is the PRKTHREAD.
	//
	return ( ULONG ) Key;
}


static BOOLEAN CfixkrsEqualsThreadChannel(
	__in ULONG_PTR KeyLhs,
	__in ULONG_PTR KeyRhs
	)
{
	//
	// N.B. Keys are PRKTHREADs, which are the primkeys.
	//
	return ( BOOLEAN ) ( KeyLhs == KeyRhs );
}

/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */
static BOOLEAN CfixkrsAcquireDriverUnloadProtection(
	__in PCFIXKRP_DRIVER_CONNECTION Conn,
	__in BOOLEAN Wait
	)
{
	if ( Conn->ExternalReferenceCount == 0 )
	{
		//
		// Driver has already released its (external) reference,
		// so it is not safe to rouch the driver any more.
		//
		return FALSE;
	}

	#pragma warning( suppress: 28103 )
	KeEnterCriticalRegion();
	
	return ExAcquireResourceSharedLite( &Conn->DriverUnloadLock, Wait );
}

static VOID CfixkrsReleaseDriverUnloadProtection(
	__in PCFIXKRP_DRIVER_CONNECTION Conn
	)
{
	#pragma warning( suppress: 28107 )
	ExReleaseResourceLite( &Conn->DriverUnloadLock );
	
	#pragma warning( suppress: 28107 )
	KeLeaveCriticalRegion();
}

static VOID CfixkrsAcquireChannelLock( 
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__out PKLOCK_QUEUE_HANDLE LockHandle	
	)
{
	KeRaiseIrql( SYNCH_LEVEL, &LockHandle->OldIrql );
	KeAcquireInStackQueuedSpinLockAtDpcLevel( 
		&Connection->Channels.Lock, LockHandle );
}

static VOID CfixkrsReleaseChannelLock( 
	__in PKLOCK_QUEUE_HANDLE LockHandle
	)
{
	KeReleaseInStackQueuedSpinLockFromDpcLevel( LockHandle );
	KeLowerIrql( LockHandle->OldIrql );
}

/*----------------------------------------------------------------------
 *
 * Lifecycle management.
 *
 */

VOID CfixkrpReferenceConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Conn
	)
{
	ASSERT( Conn->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

	InterlockedIncrement( &Conn->ReferenceCount );
}

VOID CfixkrpDereferenceConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Conn
	)
{
	ASSERT( Conn->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

	if ( 0 == InterlockedDecrement( &Conn->ReferenceCount ) )
	{
		//
		// The channel hashtable should be empty - after all, when it was
		// not, it would mean that a test routine is still active.
		// If a routine was still active, deleting this object
		// would be an invalid operation.
		//
		ASSERT( JphtGetEntryCountHashtable( &Conn->Channels.Table ) == 0 );
		JphtDeleteHashtable( &Conn->Channels.Table );

		//
		// Unregister. As the registry holds a weak reference, it still
		// references this object although the refcount has just
		// dropped to 0.
		//
		CfixkrpUnregisterDriverConnection( Conn->DriverLoadAddress );
		CfixkrpDeleteTestAdapter( Conn->Adapter );

		ExDeleteResourceLite( &Conn->DriverUnloadLock ); 

		ExFreePoolWithTag( Conn, CFIXKR_POOL_TAG );
	}
}

static VOID CfixkrsExternalReferenceConnection(
	__in PVOID ConnPointer
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) ConnPointer;
	ASSERT( Conn->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( Conn->ExternalReferenceCount == 0 );

	CfixkrpReferenceConnection( Conn );
	InterlockedIncrement( &Conn->ExternalReferenceCount );
}

static VOID CfixkrsExternalDereferenceConnection(
	__in PVOID ConnPointer
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) ConnPointer;
	ASSERT( Conn->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( Conn->ExternalReferenceCount == 1 );

	//
	// Acquire lock to wait for other threads currently accessing
	// this driver (i.e. call exports) to finish.
	//
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite( &Conn->DriverUnloadLock, TRUE );
	
	InterlockedDecrement( &Conn->ExternalReferenceCount );
	
	ExReleaseResourceLite( &Conn->DriverUnloadLock );
	KeLeaveCriticalRegion();

	//
	// As ExternalReferenceCount is now 0, the driver will not be 
	// touched any more.
	//
	// Normal dereference - this might free the object.
	//
	CfixkrpDereferenceConnection( Conn );
}

/*----------------------------------------------------------------------
 *
 * Current Report Channel handling.
 *
 */
static NTSTATUS CfixkrsSetReportChannelCurrentThread(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__in PCFIXKRP_REPORT_CHANNEL Channel
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;
	KLOCK_QUEUE_HANDLE LockHandle;
	PCFIXKRP_THREAD_CHANNEL ThreadChannel;

	ASSERT( Connection );
	ASSERT( Channel );
	ASSERT( Channel->Signature == CFIXKRP_REPORT_CHANNEL_SIGNATURE );

	//
	// Add a new entry to the hashtable.
	//
	ThreadChannel = ExAllocatePoolWithTag( 
		NonPagedPool, 
		sizeof( CFIXKRP_THREAD_CHANNEL ), 
		CFIXKR_POOL_TAG );
	if ( ThreadChannel == NULL )
	{
		return STATUS_NO_MEMORY;
	}

	ThreadChannel->Key.Thread	= KeGetCurrentThread();
	ThreadChannel->Channel		= Channel;

	CfixkrsAcquireChannelLock( Connection, &LockHandle );
	JphtPutEntryHashtable(
		&Connection->Channels.Table,
		&ThreadChannel->Key.HashtableEntry,
		&OldEntry );
	CfixkrsReleaseChannelLock( &LockHandle );

	//
	// If OldEntry != NULL, we must have enetered a recusion, which
	// should not occur.
	//
	ASSERT( OldEntry == NULL );

	return TRUE;
}

static VOID CfixkrsResetReportChannelCurrentThread(
	__in PCFIXKRP_DRIVER_CONNECTION Connection
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	KLOCK_QUEUE_HANDLE LockHandle;
	PCFIXKRP_THREAD_CHANNEL ThreadChannel;

	ASSERT( Connection );

	CfixkrsAcquireChannelLock( Connection, &LockHandle );
	JphtRemoveEntryHashtable( 
		&Connection->Channels.Table,
		( ULONG_PTR ) KeGetCurrentThread(),
		&Entry );
	CfixkrsReleaseChannelLock( &LockHandle );

	ASSERT( Entry != NULL );

	ThreadChannel = CONTAINING_RECORD(
		Entry,
		CFIXKRP_THREAD_CHANNEL,
		Key.HashtableEntry );

	ExFreePoolWithTag( ThreadChannel, CFIXKR_POOL_TAG );
}

PCFIXKRP_REPORT_CHANNEL CfixkrpGetReportChannelCurrentThread(
	__in PCFIXKRP_DRIVER_CONNECTION Connection
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	KLOCK_QUEUE_HANDLE LockHandle;
	PCFIXKRP_THREAD_CHANNEL ThreadChannel;

	ASSERT( Connection );

	CfixkrsAcquireChannelLock( Connection, &LockHandle );
	Entry = JphtGetEntryHashtable( 
		&Connection->Channels.Table,
		( ULONG_PTR ) KeGetCurrentThread() );
	CfixkrsReleaseChannelLock( &LockHandle );

	if ( Entry != NULL )
	{
		ThreadChannel = CONTAINING_RECORD(
			Entry,
			CFIXKRP_THREAD_CHANNEL,
			Key.HashtableEntry );

		ASSERT( ThreadChannel->Channel->Signature == CFIXKRP_REPORT_CHANNEL_SIGNATURE );

		return ThreadChannel->Channel;
	}
	else
	{
		return NULL;
	}
}

/*----------------------------------------------------------------------
 *
 * Exception Filters.
 *
 */
static EXCEPTION_DISPOSITION CfixkrsExceptionFilter(
	__in PEXCEPTION_POINTERS ExcpPointers,
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__out BOOLEAN *AbortRun
	)
{
	ULONG ExcpCode = ExcpPointers->ExceptionRecord->ExceptionCode;

	if ( EXCEPTION_TESTCASE_INCONCLUSIVE == ExcpCode ||
		 EXCEPTION_TESTCASE_FAILED == ExcpCode )
	{
		//
		// Testcase failed/turned out to be inconclusive.
		//
		*AbortRun = FALSE;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else if ( EXCEPTION_TESTCASE_FAILED_ABORT == ExcpCode )
	{
		//
		// Testcase failed and is to be aborted.
		//
		*AbortRun = TRUE;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
	{
		CFIX_REPORT_DISPOSITION Disp;

		Disp = CfixkrpReportUnhandledException( 
			Channel, 
			ExcpPointers );
		
		*AbortRun = ( BOOLEAN ) ( Disp == CfixAbort );
		if ( Disp == CfixBreak )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
		else
		{
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}
}

/*----------------------------------------------------------------------
 *
 * Internals.
 *
 */
NTSTATUS CfixkrpCreateAndRegisterDriverConnection(
	__in ULONGLONG DriverLoadAddress,
	__out PCFIXKRP_DRIVER_CONNECTION *Connection
	)
{
	BOOLEAN HashtableInitialized = FALSE;
	BOOLEAN ResourceInitialized = FALSE;
	NTSTATUS Status;
	PCFIXKRP_DRIVER_CONNECTION TempConn;

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
	ASSERT( DriverLoadAddress != 0 );
	ASSERT( Connection );

	//
	// Must use NonPahedPool as the report routines are to be usable
	// at any IRQL.
	//
	TempConn = ( PCFIXKRP_DRIVER_CONNECTION ) ExAllocatePoolWithTag(
		NonPagedPool,
		sizeof( CFIXKRP_DRIVER_CONNECTION ),
		CFIXKR_POOL_TAG );
	if ( ! TempConn )
	{
		return STATUS_NO_MEMORY;
	}

	RtlZeroMemory( TempConn, sizeof( CFIXKRP_DRIVER_CONNECTION ) );

	TempConn->Signature						 = CFIXKRP_DRIVER_CONNECTION_SIGNATURE;
	TempConn->DriverLoadAddress				 = DriverLoadAddress;
	TempConn->Adapter						 = NULL;

	TempConn->ReferenceCount				 = 0;
	TempConn->ExternalReferenceCount		 = 0;
	CfixkrsExternalReferenceConnection( TempConn );

	ASSERT( TempConn->ReferenceCount == 1 );
	ASSERT( TempConn->ExternalReferenceCount == 1 );

	//
	// Initialize hashtable.
	//
	if ( ! JphtInitializeHashtable(
		&TempConn->Channels.Table,
		CfixkrpAllocateNonpagedHashtableMemory,
		CfixkrpFreeHashtableMemory,
		CfixkrsHashThreadChannel,
		CfixkrsEqualsThreadChannel,
		31 ) )
	{
		Status = STATUS_NO_MEMORY;
		goto Cleanup;
	}
	HashtableInitialized = TRUE;

	KeInitializeSpinLock( &TempConn->Channels.Lock );
	
	Status = ExInitializeResourceLite( &TempConn->DriverUnloadLock );
	if ( ! NT_SUCCESS( Status ) )
	{
		goto Cleanup;
	}
	ResourceInitialized = TRUE;

	Status = CfixkrpCreateTestAdapter(
		DriverLoadAddress,
		&TempConn->Adapter );
	if ( ! NT_SUCCESS( Status ) )
	{
		goto Cleanup;
	}

	//
	// Register the connection to make it available for sessions.
	//
	Status = CfixkrpRegisterDriverConnection( DriverLoadAddress, TempConn );
	if ( ! NT_SUCCESS( Status ) )
	{
		goto Cleanup;
	}

	*Connection = TempConn;

Cleanup:
	if ( ! NT_SUCCESS( Status ) )
	{
		if ( TempConn->Adapter )
		{
			CfixkrpDeleteTestAdapter( TempConn->Adapter );
		}

		if ( ResourceInitialized )
		{
			ExDeleteResourceLite( &TempConn->DriverUnloadLock ); 
		}

		if ( HashtableInitialized )
		{
			JphtDeleteHashtable( &TempConn->Channels.Table );
		}
		
		ExFreePoolWithTag( TempConn, CFIXKR_POOL_TAG );
	}

	return Status;
}

VOID CfixkrpQuerySinkInterfaceDriverConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__out PCFIXKR_REPORT_SINK_INTERFACE Interface
	)
{
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( Connection );
	ASSERT( Connection->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );
	ASSERT( Interface );

	Interface->Base.Size						= sizeof( CFIXKR_REPORT_SINK_INTERFACE );
	Interface->Base.Version 					= CFIXKR_REPORT_SINK_VERSION;
	Interface->Base.Context 					= Connection;
	Interface->Base.InterfaceReference			= CfixkrsExternalReferenceConnection;
	Interface->Base.InterfaceDereference		= CfixkrsExternalDereferenceConnection;

	CfixkrpGetReportSinkStubs( &Interface->Methods );
}

NTSTATUS CfixkrpQueryModuleDriverConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__in ULONG MaximumBufferSize,
	__out PUCHAR IoBuffer,
	__out PULONG BufferSize
	)
{
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
	ASSERT( Connection );
	ASSERT( Connection->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );
	ASSERT( IoBuffer );
	ASSERT( BufferSize );

	return CfixkrpQueryModuleTestAdapter( 
		Connection->Adapter,
		MaximumBufferSize,
		IoBuffer,
		BufferSize );
}

NTSTATUS CfixkrpCallRoutineDriverConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__in USHORT FixtureKey,
	__in USHORT RoutineKey,
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__out BOOLEAN *RoutineRanToCompletion,
	__out BOOLEAN *AbortRun
	)
{
	KIRQL Irql;
	CFIX_PE_TESTCASE_ROUTINE Routine;
	NTSTATUS Status;

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
	ASSERT( Connection );
	ASSERT( Connection->Signature == CFIXKRP_DRIVER_CONNECTION_SIGNATURE );
	ASSERT( Channel );
	ASSERT( RoutineRanToCompletion );
	ASSERT( AbortRun );

	//
	// Get routine.
	//
	Status = CfixkrpGetRoutineTestAdapter(
		Connection->Adapter,
		FixtureKey,
		RoutineKey,
		&Routine );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	ASSERT( Routine != NULL );
	
	//
	// Associate the current thread with the given report channel.
	//
	Status = CfixkrsSetReportChannelCurrentThread(
		Connection,
		Channel );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	//
	// During the call, someone may try to unload the driver, thus,
	// acquire unload protection forstalling any unload attempts.
	//
	if ( ! CfixkrsAcquireDriverUnloadProtection( Connection, TRUE ) )
	{
		//
		// Driver has already been unloaded.
		//
		return STATUS_DRIVER_UNABLE_TO_LOAD;
	}

	Irql = KeGetCurrentIrql();

	//
	// The routine may throw exceptions, prepare for that.
	//
	__try
	{
		__try
		{
			( Routine )();
			*RoutineRanToCompletion = TRUE;
			*AbortRun = FALSE;
		}
		__except ( CfixkrsExceptionFilter(
			GetExceptionInformation(),
			Channel,
			AbortRun ) )
		{
			*RoutineRanToCompletion = FALSE;
		}
	}
	__finally
	{
		//
		// In case of a testcase abortion, it may happen that the
		// tes routine has raised the IRQL, yet did not proceed far
		// ehough to lower the IRQL again. Thus, we enforce IRQL
		// restoration here.
		//
		if ( Irql != KeGetCurrentIrql() )
		{
			DbgPrint( ( "CFIXKR: IRQL was not properly restored\n" ) );

			//
			// This is bad. Force IRQL back to original value.
			//
			ASSERT( KeGetCurrentIrql() > Irql );
			KeLowerIrql( Irql );
		}
		
		CfixkrsReleaseDriverUnloadProtection( Connection );

		//
		// Disassociate channel from current thread.
		//
		CfixkrsResetReportChannelCurrentThread( Connection );
	}

	return STATUS_SUCCESS;
}