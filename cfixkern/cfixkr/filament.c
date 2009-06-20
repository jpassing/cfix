/*----------------------------------------------------------------------
 *	Purpose:
 *		Association between threads and filaments
 *
 * Copyright:
 *		2008-2009, Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wdm.h>
#include <hashtable.h>
#include "cfixkrp.h"

//
// Not defined in wdm.h, but holds for i386 and amd64.
//
#define SYNCH_LEVEL (IPI_LEVEL-2)

typedef struct _CFIXKRP_FILAMENT_ENTRY
{
	union
	{
		PRKTHREAD Thread;
		JPHT_HASHTABLE_ENTRY HashtableEntry;
	} Key;

	PCFIXKRP_FILAMENT Filament;
} CFIXKRP_FILAMENT_ENTRY, *PCFIXKRP_FILAMENT_ENTRY;

C_ASSERT( FIELD_OFFSET( CFIXKRP_FILAMENT_ENTRY, Key.Thread ) ==
		  FIELD_OFFSET( CFIXKRP_FILAMENT_ENTRY, Key.HashtableEntry.Key ) );

/*----------------------------------------------------------------------
 *
 * Hashtable Callbacks.
 *
 */
static ULONG CfixkrsHashFilamentEntry(
	__in ULONG_PTR Key
	)
{
	//
	// N.B. Key is the PRKTHREAD.
	//
	return ( ULONG ) Key;
}


static BOOLEAN CfixkrsEqualsFilamentEntry(
	__in ULONG_PTR KeyLhs,
	__in ULONG_PTR KeyRhs
	)
{
	//
	// N.B. Keys are PRKTHREADs, which are the primkeys.
	//
	return ( BOOLEAN ) ( KeyLhs == KeyRhs );
}


static VOID CfixkrsAcquireLockFilamentRegistry( 
	__in PCFIXKRP_FILAMENT_REGISTRY Registry,
	__out PKIRQL OldIrql	
	)
{
	KeRaiseIrql( SYNCH_LEVEL, OldIrql );
	KeAcquireSpinLockAtDpcLevel( 
		&Registry->Lock );
}

static VOID CfixkrsReleaseLockFilamentRegistry( 
	__in PCFIXKRP_FILAMENT_REGISTRY Registry,
	__in KIRQL OldIrql
	)
{
	KeReleaseSpinLockFromDpcLevel( &Registry->Lock );
	KeLowerIrql( OldIrql );
}


/*----------------------------------------------------------------------
 *
 * Internal.
 *
 */

NTSTATUS CfixkrpInitializeFilamentRegistry(
	__out PCFIXKRP_FILAMENT_REGISTRY Registry
	)
{
	//
	// Initialize hashtable.
	//
	if ( ! JphtInitializeHashtable(
		&Registry->Table,
		CfixkrpAllocateNonpagedHashtableMemory,
		CfixkrpFreeHashtableMemory,
		CfixkrsHashFilamentEntry,
		CfixkrsEqualsFilamentEntry,
		31 ) )
	{
		return STATUS_NO_MEMORY;
	}

	KeInitializeSpinLock( &Registry->Lock );

	return STATUS_SUCCESS;
}

VOID CfixkrpDeleteFilamentRegistry(
	__in PCFIXKRP_FILAMENT_REGISTRY Registry
	)
{
	//
	// The hashtable should be empty - after all, when it was
	// not, it would mean that a test routine is still active.
	// If a routine was still active, deleting this object
	// would be an invalid operation.
	//
	ASSERT( JphtGetEntryCountHashtable( &Registry->Table ) == 0 );
	JphtDeleteHashtable( &Registry->Table );
}

VOID CfixkrpInitializeFilament(
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__in ULONG MainThreadId,
	__out PCFIXKRP_FILAMENT Filament
	)
{
	RtlZeroMemory( Filament, sizeof( CFIXKRP_FILAMENT ) );

	Filament->Channel		= Channel;
	Filament->MainThreadId	= MainThreadId;
}

VOID CfixkrpDeleteFilament(
	__in PCFIXKRP_FILAMENT Filament
	)
{
	UNREFERENCED_PARAMETER( Filament );
}

NTSTATUS CfixkrpSetCurrentFilament(
	__in PCFIXKRP_FILAMENT_REGISTRY Registry,
	__in PCFIXKRP_FILAMENT Filament
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;
	KIRQL OldIrql;
	PCFIXKRP_FILAMENT_ENTRY Entry;

	ASSERT( Registry );
	ASSERT( Filament );
	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	//
	// Add a new entry to the hashtable.
	//
	Entry = ExAllocatePoolWithTag( 
		NonPagedPool, 
		sizeof( CFIXKRP_FILAMENT_ENTRY ), 
		CFIXKR_POOL_TAG );
	if ( Entry == NULL )
	{
		return STATUS_NO_MEMORY;
	}

	Entry->Key.Thread	= KeGetCurrentThread();
	Entry->Filament		= Filament;

	CfixkrsAcquireLockFilamentRegistry( Registry, &OldIrql );
	JphtPutEntryHashtable(
		&Registry->Table,
		&Entry->Key.HashtableEntry,
		&OldEntry );
	CfixkrsReleaseLockFilamentRegistry( Registry, OldIrql );

	//
	// If OldEntry != NULL, we must have enetered a recusion, which
	// should not occur.
	//
	ASSERT( OldEntry == NULL );

	return TRUE;
}

VOID CfixkrpResetCurrentFilament(
	__in PCFIXKRP_FILAMENT_REGISTRY Registry
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	KIRQL OldIrql;
	PCFIXKRP_FILAMENT_ENTRY FilamentEntry;

	ASSERT( Registry );
	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	CfixkrsAcquireLockFilamentRegistry( Registry, &OldIrql );
	JphtRemoveEntryHashtable( 
		&Registry->Table,
		( ULONG_PTR ) KeGetCurrentThread(),
		&Entry );
	CfixkrsReleaseLockFilamentRegistry( Registry, OldIrql );

	ASSERT( Entry != NULL );

	FilamentEntry = CONTAINING_RECORD(
		Entry,
		CFIXKRP_FILAMENT_ENTRY,
		Key.HashtableEntry );

	ExFreePoolWithTag( FilamentEntry, CFIXKR_POOL_TAG );
}

PCFIXKRP_FILAMENT CfixkrpGetCurrentFilament(
	__in PCFIXKRP_FILAMENT_REGISTRY Registry
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	KIRQL OldIrql;
	PCFIXKRP_FILAMENT_ENTRY FilamentEntry;

	ASSERT( Registry );

	CfixkrsAcquireLockFilamentRegistry( Registry, &OldIrql );
	Entry = JphtGetEntryHashtable( 
		&Registry->Table,
		( ULONG_PTR ) KeGetCurrentThread() );
	CfixkrsReleaseLockFilamentRegistry( Registry, OldIrql );

	if ( Entry != NULL )
	{
		FilamentEntry = CONTAINING_RECORD(
			Entry,
			CFIXKRP_FILAMENT_ENTRY,
			Key.HashtableEntry );

		ASSERT( FilamentEntry->Filament->Channel->Signature == CFIXKRP_REPORT_CHANNEL_SIGNATURE );

		return FilamentEntry->Filament;
	}
	else
	{
		return NULL;
	}
}
