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

#include "cdiagp.h"
#include "list.h"
#include <hashtable.h>

#define DEFAULT_FORMAT					\
		L"Type=%Type, "					\
		L"Flags=%Flags, "				\
		L"Severity=%Severity, "			\
		L"Mode=%ProcessorMode, "		\
		L"Machine=%Machine, "			\
		L"ProcessId=%ProcessId, "		\
		L"ThreadId=%ThreadId, "			\
		L"Code=%Code, "					\
		L"Module=%Module, "				\
		L"Function=%Function, "			\
		L"File=%File, "					\
		L"Line=%Line, "					\
		L"Message=%Message"

#define CDIAGP_SESSION_SIGNATURE 'sseS'

typedef struct _HANDLER_REGISTRATION
{
	union
	{
		CDIAG_EVENT_TYPE Type;
		JPHT_HASHTABLE_ENTRY HashtableEntry;
	} Key;

	PCDIAG_HANDLER Handler;

	DWORD SeverityFilter;
} HANDLER_REGISTRATION, *PHANDLER_REGISTRATION;

typedef struct _CDIAGP_SESSION
{
	DWORD Signature;

	volatile LONG ReferenceCount;

	struct
	{
		//
		// Lock guarding sub-struct.
		//
		CRITICAL_SECTION Lock;

		PCDIAG_FORMATTER Formatter;
		PCDIAG_MESSAGE_RESOLVER Resolver;

		//
		// Default handler used when no entry in hashtable.
		// DefaultHandler->Handler May be NULL.
		//
		HANDLER_REGISTRATION DefaultHandler;

		//
		// Hashtable: EventType->HANDLER_REGISTRATION
		//
		JPHT_HASHTABLE Handlers;
	} Members;
} CDIAGP_SESSION, *PCDIAGP_SESSION;

#define CdiagsIsValidSession( p ) \
	( ( p ) && ( ( PCDIAGP_SESSION ) ( p ) )->Signature == CDIAGP_SESSION_SIGNATURE )


/*----------------------------------------------------------------------
 * 
 * Hashtable callbacks.
 *
 */
static DWORD CdiagsHashEventType(
	__in DWORD_PTR Key
	)
{
	return ( DWORD ) Key;
}


static BOOLEAN CdiagsEqualsEventType(
	__in DWORD_PTR KeyLhs,
	__in DWORD_PTR KeyRhs
	)
{
	return ( BOOLEAN ) ( ( ( DWORD ) KeyLhs ) == ( ( DWORD ) KeyRhs ) );
}

static PVOID CdiagsAllocateHashtableMemory(
	__in SIZE_T Size 
	)
{
	return CdiagpMalloc( Size, FALSE );
}

static VOID CdiagsFreeHashtableMemory(
	__in PVOID Mem
	)
{
	CdiagpFree( Mem );
}

/*----------------------------------------------------------------------
 *
 * Privates.
 *
 */
static VOID CdiagsDeleteHandlerRegistration(
	__in PHANDLER_REGISTRATION HandlerReg
	)
{
	_ASSERTE( HandlerReg->Handler );
	if ( HandlerReg->Handler )
	{
		_ASSERTE( CdiagpIsValidHandler( HandlerReg->Handler ) );
		HandlerReg->Handler->Dereference( HandlerReg->Handler );
	}

	CdiagpFree( HandlerReg );
}

static VOID CdiagsTearDownHandlerFromHashtableCallback(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID Context
	)
{
	PHANDLER_REGISTRATION HandlerReg;
	PJPHT_HASHTABLE_ENTRY OldEntry;
	
	UNREFERENCED_PARAMETER( Context );
	JphtRemoveEntryHashtable(
		Hashtable,
		Entry->Key,
		&OldEntry );

	_ASSERTE( OldEntry == Entry );

	HandlerReg = CONTAINING_RECORD(
		Entry,
		HANDLER_REGISTRATION,
		Key.HashtableEntry );

	CdiagsDeleteHandlerRegistration( HandlerReg );
}

static VOID CdiagsDeleteSession(
	__in PCDIAGP_SESSION Session
	)
{
	_ASSERTE( CdiagsIsValidSession( Session ) );

	//
	// Tear down handlers.
	//
	JphtEnumerateEntries(
		&Session->Members.Handlers,
		CdiagsTearDownHandlerFromHashtableCallback,
		NULL );
	
	JphtDeleteHashtable( &Session->Members.Handlers );

	if ( Session->Members.DefaultHandler.Handler )
	{
		Session->Members.DefaultHandler.Handler->Dereference( 
			Session->Members.DefaultHandler.Handler );
	}

	DeleteCriticalSection( &Session->Members.Lock );
	if ( Session->Members.Formatter )
	{
		Session->Members.Formatter->Dereference( Session->Members.Formatter );
	}

	if ( Session->Members.Resolver )
	{
		Session->Members.Resolver->Dereference( Session->Members.Resolver );
	}

	CdiagpFree( Session );
}

static HRESULT CdiagsQueryDefaultHandler(
	__in PCDIAGP_SESSION Session,
	__out PCDIAG_HANDLER *Handler 
	)
{
	*Handler = Session->Members.DefaultHandler.Handler;
	if ( *Handler )
	{
		( *Handler )->Reference( *Handler );
	}
	
	return S_OK;
}

static HRESULT CdiagsSetDefaultHandler(
	__in PCDIAGP_SESSION Session,
	__in PCDIAG_HANDLER Handler 
	)
{
	PCDIAG_HANDLER OldHandler;
	if ( ! CdiagpIsValidHandler( Handler ) )
	{
		return E_INVALIDARG;
	}

	//
	// Remove old handler.
	//
	OldHandler = Session->Members.DefaultHandler.Handler;
	if ( OldHandler )
	{
		OldHandler->Dereference( OldHandler );
	}

	//
	// Set new.
	//
	Handler->Reference( Handler );
	Session->Members.DefaultHandler.Handler = Handler;
	return S_OK;
}

static HRESULT CdiagsQueryHandler(
	__in PCDIAGP_SESSION Session,
	__in CDIAG_EVENT_TYPE Type,
	__out PCDIAG_HANDLER *Handler 
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	PHANDLER_REGISTRATION HandlerReg;
	Entry = JphtGetEntryHashtable( &Session->Members.Handlers, Type );
	if ( ! Entry )
	{	
		*Handler = NULL;
		return S_FALSE;
	}
	else
	{
		HandlerReg = CONTAINING_RECORD(
			Entry,
			HANDLER_REGISTRATION,
			Key.HashtableEntry );

		*Handler = HandlerReg->Handler;
		( *Handler )->Reference( *Handler );
		return S_OK;
	}
}

static HRESULT CdiagsSetHandler(
	__in PCDIAGP_SESSION Session,
	__in CDIAG_EVENT_TYPE Type,
	__in PCDIAG_HANDLER Handler 
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;

	//
	// Create new registration...
	//
	PHANDLER_REGISTRATION HandlerReg = CdiagpMalloc( 
		sizeof( HANDLER_REGISTRATION ), TRUE );

	HandlerReg->Key.Type = Type;
	HandlerReg->SeverityFilter = 0xffffffff;
	HandlerReg->Handler = Handler;
	Handler->Reference( Handler );

	//
	// Register it...
	//
	JphtPutEntryHashtable(
		&Session->Members.Handlers,
		&HandlerReg->Key.HashtableEntry,
		&OldEntry );

	//
	// Dispose old.
	//
	if ( OldEntry )
	{
		PHANDLER_REGISTRATION OldReg = CONTAINING_RECORD(
			OldEntry,
			HANDLER_REGISTRATION,
			Key.HashtableEntry );

		CdiagsDeleteHandlerRegistration( OldReg );
	}

	return S_OK;
}

static HRESULT CdiagsQuerySeverityFilter(
	__in PCDIAGP_SESSION Session,
	__in CDIAG_EVENT_TYPE Type,
	__out PDWORD Filter
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	PHANDLER_REGISTRATION HandlerReg;
	Entry = JphtGetEntryHashtable( &Session->Members.Handlers, Type );
	if ( ! Entry )
	{	
		*Filter = 0;
		return S_FALSE;
	}
	else
	{
		HandlerReg = CONTAINING_RECORD(
			Entry,
			HANDLER_REGISTRATION,
			Key.HashtableEntry );

		*Filter = HandlerReg->SeverityFilter;
		return S_OK;
	}
}

static HRESULT CdiagsSetSeverityFilter(
	__in PCDIAGP_SESSION Session,
	__in CDIAG_EVENT_TYPE Type,
	__in DWORD Filter
	)
{
	PJPHT_HASHTABLE_ENTRY Entry;
	PHANDLER_REGISTRATION HandlerReg;
	Entry = JphtGetEntryHashtable( &Session->Members.Handlers, Type );
	if ( ! Entry )
	{	
		return S_FALSE;
	}
	else
	{
		HandlerReg = CONTAINING_RECORD(
			Entry,
			HANDLER_REGISTRATION,
			Key.HashtableEntry );

		HandlerReg->SeverityFilter = Filter;
		return S_OK;
	}
}
/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */
HRESULT CDIAGCALLTYPE CdiagReferenceSession(
	__in CDIAG_SESSION_HANDLE SessionHandle
	)
{
	PCDIAGP_SESSION Session = ( PCDIAGP_SESSION ) SessionHandle;
	if ( ! CdiagsIsValidSession( Session ) )
	{
		return E_INVALIDARG;
	}

	InterlockedIncrement( &Session->ReferenceCount );
	return S_OK;
}

HRESULT CDIAGCALLTYPE CdiagDereferenceSession(
	__in CDIAG_SESSION_HANDLE SessionHandle
	)
{
	PCDIAGP_SESSION Session = ( PCDIAGP_SESSION ) SessionHandle;
	if ( ! CdiagsIsValidSession( Session ) )
	{
		return E_INVALIDARG;
	}

	if ( 0 == InterlockedDecrement( &Session->ReferenceCount ) )
	{
		CdiagsDeleteSession( Session );
	}

	return S_OK;
}

HRESULT CDIAGCALLTYPE CdiagCreateSession(
	__in_opt PCDIAG_FORMATTER Formatter,
	__in_opt PCDIAG_MESSAGE_RESOLVER Resolver,
	__out CDIAG_SESSION_HANDLE *SessionHandle
	)
{
	HRESULT Hr = E_UNEXPECTED;
	PCDIAGP_SESSION Session;

	if ( ! SessionHandle )
	{
		return E_INVALIDARG;
	}

	Session = ( PCDIAGP_SESSION ) CdiagpMalloc( sizeof( CDIAGP_SESSION ), TRUE );
	if ( ! Session )
	{
		return E_OUTOFMEMORY;
	}

	Session->Signature = CDIAGP_SESSION_SIGNATURE;
	Session->ReferenceCount = 1;
	Session->Members.DefaultHandler.SeverityFilter = 0xffffffff;
	InitializeCriticalSection( &Session->Members.Lock );

	if ( ! JphtInitializeHashtable(
		&Session->Members.Handlers,
		CdiagsAllocateHashtableMemory,
		CdiagsFreeHashtableMemory,
		CdiagsHashEventType,
		CdiagsEqualsEventType,
		23 ) )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	if ( Resolver )
	{
		Resolver->Reference( Resolver );
		Session->Members.Resolver = Resolver;
	}
	else
	{
		//
		// Create default.
		//
		Hr = CdiagCreateMessageResolver(
			&Session->Members.Resolver );
		if ( FAILED( Hr ) )
		{
			goto Cleanup;
		}
	}

	if ( Formatter )
	{
		Formatter->Reference( Formatter );
		Session->Members.Formatter = Formatter;
	}
	else
	{
		//
		// Create default.
		//
		Hr = CdiagCreateFormatter(
			DEFAULT_FORMAT,
			Session->Members.Resolver,
			0,
			&Session->Members.Formatter );
		if ( FAILED( Hr ) )
		{
			goto Cleanup;
		}
	}

	Hr = S_OK;
	*SessionHandle = Session;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( Session->Members.Formatter )
		{
			Session->Members.Formatter->Dereference( Session->Members.Formatter );
		}

		if ( Session->Members.Resolver )
		{
			Session->Members.Resolver->Dereference( Session->Members.Resolver );
		}

		CdiagpFree( Session );
	}

	return Hr;
}


HRESULT CDIAGCALLTYPE CdiagHandleEvent(
	__in CDIAG_SESSION_HANDLE SessionHandle,
	__in PCDIAG_EVENT_PACKET Packet
	)
{
	PCDIAGP_SESSION Session = ( PCDIAGP_SESSION ) SessionHandle;
	PJPHT_HASHTABLE_ENTRY Entry;
	PHANDLER_REGISTRATION HandlerReg;
	PCDIAG_HANDLER Handler;
	DWORD SeverityFilter;
	HRESULT Hr;

	if ( ! Packet ||
		 ! CdiagsIsValidSession( Session ) )
	{
		return E_INVALIDARG;
	}

	//
	// Grab handler registration.
	//
	EnterCriticalSection( &Session->Members.Lock );

	Entry = JphtGetEntryHashtable( 
		&Session->Members.Handlers, 
		Packet->Type );
	if ( Entry )
	{
		HandlerReg = CONTAINING_RECORD(
			Entry,
			HANDLER_REGISTRATION,
			Key.HashtableEntry );
	}
	else
	{
		//
		// Use default.
		//
		HandlerReg = &Session->Members.DefaultHandler;
	}

	Handler = HandlerReg->Handler;
	SeverityFilter =  HandlerReg->SeverityFilter;

	//
	// In order to leave the critsec early, we need to add-ref the
	// handler.
	//
	if ( Handler )
	{
		Handler->Reference( Handler );
	}

	LeaveCriticalSection( &Session->Members.Lock );

	if ( Handler )
	{
		//
		// Filter & Handle.
		//
		if ( 0 != ( ( 1 << Packet->Type ) & SeverityFilter ) )
		{
			Hr = Handler->Handle( Handler, Packet );
		}
		else
		{
			Hr = S_FALSE;
		}

		Handler->Dereference( Handler );
	}
	else
	{
		Hr = S_FALSE;
	}

	return Hr;
}

HRESULT CDIAGCALLTYPE CdiagSetInformationSession(
	__in_opt CDIAG_SESSION_HANDLE SessionHandle,
	__in CDIAG_SESSION_INFO_CLASS Class,
	__in DWORD EventType,
	__in PVOID Value
	)
{
	PCDIAGP_SESSION Session = ( PCDIAGP_SESSION ) SessionHandle;
	HRESULT Hr = E_UNEXPECTED;
	PCDIAG_HANDLER Handler;

	if ( ! CdiagsIsValidSession( Session ) ||
		 Class > ( DWORD ) CdiagSessionMaxClass ||
		 ! Value )
	{
		return E_INVALIDARG;
	}

	//
	// Protect against concurrent modifications.
	//
	EnterCriticalSection( &Session->Members.Lock );

	switch ( Class )
	{
	case CdiagSessionDefaultHandler:
		Handler = ( PCDIAG_HANDLER ) Value;
		Hr = CdiagsSetDefaultHandler( Session, Handler );
		break;

	case CdiagSessionHandler:
		Handler = ( PCDIAG_HANDLER ) Value;
		Hr = CdiagsSetHandler( Session, EventType, Handler );
		break;

	case CdiagSessionSeverityFilter:
		Hr = CdiagsSetSeverityFilter( Session, EventType, *( ( PDWORD ) Value ) );
		break;

	case CdiagSessionResolver:
		Session->Members.Resolver->Dereference( Session->Members.Resolver );
		Session->Members.Resolver = ( PCDIAG_MESSAGE_RESOLVER ) Value;
		Session->Members.Resolver->Reference( Session->Members.Resolver );
		Hr = S_OK;
		break;

	case CdiagSessionFormatter:
		Session->Members.Formatter->Dereference( Session->Members.Formatter );
		Session->Members.Formatter = ( PCDIAG_FORMATTER ) Value;
		Session->Members.Formatter->Reference( Session->Members.Formatter );
		Hr = S_OK;
		break;

	default:
		Hr = E_INVALIDARG;
		break;
	}

	LeaveCriticalSection( &Session->Members.Lock );

	return Hr;
}

HRESULT CDIAGCALLTYPE CdiagQueryInformationSession(
	__in_opt CDIAG_SESSION_HANDLE SessionHandle,
	__in CDIAG_SESSION_INFO_CLASS Class,
	__in DWORD EventType,
	__out PVOID *Value
	)
{
	PCDIAGP_SESSION Session = ( PCDIAGP_SESSION ) SessionHandle;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! CdiagsIsValidSession( Session ) ||
		 Class > ( DWORD ) CdiagSessionMaxClass ||
		 ! Value )
	{
		return E_INVALIDARG;
	}

	//
	// Protect against concurrent modifications.
	//
	EnterCriticalSection( &Session->Members.Lock );

	switch ( Class )
	{
	case CdiagSessionDefaultHandler:
		Hr = CdiagsQueryDefaultHandler( Session, ( PCDIAG_HANDLER* ) Value );
		break;

	case CdiagSessionHandler:
		Hr = CdiagsQueryHandler( Session, EventType, ( PCDIAG_HANDLER* ) Value );
		break;

	case CdiagSessionSeverityFilter:
		Hr = CdiagsQuerySeverityFilter( Session, EventType, ( PDWORD ) Value );
		break;

	case CdiagSessionResolver:
		Session->Members.Resolver->Reference( Session->Members.Resolver );
		*Value = ( PCDIAG_MESSAGE_RESOLVER ) Session->Members.Resolver;
		Hr = S_OK;
		break;

	case CdiagSessionFormatter:
		Session->Members.Formatter->Reference( Session->Members.Formatter );
		*Value = ( PCDIAG_FORMATTER ) Session->Members.Formatter;
		Hr = S_OK;
		break;

	default:
		Hr = E_INVALIDARG;
		break;
	}

	LeaveCriticalSection( &Session->Members.Lock );

	return Hr;
}
