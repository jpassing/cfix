/*----------------------------------------------------------------------
 * Purpose:
 *		Implementation of the default message resolver
 *
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

#define JPDIAGAPI 

#include <stdlib.h>
#include "internal.h"
#include "list.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#include <shlwapi.h>
#pragma warning( pop )

#define MAX_DLLNAME 64
#define MAX_MESSAGE_CCH 0xFFFF

#define RESOLVER_ALL_FLAGS ( JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS |	\
							 JPDIAG_MSGRES_NO_SYSTEM |				\
							 JPDIAG_MSGRES_FALLBACK_TO_DEFAULT )

typedef struct _RESOLVER
{
	JPDIAG_MESSAGE_RESOLVER Base;

	volatile LONG ReferenceCount;

	struct
	{
		//
		// Lock guarding the list.
		//
		CRITICAL_SECTION Lock;

		//
		// List of REGISTERED_DLLs.
		//
		LIST_ENTRY ListHead;
	} RegisteredDlls;
} RESOLVER, *PRESOLVER;

typedef struct _REGISTERED_DLL
{
	LIST_ENTRY ListEntry;

	//
	// Name of DLL.
	//
	WCHAR Name[ MAX_DLLNAME ];

	//
	// Module handle. The loadcount of the DLL must have been 
	// incremented.
	//
	HMODULE Module;
} REGISTERED_DLL, *PREGISTERED_DLL;


/*----------------------------------------------------------------------
 *
 * Private helper routines
 *
 */
#define JpdiagsIsValidResolver( store ) \
	( ( store != NULL && store->Base.Size == sizeof( RESOLVER ) ) )

/*++
	Routine description:
		Find a REGISTERED_DLL record in the list. As the list is
		assumed to be small usuall< 0-5 entries, sequential
		search is fine.
--*/
static PREGISTERED_DLL JpdiagsFindRegisteredDll(
	__in LPCWSTR Name,
	__in PLIST_ENTRY ListHead
	)
{
	PLIST_ENTRY Entry = ListHead->Flink;

	_ASSERTE( Name );
	_ASSERTE( ListHead );

	while ( Entry != ListHead )
	{
		PREGISTERED_DLL RegDll = 
			CONTAINING_RECORD( Entry, REGISTERED_DLL, ListEntry );

		if ( 0 == _wcsicmp( Name, RegDll->Name ) )
		{
			return RegDll;
		}

		Entry = Entry->Flink;
	}

	return NULL;
}

static VOID JpdiagsDeleteRegisteredDllEntry(
	__in PREGISTERED_DLL RegDll
)
{
	_ASSERTE( RegDll );

	if ( RegDll->Module )
	{
		_VERIFY( FreeLibrary( RegDll->Module ) );
	}

	if ( RegDll->ListEntry.Flink && RegDll->ListEntry.Blink )
	{
		RemoveEntryList( &RegDll->ListEntry );
	}

	JpdiagpFree( RegDll );
}

/*++
	Routine description:
		Try to resolve a message from a given module or from system
		(if Module is NULL)
--*/
static HRESULT JpdiagsFormatMessage(
	__in HMODULE Module,
	__in DWORD MessageId,
	__in BOOL UseInsertionStrings,
	__in_opt PCTSTR* InsertionStrings,
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer
	)
{
	BOOL Success;
	
	_ASSERTE( BufferSizeInChars );
	_ASSERTE( BufferSizeInChars < MAX_MESSAGE_CCH );
	_ASSERTE( Buffer );

	Success = FormatMessage(
		( ( UseInsertionStrings 
			? 0 
			: FORMAT_MESSAGE_IGNORE_INSERTS ) |
		  ( Module 
			? FORMAT_MESSAGE_FROM_HMODULE 
			: FORMAT_MESSAGE_FROM_SYSTEM ) |
		  FORMAT_MESSAGE_ARGUMENT_ARRAY ),
		( Module
		  ? Module
		  : NULL ),
		MessageId,
		0,
		Buffer,
		( DWORD ) BufferSizeInChars,
		( va_list* ) ( DWORD_PTR* ) InsertionStrings ) > 0;

	if ( Success )
	{
		return S_OK;
	}
	else 
	{
		DWORD Err = GetLastError();
		if ( ERROR_INSUFFICIENT_BUFFER == Err || ERROR_MORE_DATA == Err )
		{
			return JPDIAG_E_BUFFER_TOO_SMALL;
		}
		else
		{
			return HRESULT_FROM_WIN32( Err );
		}
	}
}

/*----------------------------------------------------------------------
 *
 * JPDIAG_MESSAGE_RESOLVER methods
 *
 */

HRESULT JpdiagsRegisterMessageDll(
	__in PJPDIAG_MESSAGE_RESOLVER This,
	__in PCWSTR Name,
	__in DWORD Flags,
	__reserved DWORD Priority
	)
{
	PRESOLVER Resolver = ( PRESOLVER ) This;
	HRESULT Hr = E_UNEXPECTED;
	PREGISTERED_DLL RegDll = NULL;

	if ( ! JpdiagsIsValidResolver( Resolver ) ||
		 ! JpdiagpIsStringValid( Name, 5, MAX_PATH, FALSE ) ||
		 ( Flags != 0 && Flags != JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH ) ||
		 Priority != 0 )
	{
		Hr = E_INVALIDARG;
		goto Cleanup;
	}

	//
	// Create a REGISTERED_DLL struct.
	//
	RegDll = ( PREGISTERED_DLL ) JpdiagpMalloc(
		sizeof( REGISTERED_DLL ), TRUE );
	if ( ! RegDll )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	if ( Flags & JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH )
	{
		//
		// Name is a path - retrieve name portion of it.
		//
		WCHAR tmpName[ MAX_PATH ];
		Hr = StringCchCopy( tmpName, _countof( tmpName ), Name );
		if ( FAILED( Hr ) ) goto Cleanup;

		PathStripPath( tmpName );

		Hr = StringCchCopy( RegDll->Name, MAX_DLLNAME, tmpName );
		if ( FAILED( Hr ) ) goto Cleanup;

		//
		// Load with altered search path.
		//
		RegDll->Module = LoadLibraryEx(
			Name,
			NULL,
			LOAD_WITH_ALTERED_SEARCH_PATH );
	}
	else
	{
		Hr = StringCchCopy( RegDll->Name, MAX_DLLNAME, Name );
		if ( FAILED( Hr ) ) goto Cleanup;

		//
		// Ordinary load/increment load count.
		//
		RegDll->Module = LoadLibrary( Name );
	}

	if ( ! RegDll->Module )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	//
	// Insert it to list.
	//
	EnterCriticalSection( &Resolver->RegisteredDlls.Lock );

	if ( NULL != JpdiagsFindRegisteredDll( 
		Name, &Resolver->RegisteredDlls.ListHead ) )
	{
		Hr = JPDIAG_E_ALREADY_REGISTERED; 
	}
	else
	{
		InsertTailList( 
			&Resolver->RegisteredDlls.ListHead, 
			&RegDll->ListEntry );

		Hr = S_OK;
	}

	_ASSERTE( ! IsListEmpty( &Resolver->RegisteredDlls.ListHead ) );

	LeaveCriticalSection( &Resolver->RegisteredDlls.Lock );

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( RegDll )
		{
			JpdiagsDeleteRegisteredDllEntry( RegDll );
		}
	}

	return Hr;
}

HRESULT JpdiagsUnregisterMessageDll(
	__in PJPDIAG_MESSAGE_RESOLVER This,
	__in PCWSTR Name
	)
{
	PRESOLVER Resolver = ( PRESOLVER ) This;
	HRESULT Hr = E_UNEXPECTED;
	PREGISTERED_DLL RegDll = NULL;

	if ( ! JpdiagsIsValidResolver( Resolver ) ||
		 ! JpdiagpIsStringValid( Name, 5, MAX_DLLNAME - 1, FALSE ) )
	{
		return E_INVALIDARG;
	}

	EnterCriticalSection( &Resolver->RegisteredDlls.Lock );

	RegDll = JpdiagsFindRegisteredDll( 
		Name, &Resolver->RegisteredDlls.ListHead );
	if ( ! RegDll )
	{
		Hr = JPDIAG_E_DLL_NOT_REGISTERED;
	}
	else
	{
		JpdiagsDeleteRegisteredDllEntry( RegDll );
		_ASSERTE( NULL ==
			JpdiagsFindRegisteredDll( 
				Name, &Resolver->RegisteredDlls.ListHead ) );

		Hr = S_OK;
	}

	LeaveCriticalSection( &Resolver->RegisteredDlls.Lock );

	return Hr;
}

HRESULT JpdiagsResolveMessage(
	__in PJPDIAG_MESSAGE_RESOLVER This,
	__in DWORD MessageId,
	__in DWORD Flags,
	__in_opt PCTSTR* InsertionStrings,
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer
	)
{
	PRESOLVER Resolver = ( PRESOLVER ) This;
	PLIST_ENTRY Entry;
	BOOL Resolved = FALSE;
	BOOL UseInsertionStrings = 
		( 0 == ( Flags & JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS ) );
	BOOL FallbackToDefault = 
		( Flags & JPDIAG_MSGRES_FALLBACK_TO_DEFAULT );
	HRESULT Hr = S_OK;

	if ( ! JpdiagsIsValidResolver( Resolver ) ||
		( Flags & (~RESOLVER_ALL_FLAGS) ) != 0 ||
	    BufferSizeInChars == 0 ||
		BufferSizeInChars > MAX_MESSAGE_CCH ||
		! Buffer )
	{
		return E_INVALIDARG;
	}

	//
	// First, try system.
	//
	if ( ! ( Flags & JPDIAG_MSGRES_NO_SYSTEM ) )
	{
		Hr = JpdiagsFormatMessage(
			NULL,	// System
			MessageId,
			UseInsertionStrings,
			InsertionStrings,
			BufferSizeInChars,
			Buffer );
		if ( S_OK == Hr)
		{
			//
			// Msg found.
			//
			return S_OK;
		}
		else if ( JPDIAG_E_BUFFER_TOO_SMALL == Hr )
		{
			//
			// Buffer too small, bail out.
			//
			return Hr;
		}
		else
		{
			//
			// Continue search.
			//
		}
	}

	//
	// Try registered DLLs.
	//
	EnterCriticalSection( &Resolver->RegisteredDlls.Lock );
	
	Entry = Resolver->RegisteredDlls.ListHead.Flink;
	while ( Entry != &Resolver->RegisteredDlls.ListHead && ! Resolved )
	{
		PREGISTERED_DLL RegDll = 
			CONTAINING_RECORD( Entry, REGISTERED_DLL, ListEntry );

		Hr = JpdiagsFormatMessage(
			RegDll->Module,
			MessageId,
			UseInsertionStrings,
			InsertionStrings,
			BufferSizeInChars,
			Buffer );
		if ( S_OK == Hr )
		{
			//
			// Msg found.
			//
			Resolved = TRUE;
		}
		else if ( JPDIAG_E_BUFFER_TOO_SMALL == Hr )
		{
			//
			// Buffer too small, bail out.
			//
			break;
		}
		else
		{
			//
			// Continue search.
			//
		}

		Entry = Entry->Flink;
	}
	LeaveCriticalSection( &Resolver->RegisteredDlls.Lock );

	if ( Resolved )
	{
		return S_OK;
	}
	else if ( JPDIAG_E_BUFFER_TOO_SMALL == Hr )
	{
		return Hr;
	}
	else
	{
		//
		// Still no message found.
		//
		if ( FallbackToDefault )
		{
			//
			// Use default message.
			//
			Hr = StringCchPrintf(
				Buffer,
				BufferSizeInChars,
				L"0x%08X",
				MessageId );
			if ( STRSAFE_E_INSUFFICIENT_BUFFER == Hr )
			{
				return JPDIAG_E_BUFFER_TOO_SMALL;
			}
			else
			{
				return S_OK;
			}
		}
		else
		{
			return JPDIAG_E_UNKNOWN_MESSAGE;
		}
	}
}

HRESULT JpdiagsDeleteResolver(
	__in PRESOLVER Resolver
	)
{
	PLIST_ENTRY Entry;
	if ( ! JpdiagsIsValidResolver( Resolver ) )
	{
		return E_INVALIDARG;
	}

	//
	// Free all entries.
	//
	EnterCriticalSection( &Resolver->RegisteredDlls.Lock );
	
	Entry = Resolver->RegisteredDlls.ListHead.Flink;
	while ( Entry != &Resolver->RegisteredDlls.ListHead )
	{
		PREGISTERED_DLL RegDll = 
			CONTAINING_RECORD( Entry, REGISTERED_DLL, ListEntry );

		Entry = Entry->Flink;
		
		JpdiagsDeleteRegisteredDllEntry( RegDll );
	}
	LeaveCriticalSection( &Resolver->RegisteredDlls.Lock );

	_ASSERTE( IsListEmpty( &Resolver->RegisteredDlls.ListHead ) );

	//
	// Free object.
	//
	DeleteCriticalSection( &Resolver->RegisteredDlls.Lock );
	JpdiagpFree( Resolver );

	return S_OK;
}

static VOID JpdiagsReferenceResolver(
	__in PJPDIAG_MESSAGE_RESOLVER This
	)
{
	PRESOLVER Resolver = ( PRESOLVER ) This;
	_ASSERTE( JpdiagsIsValidResolver( Resolver ) );

	InterlockedIncrement( &Resolver->ReferenceCount );
}

static VOID JpdiagsDereferenceResolver(
	__in PJPDIAG_MESSAGE_RESOLVER This
	)
{
	PRESOLVER Resolver = ( PRESOLVER ) This;

	_ASSERTE( JpdiagsIsValidResolver( Resolver ) );

	if ( 0 == InterlockedDecrement( &Resolver->ReferenceCount ) )
	{
		_VERIFY( S_OK == JpdiagsDeleteResolver( Resolver ) );
	}
}

/*----------------------------------------------------------------------
 *
 * Public
 *
 */

HRESULT JPDIAGCALLTYPE JpdiagCreateMessageResolver(
	__out PJPDIAG_MESSAGE_RESOLVER *MessageResolver
	)
{
	PRESOLVER Resolver = NULL;
	if ( ! MessageResolver )
	{
		return E_INVALIDARG;
	}

	*MessageResolver = NULL;

	//
	// Allocate.
	//
	Resolver = JpdiagpMalloc( sizeof( RESOLVER ), TRUE );
	if ( ! Resolver )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize.
	//
	Resolver->ReferenceCount = 1;
	Resolver->Base.Size = sizeof( RESOLVER );
	
	Resolver->Base.Reference			= JpdiagsReferenceResolver;
	Resolver->Base.Dereference			= JpdiagsDereferenceResolver;
	Resolver->Base.RegisterMessageDll	= JpdiagsRegisterMessageDll;
	Resolver->Base.ResolveMessage		= JpdiagsResolveMessage;
	Resolver->Base.UnregisterMessageDll = JpdiagsUnregisterMessageDll;

	InitializeCriticalSection( &Resolver->RegisteredDlls.Lock );
	InitializeListHead( &Resolver->RegisteredDlls.ListHead );

	*MessageResolver = &Resolver->Base;

	return S_OK;
}
