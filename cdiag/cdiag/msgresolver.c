/*----------------------------------------------------------------------
 * Purpose:
 *		Implementation of the default message resolver
 *
 * Copyright:
 *		2007-2009 Johannes Passing (passing at users.sourceforge.net)
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

#define CDIAGAPI 

#include <stdlib.h>
#include "cdiagp.h"
#include "list.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#include <shlwapi.h>
#pragma warning( pop )

#define MAX_DLLNAME 64
#define MAX_MESSAGE_CCH 0xFFFF

#define CDIAGP_RESOLVER_ALL_FLAGS ( CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS |	\
							 CDIAG_MSGRES_NO_SYSTEM |				\
							 CDIAG_MSGRES_FALLBACK_TO_DEFAULT |	\
							 CDIAG_MSGRES_STRIP_NEWLINES )

typedef struct _CDIAGP_RESOLVER
{
	CDIAG_MESSAGE_RESOLVER Base;

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
} CDIAGP_RESOLVER, *PCDIAGP_RESOLVER;

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
#define CdiagsIsValidResolver( store ) \
	( ( store != NULL && store->Base.Size == sizeof( CDIAGP_RESOLVER ) ) )

/*++
	Routine description:
		Find a REGISTERED_DLL record in the list. As the list is
		assumed to be small usuall< 0-5 entries, sequential
		search is fine.
--*/
static PREGISTERED_DLL CdiagsFindRegisteredDll(
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

static VOID CdiagsDeleteRegisteredDllEntry(
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

	CdiagpFree( RegDll );
}

/*++
	Routine description:
		Try to resolve a message from a given module or from system
		(if Module is NULL)
--*/
static HRESULT CdiagsFormatMessage(
	__in_opt HMODULE Module,
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
			return CDIAG_E_BUFFER_TOO_SMALL;
		}
		else
		{
			return HRESULT_FROM_WIN32( Err );
		}
	}
}

/*----------------------------------------------------------------------
 *
 * CDIAG_MESSAGE_CDIAGP_RESOLVER methods
 *
 */

HRESULT CdiagsRegisterMessageDll(
	__in PCDIAG_MESSAGE_RESOLVER This,
	__in PCWSTR Name,
	__in DWORD Flags,
	__reserved DWORD Priority
	)
{
	PCDIAGP_RESOLVER Resolver = ( PCDIAGP_RESOLVER ) This;
	HRESULT Hr = E_UNEXPECTED;
	PREGISTERED_DLL RegDll = NULL;

	if ( ! CdiagsIsValidResolver( Resolver ) ||
		 ! CdiagpIsStringValid( Name, 5, MAX_PATH, FALSE ) ||
		 ( Flags != 0 && Flags != CDIAG_MSGRES_REGISTER_EXPLICIT_PATH ) ||
		 Priority != 0 )
	{
		Hr = E_INVALIDARG;
		goto Cleanup;
	}

	//
	// Create a REGISTERED_DLL struct.
	//
	RegDll = ( PREGISTERED_DLL ) CdiagpMalloc(
		sizeof( REGISTERED_DLL ), TRUE );
	if ( ! RegDll )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	if ( Flags & CDIAG_MSGRES_REGISTER_EXPLICIT_PATH )
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

	if ( NULL != CdiagsFindRegisteredDll( 
		Name, &Resolver->RegisteredDlls.ListHead ) )
	{
		Hr = CDIAG_E_ALREADY_REGISTERED; 
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
			CdiagsDeleteRegisteredDllEntry( RegDll );
		}
	}

	return Hr;
}

HRESULT CdiagsUnregisterMessageDll(
	__in PCDIAG_MESSAGE_RESOLVER This,
	__in PCWSTR Name
	)
{
	PCDIAGP_RESOLVER Resolver = ( PCDIAGP_RESOLVER ) This;
	HRESULT Hr = E_UNEXPECTED;
	PREGISTERED_DLL RegDll = NULL;

	if ( ! CdiagsIsValidResolver( Resolver ) ||
		 ! CdiagpIsStringValid( Name, 5, MAX_DLLNAME - 1, FALSE ) )
	{
		return E_INVALIDARG;
	}

	EnterCriticalSection( &Resolver->RegisteredDlls.Lock );

	RegDll = CdiagsFindRegisteredDll( 
		Name, &Resolver->RegisteredDlls.ListHead );
	if ( ! RegDll )
	{
		Hr = CDIAG_E_DLL_NOT_REGISTERED;
	}
	else
	{
		CdiagsDeleteRegisteredDllEntry( RegDll );
		_ASSERTE( NULL ==
			CdiagsFindRegisteredDll( 
				Name, &Resolver->RegisteredDlls.ListHead ) );

		Hr = S_OK;
	}

	LeaveCriticalSection( &Resolver->RegisteredDlls.Lock );

	return Hr;
}

VOID CdiagsStripNewlines(
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer
	)
{
	ULONG Index;

	//
	// Remove LFs and CRs.
	//
	for ( Index = 0; Index < BufferSizeInChars; Index++ )
	{
		if ( Buffer[ Index ] == L'\r' ||
			 Buffer[ Index ] == L'\n' )
		{
			Buffer[ Index ] = L' ';
		}
		else if ( Buffer[ Index ] == L'\0' )
		{
			break;
		}
	}

	//
	// Strip trailing spaces.
	//
	while ( Index > 0 )
	{
		Index--;
		if ( Buffer[ Index ] == L' ' )
		{
			Buffer[ Index ] = L'\0';
			break;
		}
	} 
}

HRESULT CdiagsResolveMessage(
	__in PCDIAG_MESSAGE_RESOLVER This,
	__in DWORD MessageId,
	__in DWORD Flags,
	__in_opt PCTSTR* InsertionStrings,
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer
	)
{
	PCDIAGP_RESOLVER Resolver = ( PCDIAGP_RESOLVER ) This;
	PLIST_ENTRY Entry;
	BOOL Resolved = FALSE;
	BOOL UseInsertionStrings = 
		( 0 == ( Flags & CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS ) );
	BOOL FallbackToDefault = 
		( Flags & CDIAG_MSGRES_FALLBACK_TO_DEFAULT );
	HRESULT Hr = S_OK;

	if ( ! CdiagsIsValidResolver( Resolver ) ||
		( Flags & (~CDIAGP_RESOLVER_ALL_FLAGS) ) != 0 ||
	    BufferSizeInChars == 0 ||
		BufferSizeInChars > MAX_MESSAGE_CCH ||
		! Buffer )
	{
		return E_INVALIDARG;
	}

	//
	// First, try system.
	//
	if ( ! ( Flags & CDIAG_MSGRES_NO_SYSTEM ) )
	{
		Hr = CdiagsFormatMessage(
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
			Resolved = TRUE;
		}
		else if ( CDIAG_E_BUFFER_TOO_SMALL == Hr )
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

	if ( ! Resolved )
	{
		//
		// Try registered DLLs.
		//
		EnterCriticalSection( &Resolver->RegisteredDlls.Lock );
		
		Entry = Resolver->RegisteredDlls.ListHead.Flink;
		while ( Entry != &Resolver->RegisteredDlls.ListHead && ! Resolved )
		{
			PREGISTERED_DLL RegDll = 
				CONTAINING_RECORD( Entry, REGISTERED_DLL, ListEntry );

			Hr = CdiagsFormatMessage(
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
			else if ( CDIAG_E_BUFFER_TOO_SMALL == Hr )
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
	}

	if ( Resolved )
	{
		if ( Flags & CDIAG_MSGRES_STRIP_NEWLINES )
		{
			CdiagsStripNewlines( BufferSizeInChars, Buffer );
		}

		return S_OK;
	}
	else if ( CDIAG_E_BUFFER_TOO_SMALL == Hr )
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
				return CDIAG_E_BUFFER_TOO_SMALL;
			}
			else
			{
				return S_OK;
			}
		}
		else
		{
			return CDIAG_E_UNKNOWN_MESSAGE;
		}
	}
}

HRESULT CdiagsDeleteResolver(
	__in PCDIAGP_RESOLVER Resolver
	)
{
	PLIST_ENTRY Entry;
	if ( ! CdiagsIsValidResolver( Resolver ) )
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
		
		CdiagsDeleteRegisteredDllEntry( RegDll );
	}
	LeaveCriticalSection( &Resolver->RegisteredDlls.Lock );

	_ASSERTE( IsListEmpty( &Resolver->RegisteredDlls.ListHead ) );

	//
	// Free object.
	//
	DeleteCriticalSection( &Resolver->RegisteredDlls.Lock );
	CdiagpFree( Resolver );

	return S_OK;
}

static VOID CdiagsReferenceResolver(
	__in PCDIAG_MESSAGE_RESOLVER This
	)
{
	PCDIAGP_RESOLVER Resolver = ( PCDIAGP_RESOLVER ) This;
	_ASSERTE( CdiagsIsValidResolver( Resolver ) );

	InterlockedIncrement( &Resolver->ReferenceCount );
}

static VOID CdiagsDereferenceResolver(
	__in PCDIAG_MESSAGE_RESOLVER This
	)
{
	PCDIAGP_RESOLVER Resolver = ( PCDIAGP_RESOLVER ) This;

	_ASSERTE( CdiagsIsValidResolver( Resolver ) );

	if ( 0 == InterlockedDecrement( &Resolver->ReferenceCount ) )
	{
		_VERIFY( S_OK == CdiagsDeleteResolver( Resolver ) );
	}
}

/*----------------------------------------------------------------------
 *
 * Public
 *
 */

HRESULT CDIAGCALLTYPE CdiagCreateMessageResolver(
	__out PCDIAG_MESSAGE_RESOLVER *MessageResolver
	)
{
	PCDIAGP_RESOLVER Resolver = NULL;
	if ( ! MessageResolver )
	{
		return E_INVALIDARG;
	}

	*MessageResolver = NULL;

	//
	// Allocate.
	//
	Resolver = CdiagpMalloc( sizeof( CDIAGP_RESOLVER ), TRUE );
	if ( ! Resolver )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize.
	//
	Resolver->ReferenceCount = 1;
	Resolver->Base.Size = sizeof( CDIAGP_RESOLVER );
	
	Resolver->Base.Reference			= CdiagsReferenceResolver;
	Resolver->Base.Dereference			= CdiagsDereferenceResolver;
	Resolver->Base.RegisterMessageDll	= CdiagsRegisterMessageDll;
	Resolver->Base.ResolveMessage		= CdiagsResolveMessage;
	Resolver->Base.UnregisterMessageDll = CdiagsUnregisterMessageDll;

	InitializeCriticalSection( &Resolver->RegisteredDlls.Lock );
	InitializeListHead( &Resolver->RegisteredDlls.ListHead );

	*MessageResolver = &Resolver->Base;

	return S_OK;
}
