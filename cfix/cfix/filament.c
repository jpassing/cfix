/*----------------------------------------------------------------------
 * Purpose:
 *		Current execution context management.
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

#define CFIXAPI

#include "cfixp.h"
#include <stdlib.h>

//
// Slot for holding the current CFIXP_FILAMENT during
// testcase execution.
//
static DWORD CfixsTlsSlotForFilament = TLS_OUT_OF_INDEXES;

//
// Separate slots are needed for storage as these "survive"
// multiple filaments.
//
// Only used on main thread.
//
static DWORD CfixsDefaultStorageSlot = TLS_OUT_OF_INDEXES;
static DWORD CfixsReservedForCcStorageSlot = TLS_OUT_OF_INDEXES;

static HRESULT CfixsGetCurrentThreadHandle( 
	__out HANDLE *Thread
	)
{
	if ( ! DuplicateHandle(
		GetCurrentProcess(),
		GetCurrentThread(),
		GetCurrentProcess(),
		Thread,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS ) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
	else
	{
		return S_OK;
	}
}

#pragma warning( push )
#pragma warning( disable: 6386 )	// False buffer overflow warning.

static HRESULT CfixsRegisterChildThreadFilament(
	__in PCFIXP_FILAMENT Filament,
	__in HANDLE Thread
	)
{
	HRESULT Hr;
	EnterCriticalSection( &Filament->ChildThreads.Lock );
	
	if ( Filament->ChildThreads.ThreadCount ==
		_countof( Filament->ChildThreads.Threads ) )
	{
		Hr = CFIX_E_TOO_MANY_CHILD_THREADS;
		goto Cleanup;
	}

	Filament->ChildThreads.Threads[ 
		Filament->ChildThreads.ThreadCount++ ] = Thread;

	Hr = S_OK;

Cleanup:
	LeaveCriticalSection( &Filament->ChildThreads.Lock );

	return Hr;
}

#pragma warning( pop )

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */
BOOL CfixpSetupFilamentTls()
{
	CfixsTlsSlotForFilament			= TlsAlloc();
	CfixsDefaultStorageSlot			= TlsAlloc();
	CfixsReservedForCcStorageSlot	= TlsAlloc();
	
	return 
		CfixsTlsSlotForFilament != TLS_OUT_OF_INDEXES &&
		CfixsDefaultStorageSlot != TLS_OUT_OF_INDEXES &&
		CfixsReservedForCcStorageSlot != TLS_OUT_OF_INDEXES;
}

BOOL CfixpTeardownFilamentTls()
{
	return 
		TlsFree( CfixsTlsSlotForFilament ) &&
		TlsFree( CfixsDefaultStorageSlot ) &&
		TlsFree( CfixsReservedForCcStorageSlot );
}

VOID CfixpInitializeFilament(
	__in PCFIX_EXECUTION_CONTEXT ExecutionContext,
	__in ULONG MainThreadId,
	__in ULONG Flags,
	__in BOOL RestoreStorage,
	__out PCFIXP_FILAMENT Filament
	)
{
	ZeroMemory( Filament, sizeof( CFIXP_FILAMENT ) );

	Filament->ExecutionContext	= ExecutionContext;
	Filament->MainThreadId		= MainThreadId;
	Filament->Flags				= Flags;

	InitializeCriticalSection( &Filament->ChildThreads.Lock );

	if ( RestoreStorage && ( GetCurrentThreadId() == MainThreadId ) )
	{
		//
		// Load values from previous incarnation.
		//
		Filament->Storage.DefaultSlot = TlsGetValue( CfixsDefaultStorageSlot );
		Filament->Storage.CcSlot	  = TlsGetValue( CfixsReservedForCcStorageSlot );
	}
}

VOID CfixpDestroyFilament(
	__in PCFIXP_FILAMENT Filament
	)
{
	ULONG Index;

	//
	// Close all handles to child threads.
	//
	for ( Index = 0; Index < Filament->ChildThreads.ThreadCount; Index++ )
	{
		VERIFY( CloseHandle( Filament->ChildThreads.Threads[ Index ] ) );
	}

	DeleteCriticalSection( &Filament->ChildThreads.Lock );

	if ( GetCurrentThreadId() == Filament->MainThreadId )
	{
		TlsSetValue( CfixsDefaultStorageSlot, Filament->Storage.DefaultSlot );
		TlsSetValue( CfixsReservedForCcStorageSlot, Filament->Storage.CcSlot );
	}
}

HRESULT CfixpSetCurrentFilament(
	__in PCFIXP_FILAMENT NewFilament,
	__out_opt PCFIXP_FILAMENT *Prev
	)
{
	PCFIXP_FILAMENT OldFilament;

	( VOID ) CfixpGetCurrentFilament( &OldFilament );

	if ( OldFilament != NULL )
	{
		OldFilament->ExecutionContext->Dereference( OldFilament->ExecutionContext );
	}
	else
	{
		ASSERT( NewFilament != NULL );
	}

	if ( NewFilament != NULL &&
		 GetCurrentThreadId() != NewFilament->MainThreadId )
	{
		HANDLE CurrentThread;
		HRESULT Hr;

		//
		// N.B. Obtain a "real" handle s.t. it can be used from a different
		// thread.
		//
		Hr = CfixsGetCurrentThreadHandle( &CurrentThread );
		if ( FAILED( Hr ) )
		{
			return Hr;
		}

		//
		// This is not the main thread - so register as child thread.
		//
		// N.B. Use real handle to thread, not a pseudo-handle. The
		// handle will be closed by CfixpDestroyFilament.
		//
		Hr = CfixsRegisterChildThreadFilament( NewFilament, CurrentThread );
		if ( FAILED( Hr ) )
		{
			( VOID ) TlsSetValue( CfixsTlsSlotForFilament, NULL );
			( VOID ) CloseHandle( CurrentThread );
			return Hr;
		}
	}

	//
	// N.B. Resetting the filament does not impact the child handle
	// list in the filament - the list is maintained until the filament
	// is destroyed.
	//

	if ( NewFilament != NULL )
	{
		NewFilament->ExecutionContext->Reference( NewFilament->ExecutionContext );
	}
	else
	{
		ASSERT( OldFilament != NULL );
	}

	( VOID ) TlsSetValue( CfixsTlsSlotForFilament, NewFilament );

	if ( Prev )
	{
		*Prev = OldFilament;
	}

	return S_OK;
}

HRESULT CfixpGetCurrentFilament(
	__out PCFIXP_FILAMENT *Filament
	)
{
	if ( ! Filament )
	{
		return E_INVALIDARG;
	}

	*Filament = ( PCFIXP_FILAMENT ) TlsGetValue( CfixsTlsSlotForFilament );

	if ( *Filament )
	{
		return S_OK;
	}
	else
	{
		return CFIX_E_UNKNOWN_THREAD;
	}
}

HRESULT CfixpJoinChildThreadsFilament(
	__in PCFIXP_FILAMENT Filament,
	__in ULONG Timeout
	)
{
	HRESULT Hr = S_OK;
	DWORD WaitResult;

	EnterCriticalSection( &Filament->ChildThreads.Lock );
	
	if ( Filament->ChildThreads.ThreadCount > 0 )
	{
		WaitResult = WaitForMultipleObjects(
			Filament->ChildThreads.ThreadCount,
			Filament->ChildThreads.Threads,
			TRUE,
			Timeout );
		if ( WaitResult == WAIT_TIMEOUT )
		{
			Hr = CFIX_E_FILAMENT_JOIN_TIMEOUT;
		}
	}

	LeaveCriticalSection( &Filament->ChildThreads.Lock );

	return Hr;
}