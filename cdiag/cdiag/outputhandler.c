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

typedef struct _CDIAGP_OUT_HANDLER
{
	CDIAG_HANDLER Base;

	volatile LONG ReferenceCount;

	PCDIAG_FORMATTER Formatter;

	CDIAG_OUTPUT_ROUTINE OutputRoutine;
} CDIAGP_OUT_HANDLER, *PCDIAGP_OUT_HANDLER;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static HRESULT CdiagsOutHandle(
	__in PCDIAG_HANDLER This,
	__in PCDIAG_EVENT_PACKET Packet
	)
{
	WCHAR Buffer[ 2048 ];
	PCDIAGP_OUT_HANDLER Oh = ( PCDIAGP_OUT_HANDLER ) This;
	HRESULT Hr;

	if ( ! Oh ||
		! CdiagpIsValidHandler( Oh ) )
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


static VOID CdiagsOutDeleteHandler( 
	__in PCDIAGP_OUT_HANDLER Oh 
	)
{
	if ( Oh->Formatter )
	{
		Oh->Formatter->Dereference( Oh->Formatter );
	}

	CdiagpFree( Oh );
}

static HRESULT CdiagsOutSetNextHandler(
	__in PCDIAG_HANDLER This,
	__in PCDIAG_HANDLER Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return CDIAG_E_CHAINING_NOT_SUPPORTED;
}

static HRESULT CdiagsOutGetNextHandler(
	__in PCDIAG_HANDLER This,
	__out_opt PCDIAG_HANDLER *Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return CDIAG_E_CHAINING_NOT_SUPPORTED;
}

static VOID CdiagsOutReferenceHandler(
	__in PCDIAG_HANDLER This
	)
{
	PCDIAGP_OUT_HANDLER Oh = ( PCDIAGP_OUT_HANDLER ) This;
	_ASSERTE( CdiagpIsValidHandler( Oh ) );

	InterlockedIncrement( &Oh->ReferenceCount );
}

static VOID CdiagsOutDereferenceHandler(
	__in PCDIAG_HANDLER This
	)
{
	PCDIAGP_OUT_HANDLER Oh = ( PCDIAGP_OUT_HANDLER ) This;
	_ASSERTE( CdiagpIsValidHandler( Oh ) );

	if ( 0 == InterlockedDecrement( &Oh->ReferenceCount ) )
	{
		CdiagsOutDeleteHandler( Oh );
	}
}

/*----------------------------------------------------------------------
 *
 * Public.
 *
 */

CDIAGAPI HRESULT CDIAGCALLTYPE CdiagCreateOutputHandler(
	__in CDIAG_SESSION_HANDLE Session,
	__in CDIAG_OUTPUT_ROUTINE OutputRoutine,
	__out PCDIAG_HANDLER *Handler
	)
{
	PCDIAGP_OUT_HANDLER Oh = NULL;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! Session || ! OutputRoutine || ! Handler )
	{
		return E_INVALIDARG;
	}

	*Handler = NULL;

	//
	// Allocate.
	//
	Oh = CdiagpMalloc( sizeof( CDIAGP_OUT_HANDLER ), TRUE );
	if ( ! Oh )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize.
	//
	Oh->ReferenceCount = 1;
	Oh->Base.Size = sizeof( CDIAG_HANDLER );
	
	Oh->Base.Reference			= CdiagsOutReferenceHandler;
	Oh->Base.Dereference		= CdiagsOutDereferenceHandler;
	Oh->Base.GetNextHandler		= CdiagsOutGetNextHandler;
	Oh->Base.SetNextHandler		= CdiagsOutSetNextHandler;
	Oh->Base.Handle				= CdiagsOutHandle;

	Oh->OutputRoutine = OutputRoutine;

	//
	// Obtain formatter.
	//
	Hr = CdiagQueryInformationSession(
		Session,
		CdiagSessionFormatter,
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
			CdiagsOutDeleteHandler( Oh );
		}
	}

	return Hr;
}
