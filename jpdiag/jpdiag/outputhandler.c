/*----------------------------------------------------------------------
 * Purpose:
 *		Handler that passes formatted ouput to a callback.
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

typedef struct _OUT_HANDLER
{
	JPDIAG_HANDLER Base;

	volatile LONG ReferenceCount;

	PJPDIAG_FORMATTER Formatter;

	JPDIAG_OUTPUT_ROUTINE OutputRoutine;
} OUT_HANDLER, *POUT_HANDLER;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static HRESULT JpdiagsOutHandle(
	__in PJPDIAG_HANDLER This,
	__in PJPDIAG_EVENT_PACKET Packet
	)
{
	WCHAR Buffer[ 1024 ];
	POUT_HANDLER Oh = ( POUT_HANDLER ) This;
	HRESULT Hr;

	if ( ! Oh ||
		! JpdiagpIsValidHandler( Oh ) )
	{
		return E_INVALIDARG;
	}
	
	Hr = Oh->Formatter->Format(
		Oh->Formatter,
		Packet,
		_countof( Buffer ),
		Buffer );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	//
	// Output.
	//
	( Oh->OutputRoutine ) ( Buffer );
	return S_OK;
}


static VOID JpdiagsOutDeleteHandler( 
	__in POUT_HANDLER Oh 
	)
{
	if ( Oh->Formatter )
	{
		Oh->Formatter->Dereference( Oh->Formatter );
	}

	JpdiagpFree( Oh );
}

static HRESULT JpdiagsOutSetNextHandler(
	__in PJPDIAG_HANDLER This,
	__in PJPDIAG_HANDLER Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return JPDIAG_E_CHAINING_NOT_SUPPORTED;
}

static HRESULT JpdiagsOutGetNextHandler(
	__in PJPDIAG_HANDLER This,
	__out_opt PJPDIAG_HANDLER *Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return JPDIAG_E_CHAINING_NOT_SUPPORTED;
}

static VOID JpdiagsOutReferenceHandler(
	__in PJPDIAG_HANDLER This
	)
{
	POUT_HANDLER Oh = ( POUT_HANDLER ) This;
	_ASSERTE( JpdiagpIsValidHandler( Oh ) );

	InterlockedIncrement( &Oh->ReferenceCount );
}

static VOID JpdiagsOutDereferenceHandler(
	__in PJPDIAG_HANDLER This
	)
{
	POUT_HANDLER Oh = ( POUT_HANDLER ) This;
	_ASSERTE( JpdiagpIsValidHandler( Oh ) );

	if ( 0 == InterlockedDecrement( &Oh->ReferenceCount ) )
	{
		JpdiagsOutDeleteHandler( Oh );
	}
}

/*----------------------------------------------------------------------
 *
 * Public.
 *
 */

JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateOutputHandler(
	__in JPDIAG_SESSION_HANDLE Session,
	__in JPDIAG_OUTPUT_ROUTINE OutputRoutine,
	__out PJPDIAG_HANDLER *Handler
	)
{
	POUT_HANDLER Oh = NULL;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! Session || ! OutputRoutine || ! Handler )
	{
		return E_INVALIDARG;
	}

	*Handler = NULL;

	//
	// Allocate.
	//
	Oh = JpdiagpMalloc( sizeof( OUT_HANDLER ), TRUE );
	if ( ! Oh )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize.
	//
	Oh->ReferenceCount = 1;
	Oh->Base.Size = sizeof( JPDIAG_HANDLER );
	
	Oh->Base.Reference			= JpdiagsOutReferenceHandler;
	Oh->Base.Dereference		= JpdiagsOutDereferenceHandler;
	Oh->Base.GetNextHandler		= JpdiagsOutGetNextHandler;
	Oh->Base.SetNextHandler		= JpdiagsOutSetNextHandler;
	Oh->Base.Handle				= JpdiagsOutHandle;

	Oh->OutputRoutine = OutputRoutine;

	//
	// Obtain formatter.
	//
	Hr = JpdiagQueryInformationSession(
		Session,
		JpdiagSessionFormatter,
		0,
		&Oh->Formatter );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	*Handler = &Oh->Base;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( Oh )
		{
			JpdiagsOutDeleteHandler( Oh );
		}
	}

	return Hr;
}
