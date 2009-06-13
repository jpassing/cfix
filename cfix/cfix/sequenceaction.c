/*----------------------------------------------------------------------
 * Purpose:
 *		Execution os a sequence of actions.
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

#define CFIXAPI

#include "cfixp.h"
#include "list.h"
#include <stdlib.h>

typedef struct _SEQUENCE_ENTRY
{
	LIST_ENTRY ListEntry;

	//
	// Referenced action.
	//
	PCFIX_ACTION Action;
} SEQUENCE_ENTRY, *PSEQUENCE_ENTRY;

#define SEQUENCE_ACTION_SIGNATURE 'AqeS'

typedef struct _SEQUENCE_ACTION
{
	//
	// Always set to SEQUENCE_ACTION_SIGNATURE.
	//
	DWORD Signature;

	CFIX_ACTION Base;

	volatile LONG ReferenceCount;

	struct
	{
		CRITICAL_SECTION Lock;
		LIST_ENTRY ListHead;
		LONG Count;
	} Entries;
} SEQUENCE_ACTION, *PSEQUENCE_ACTION;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static VOID CfixsDeleteSequenceAction( 
	__in PSEQUENCE_ACTION Action 
	)
{
	//
	// Unreference and delete all entries.
	//
	PLIST_ENTRY Entry = Action->Entries.ListHead.Flink;

	while ( Entry != &Action->Entries.ListHead )
	{
		PSEQUENCE_ENTRY SeqEntry = CONTAINING_RECORD(
			Entry,
			SEQUENCE_ENTRY,
			ListEntry );
		PLIST_ENTRY NextEntry = Entry->Flink;

		SeqEntry->Action->Dereference( SeqEntry->Action );
		free( SeqEntry );

		Entry = NextEntry;
	}

	DeleteCriticalSection( &Action->Entries.Lock );
	free( Action );
}

static VOID CfixsReferenceSequenceAction(
	__in PCFIX_ACTION This
	)
{
	PSEQUENCE_ACTION Action = CONTAINING_RECORD(
		This,
		SEQUENCE_ACTION,
		Base );
	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == SEQUENCE_ACTION_SIGNATURE );

	InterlockedIncrement( &Action->ReferenceCount );
}

static VOID CfixsDereferenceSequenceAction(
	__in PCFIX_ACTION This
	)
{
	PSEQUENCE_ACTION Action = CONTAINING_RECORD(
		This,
		SEQUENCE_ACTION,
		Base );
	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == SEQUENCE_ACTION_SIGNATURE );

	if ( 0 == InterlockedDecrement( &Action->ReferenceCount ) )
	{
		CfixsDeleteSequenceAction( Action );
	}
}

static HRESULT CfixsRunSequenceAction(
	__in PCFIX_ACTION This,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	PSEQUENCE_ACTION Action = CONTAINING_RECORD(
		This,
		SEQUENCE_ACTION,
		Base );
	PCFIX_ACTION *ActionsArray = NULL;
	UINT ActionsCount;
	HRESULT Hr;
	UINT Index = 0;
	if ( ! CfixIsValidAction( This ) ||
		 ! CfixIsValidContext( Context ) ||
		 Action->Signature != SEQUENCE_ACTION_SIGNATURE )
	{
		return E_INVALIDARG;
	}

	//
	// Within protection of the lock, collect the list
	// of actions and stabilize each pointer.
	//
	EnterCriticalSection( &Action->Entries.Lock );

	ActionsCount = Action->Entries.Count;

	if ( ActionsCount == 0 )
	{
		//
		// Empty sequence -> NOP.
		//
		Hr = S_OK;
	}
	else
	{
		ActionsArray = malloc( ActionsCount * sizeof( PCFIX_ACTION ) );
		
		if ( ! ActionsArray )
		{
			Hr = E_OUTOFMEMORY;
		}
		else
		{
			PLIST_ENTRY Entry = Action->Entries.ListHead.Flink;
			Index = 0;
			while ( Entry != &Action->Entries.ListHead )
			{
				PSEQUENCE_ENTRY SeqEntry = CONTAINING_RECORD(
					Entry,
					SEQUENCE_ENTRY,
					ListEntry );

				//
				// Stabilize pointer so it is guaranteed to remain valid
				// after leaving the lock.
				//
				SeqEntry->Action->Reference( SeqEntry->Action );
				ActionsArray[ Index++ ] = SeqEntry->Action;

				Entry = Entry->Flink;
			}

			Hr = S_OK;
		}

		ASSERT( ( E_OUTOFMEMORY != Hr ) == ( Index == ActionsCount ) );
	}

	LeaveCriticalSection( &Action->Entries.Lock );

	if ( FAILED( Hr ) || ActionsCount == 0 )
	{
		return Hr;
	}

	//
	// Now run actions. We do not own the lock any more.
	// 
	for ( Index = 0; Index < ActionsCount; Index++ )
	{
		Hr = ActionsArray[ Index ]->Run( ActionsArray[ Index ], Context );
		if ( FAILED( Hr ) )
		{
			break;
		}
	}

	//
	// Release references.
	//
	for ( Index = 0; Index < ActionsCount; Index++ )
	{
		ActionsArray[ Index ]->Dereference( ActionsArray[ Index ] );
	}
	
	free( ActionsArray );
	return Hr;
}

/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */
CFIXAPI HRESULT CFIXCALLTYPE CfixCreateSequenceAction(
	__out PCFIX_ACTION *Action
	)
{
	PSEQUENCE_ACTION NewAction = NULL;
	HRESULT Hr = E_UNEXPECTED;
	if ( ! Action )
	{
		return E_INVALIDARG;
	}

	//
	// Allocate.
	//
	NewAction = malloc( sizeof( SEQUENCE_ACTION ) );
	if ( ! NewAction )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	NewAction->Signature		= SEQUENCE_ACTION_SIGNATURE;
	NewAction->ReferenceCount	= 1;
	NewAction->Entries.Count	= 0;

	NewAction->Base.Version		= CFIX_ACTION_VERSION;
	NewAction->Base.Run			= CfixsRunSequenceAction;
	NewAction->Base.Reference	= CfixsReferenceSequenceAction;
	NewAction->Base.Dereference	= CfixsDereferenceSequenceAction;

	InitializeListHead( &NewAction->Entries.ListHead );
	InitializeCriticalSection( &NewAction->Entries.Lock );

	*Action = &NewAction->Base;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) && NewAction )
	{
		free( NewAction );
	}

	return Hr;
}

CFIXAPI HRESULT CFIXCALLTYPE CfixAddEntrySequenceAction(
	__in PCFIX_ACTION SequenceAction,
	__in PCFIX_ACTION ActionToAdd
	)
{
	PSEQUENCE_ACTION Action = CONTAINING_RECORD(
		SequenceAction,
		SEQUENCE_ACTION,
		Base );
	PSEQUENCE_ENTRY Entry;
	
	if ( ! CfixIsValidAction( SequenceAction ) ||
		 ! CfixIsValidAction( ActionToAdd ) ||
		 ActionToAdd == SequenceAction ||
		 Action->Signature != SEQUENCE_ACTION_SIGNATURE  )
	{
		return E_INVALIDARG;
	}

	//
	// Allocate an entry...
	//
	Entry = malloc( sizeof( SEQUENCE_ACTION ) );
	if ( ! Entry )
	{
		return E_OUTOFMEMORY;
	}

	//
	// ...and enlist it.
	//
	ActionToAdd->Reference( ActionToAdd );
	Entry->Action = ActionToAdd;

	EnterCriticalSection( &Action->Entries.Lock );
	
	InsertTailList( &Action->Entries.ListHead, &Entry->ListEntry );
	Action->Entries.Count++;

	LeaveCriticalSection( &Action->Entries.Lock );

	return S_OK;
}