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

static HRESULT CfixsSetCurrentFilament(
	__in PCFIXP_FILAMENT NewFilament,
	__in BOOL DerivedFromDefaultFilament,
	__out_opt PCFIXP_FILAMENT *Prev
	);

//
// Slot for holding the current CFIXP_FILAMENT during
// testcase execution.
//
// The lowest bit is used to indicate whether the filament
// is derived from the default filament.
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

//
// Global filament to use as fallback. May be NULL if feature is 
// disabled.
//
static struct _CFIXP_DEFAULT_FILAMENT
{
	PCFIXP_FILAMENT Filament;
	CRITICAL_SECTION Lock;
} CfixsDefaultFilament = { NULL };

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

static HRESULT CfixsSetDefaultFilament(
	__in_opt PCFIXP_FILAMENT Filament
	)
{
	HRESULT Hr;

	EnterCriticalSection( &CfixsDefaultFilament.Lock );
	
	if ( CfixsDefaultFilament.Filament != NULL && Filament != NULL )
	{
		return CFIX_E_DEFAULT_FILAMENT_CONFLICT;
	}
	else if ( CfixsDefaultFilament.Filament == NULL && Filament == NULL )
	{
		Hr = S_OK;
	}
	else
	{
		CfixsDefaultFilament.Filament = Filament;
		Hr = S_OK;
	}

	LeaveCriticalSection( &CfixsDefaultFilament.Lock );

	return Hr;
}

static void CfixsGetTlsFilament(
	__out PCFIXP_FILAMENT *Filament,
	__out PBOOL DerivedFromDefaultFilament
	)
{
	PVOID Value = TlsGetValue( CfixsTlsSlotForFilament );
	
	*Filament = ( PCFIXP_FILAMENT ) ( ( DWORD_PTR ) Value & ~1 );
	*DerivedFromDefaultFilament = ( BOOL ) ( ( DWORD_PTR ) Value & 1 );
}

static void CfixsSetTlsFilamant(
	__in PCFIXP_FILAMENT Filament,
	__in BOOL DerivedFromDefaultFilament
	)
{
	PVOID Value;

	ASSERT( Filament != NULL || ! DerivedFromDefaultFilament );
	
	Value = ( PVOID ) ( ( DWORD_PTR ) Filament |
		( DerivedFromDefaultFilament ? 1 : 0 ) );
	TlsSetValue( CfixsTlsSlotForFilament, Value );
}

static HRESULT CfixsGetCurrentFilament(
	__in BOOL DeriveDefaultIfUnavailable,
	__out_opt PBOOL DerivedFromDefaultFilament,
	__out PCFIXP_FILAMENT *Filament
	)
{
	BOOL DerivedFromDefault;

	if ( ! Filament )
	{
		return E_INVALIDARG;
	}

	CfixsGetTlsFilament( Filament, &DerivedFromDefault );
	
	if ( *Filament )
	{
		if ( DerivedFromDefaultFilament )
		{
			*DerivedFromDefaultFilament = DerivedFromDefault;
		}

		return S_OK;
	}
	else if ( DeriveDefaultIfUnavailable &&
		CfixsDefaultFilament.Filament != NULL )
	{
		HRESULT Hr;

		//
		// Accept default filament as current filament.
		//
		// N.B. There is a slight chance for a race condition here:
		// It is possible that between us having checked for the
		// existance of a default filament and having registered the
		// current thread as child in this filament, the enclosing test
		// finished and has revoked the default filament.
		//
		// For this reason, we have to grab the lock while calling 
		// CfixpSetCurrentFilament.
		//
		EnterCriticalSection( &CfixsDefaultFilament.Lock );
		
		if ( CfixsDefaultFilament.Filament != NULL )
		{
			//
			// Register as child.
			//
			Hr = CfixsSetCurrentFilament(
				CfixsDefaultFilament.Filament,
				TRUE,
				NULL );

			if ( SUCCEEDED( Hr ) )
			{
				Hr = CfixsGetCurrentFilament( 
					FALSE, 
					DerivedFromDefaultFilament,
					Filament );
			}
		}
		else
		{
			Hr = CFIX_E_UNKNOWN_THREAD;
		}

		LeaveCriticalSection( &CfixsDefaultFilament.Lock );

		return Hr;
	}

	return CFIX_E_UNKNOWN_THREAD;
}

static HRESULT CfixsSetCurrentFilament(
	__in PCFIXP_FILAMENT NewFilament,
	__in BOOL DerivedFromDefaultFilament,
	__out_opt PCFIXP_FILAMENT *Prev
	)
{
	HRESULT Hr;
	PCFIXP_FILAMENT OldFilament;

	( VOID ) CfixsGetCurrentFilament( FALSE, NULL, &OldFilament );

	if ( OldFilament != NULL )
	{
		OldFilament->ExecutionContext->Dereference( OldFilament->ExecutionContext );
	}

	if ( NewFilament != NULL &&
		 GetCurrentThreadId() != NewFilament->MainThreadId )
	{
		HANDLE CurrentThread;
		
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
			CfixsSetTlsFilamant( NULL, FALSE );
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

		if ( ! CfixpFlagOn( NewFilament->Flags, CFIXP_FILAMENT_FLAG_DEFAULT_FILAMENT ) )
		{
			//
			// Force-revoke any leftover default filament.
			//
			// N.B. Only applies when filaments are nested.
			//
			VERIFY( S_OK == CfixsSetDefaultFilament( NULL ) );
		}
		else if ( GetCurrentThreadId() == NewFilament->MainThreadId &&
			 CfixpFlagOn( NewFilament->Flags, CFIXP_FILAMENT_FLAG_DEFAULT_FILAMENT ) )
		{
			//
			// Main thread registering default filament.
			//
			Hr = CfixsSetDefaultFilament( NewFilament );
			if ( FAILED( Hr ) )
			{
				//
				// Main reason for failure: conflicting default filament.
				//
				return Hr;
			}
		}
	}
	else if ( OldFilament != NULL )
	{
		if ( GetCurrentThreadId() == OldFilament->MainThreadId &&
			 CfixpFlagOn( OldFilament->Flags, CFIXP_FILAMENT_FLAG_DEFAULT_FILAMENT ) )
		{
			//
			// Main thread revoking default filament.
			//
			VERIFY( S_OK == CfixsSetDefaultFilament( NULL ) );
		}
	}

	CfixsSetTlsFilamant( NewFilament, DerivedFromDefaultFilament );

	if ( Prev )
	{
		*Prev = OldFilament;
	}

	return S_OK;
}

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */

BOOL CfixpSetupFilamentTls()
{
	InitializeCriticalSection( &CfixsDefaultFilament.Lock );

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
	DeleteCriticalSection( &CfixsDefaultFilament.Lock );

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
	return CfixsSetCurrentFilament( NewFilament, FALSE, Prev );
}

HRESULT CfixpGetCurrentFilament(
	__out PCFIXP_FILAMENT *Filament,
	__out_opt PBOOL DerivedFromDefault
	)
{
	return CfixsGetCurrentFilament( TRUE, DerivedFromDefault, Filament );
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

VOID CfixpCleanupLeakedFilamentForDetachingThread()
{
	//
	// If a thread used the default filament (i.e. auto-registered)
	// and the thread exists prematurely because of an unhandles
	// exception or a similar event, its filament will be leaked.
	//
	// Called from DllMain, this routine will sweep such filaments
	// that would otherwise be leaked.
	//
	HRESULT Hr;
	PCFIXP_FILAMENT Filament;
	BOOL IsDerivedFromDefault;

	Hr = CfixsGetCurrentFilament(
		FALSE,
		&IsDerivedFromDefault,
		&Filament );
	if ( S_OK == Hr && Filament != NULL )
	{
		CfixsSetCurrentFilament( NULL, FALSE, NULL );
	}
}

HRESULT CfixRegisterThread( 
	__reserved PVOID Reserved 
	)
{
	BOOL DerivedFromDefault;
	PCFIXP_FILAMENT Filament;
	HRESULT Hr;
	
	if ( Reserved != NULL )
	{
		return E_INVALIDARG;
	}
	
	Hr = CfixpGetCurrentFilament( &Filament, &DerivedFromDefault );

	if ( Hr == CFIX_E_UNKNOWN_THREAD )
	{
		return E_UNEXPECTED;
	}
	else if ( SUCCEEDED( Hr ) && ! DerivedFromDefault )
	{
		//
		// Already registered.
		//
		return E_UNEXPECTED;
	}
	else
	{
		return Hr;
	}
}