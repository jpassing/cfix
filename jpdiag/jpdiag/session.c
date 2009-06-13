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

#include "internal.h"
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

#define SESSION_SIGNATURE 'sseS'

typedef struct _HANDLER_REGISTRATION
{
	union
	{
		JPDIAG_EVENT_TYPE Type;
		JPHT_HASHTABLE_ENTRY HashtableEntry;
	} Key;

	PJPDIAG_HANDLER Handler;

	DWORD SeverityFilter;
} HANDLER_REGISTRATION, *PHANDLER_REGISTRATION;

typedef struct _SESSION
{
	DWORD Signature;

	volatile LONG ReferenceCount;

	struct
	{
		//
		// Lock guarding sub-struct.
		//
		CRITICAL_SECTION Lock;

		PJPDIAG_FORMATTER Formatter;
		PJPDIAG_MESSAGE_RESOLVER Resolver;

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
} SESSION, *PSESSION;

#define JpdiagsIsValidSession( p ) \
	( ( p ) && ( ( PSESSION ) ( p ) )->Signature == SESSION_SIGNATURE )


/*----------------------------------------------------------------------
 * 
 * Hashtable callbacks.
 *
 */
static DWORD JpdiagsHashEventType(
	__in DWORD_PTR Key
	)
{
	return ( DWORD ) Key;
}


static BOOLEAN JpdiagsEqualsEventType(
	__in DWORD_PTR KeyLhs,
	__in DWORD_PTR KeyRhs
	)
{
	return ( BOOLEAN ) ( ( ( DWORD ) KeyLhs ) == ( ( DWORD ) KeyRhs ) );
}

static PVOID JpdiagsAllocateHashtableMemory(
	__in SIZE_T Size 
	)
{
	return JpdiagpMalloc( Size, FALSE );
}

static VOID JpdiagsFreeHashtableMemory(
	__in PVOID Mem
	)
{
	JpdiagpFree( Mem );
}

/*----------------------------------------------------------------------
 *
 * Privates.
 *
 */
static VOID JpdiagsDeleteHandlerRegistration(
	__in PHANDLER_REGISTRATION HandlerReg
	)
{
	_ASSERTE( HandlerReg->Handler );
	if ( HandlerReg->Handler )
	{
		_ASSERTE( JpdiagpIsValidHandler( HandlerReg->Handler ) );
		HandlerReg->Handler->Dereference( HandlerReg->Handler );
	}

	JpdiagpFree( HandlerReg );
}

static VOID JpdiagsTearDownHandlerFromHashtableCallback(
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

	JpdiagsDeleteHandlerRegistration( HandlerReg );
}

static VOID JpdiagsDeleteSession(
	__in PSESSION Session
	)
{
	_ASSERTE( JpdiagsIsValidSession( Session ) );

	//
	// Tear down handlers.
	//
	JphtEnumerateEntries(
		&Session->Members.Handlers,
		JpdiagsTearDownHandlerFromHashtableCallback,
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

	JpdiagpFree( Session );
}

static HRESULT JpdiagsQueryDefaultHandler(
	__in PSESSION Session,
	__out PJPDIAG_HANDLER *Handler 
	)
{
	*Handler = Session->Members.DefaultHandler.Handler;
	if ( *Handler )
	{
		( *Handler )->Reference( *Handler );
	}
	
	return S_OK;
}

static HRESULT JpdiagsSetDefaultHandler(
	__in PSESSION Session,
	__in PJPDIAG_HANDLER Handler 
	)
{
	PJPDIAG_HANDLER OldHandler;
	if ( ! JpdiagpIsValidHandler( Handler ) )
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

static HRESULT JpdiagsQueryHandler(
	__in PSESSION Session,
	__in JPDIAG_EVENT_TYPE Type,
	__out PJPDIAG_HANDLER *Handler 
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

static HRESULT JpdiagsSetHandler(
	__in PSESSION Session,
	__in JPDIAG_EVENT_TYPE Type,
	__in PJPDIAG_HANDLER Handler 
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;

	//
	// Create new registration...
	//
	PHANDLER_REGISTRATION HandlerReg = JpdiagpMalloc( 
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

		JpdiagsDeleteHandlerRegistration( OldReg );
	}

	return S_OK;
}

static HRESULT JpdiagsQuerySeverityFilter(
	__in PSESSION Session,
	__in JPDIAG_EVENT_TYPE Type,
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

static HRESULT JpdiagsSetSeverityFilter(
	__in PSESSION Session,
	__in JPDIAG_EVENT_TYPE Type,
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
HRESULT JPDIAGCALLTYPE JpdiagReferenceSession(
	__in JPDIAG_SESSION_HANDLE SessionHandle
	)
{
	PSESSION Session = ( PSESSION ) SessionHandle;
	if ( ! JpdiagsIsValidSession( Session ) )
	{
		return E_INVALIDARG;
	}

	InterlockedIncrement( &Session->ReferenceCount );
	return S_OK;
}

HRESULT JPDIAGCALLTYPE JpdiagDereferenceSession(
	__in JPDIAG_SESSION_HANDLE SessionHandle
	)
{
	PSESSION Session = ( PSESSION ) SessionHandle;
	if ( ! JpdiagsIsValidSession( Session ) )
	{
		return E_INVALIDARG;
	}

	if ( 0 == InterlockedDecrement( &Session->ReferenceCount ) )
	{
		JpdiagsDeleteSession( Session );
	}

	return S_OK;
}

HRESULT JPDIAGCALLTYPE JpdiagCreateSession(
	__in_opt PJPDIAG_FORMATTER Formatter,
	__in_opt PJPDIAG_MESSAGE_RESOLVER Resolver,
	__out JPDIAG_SESSION_HANDLE *SessionHandle
	)
{
	HRESULT Hr = E_UNEXPECTED;
	PSESSION Session;

	if ( ! SessionHandle )
	{
		return E_INVALIDARG;
	}

	Session = ( PSESSION ) JpdiagpMalloc( sizeof( SESSION ), TRUE );
	if ( ! Session )
	{
		return E_OUTOFMEMORY;
	}

	Session->Signature = SESSION_SIGNATURE;
	Session->ReferenceCount = 1;
	Session->Members.DefaultHandler.SeverityFilter = 0xffffffff;
	InitializeCriticalSection( &Session->Members.Lock );

	if ( ! JphtInitializeHashtable(
		&Session->Members.Handlers,
		JpdiagsAllocateHashtableMemory,
		JpdiagsFreeHashtableMemory,
		JpdiagsHashEventType,
		JpdiagsEqualsEventType,
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
		Hr = JpdiagCreateMessageResolver(
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
		Hr = JpdiagCreateFormatter(
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

		JpdiagpFree( Session );
	}

	return Hr;
}


HRESULT JPDIAGCALLTYPE JpdiagHandleEvent(
	__in JPDIAG_SESSION_HANDLE SessionHandle,
	__in PJPDIAG_EVENT_PACKET Packet
	)
{
	PSESSION Session = ( PSESSION ) SessionHandle;
	PJPHT_HASHTABLE_ENTRY Entry;
	PHANDLER_REGISTRATION HandlerReg;
	PJPDIAG_HANDLER Handler;
	DWORD SeverityFilter;
	HRESULT Hr;

	if ( ! Packet ||
		 ! JpdiagsIsValidSession( Session ) )
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

HRESULT JPDIAGCALLTYPE JpdiagSetInformationSession(
	__in_opt JPDIAG_SESSION_HANDLE SessionHandle,
	__in JPDIAG_SESSION_INFO_CLASS Class,
	__in DWORD EventType,
	__in PVOID Value
	)
{
	PSESSION Session = ( PSESSION ) SessionHandle;
	HRESULT Hr = E_UNEXPECTED;
	PJPDIAG_HANDLER Handler;

	if ( ! JpdiagsIsValidSession( Session ) ||
		 Class > ( DWORD ) JpdiagSessionMaxClass ||
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
	case JpdiagSessionDefaultHandler:
		Handler = ( PJPDIAG_HANDLER ) Value;
		Hr = JpdiagsSetDefaultHandler( Session, Handler );
		break;

	case JpdiagSessionHandler:
		Handler = ( PJPDIAG_HANDLER ) Value;
		Hr = JpdiagsSetHandler( Session, EventType, Handler );
		break;

	case JpdiagSessionSeverityFilter:
		Hr = JpdiagsSetSeverityFilter( Session, EventType, *( ( PDWORD ) Value ) );
		break;

	case JpdiagSessionResolver:
		Session->Members.Resolver->Dereference( Session->Members.Resolver );
		Session->Members.Resolver = ( PJPDIAG_MESSAGE_RESOLVER ) Value;
		Session->Members.Resolver->Reference( Session->Members.Resolver );
		Hr = S_OK;
		break;

	case JpdiagSessionFormatter:
		Session->Members.Formatter->Dereference( Session->Members.Formatter );
		Session->Members.Formatter = ( PJPDIAG_FORMATTER ) Value;
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

HRESULT JPDIAGCALLTYPE JpdiagQueryInformationSession(
	__in_opt JPDIAG_SESSION_HANDLE SessionHandle,
	__in JPDIAG_SESSION_INFO_CLASS Class,
	__in DWORD EventType,
	__out PVOID *Value
	)
{
	PSESSION Session = ( PSESSION ) SessionHandle;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! JpdiagsIsValidSession( Session ) ||
		 Class > ( DWORD ) JpdiagSessionMaxClass ||
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
	case JpdiagSessionDefaultHandler:
		Hr = JpdiagsQueryDefaultHandler( Session, ( PJPDIAG_HANDLER* ) Value );
		break;

	case JpdiagSessionHandler:
		Hr = JpdiagsQueryHandler( Session, EventType, ( PJPDIAG_HANDLER* ) Value );
		break;

	case JpdiagSessionSeverityFilter:
		Hr = JpdiagsQuerySeverityFilter( Session, EventType, ( PDWORD ) Value );
		break;

	case JpdiagSessionResolver:
		Session->Members.Resolver->Reference( Session->Members.Resolver );
		*Value = ( PJPDIAG_MESSAGE_RESOLVER ) Session->Members.Resolver;
		Hr = S_OK;
		break;

	case JpdiagSessionFormatter:
		Session->Members.Formatter->Reference( Session->Members.Formatter );
		*Value = ( PJPDIAG_FORMATTER ) Session->Members.Formatter;
		Hr = S_OK;
		break;

	default:
		Hr = E_INVALIDARG;
		break;
	}

	LeaveCriticalSection( &Session->Members.Lock );

	return Hr;
}
