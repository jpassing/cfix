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
// Slot for holding a CFIXP_CONTEXT_INFO during
// testcase execution.
//
static DWORD CfixsTlsSlotForContext = TLS_OUT_OF_INDEXES;

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */
BOOL CfixpSetupContextTls()
{
	CfixsTlsSlotForContext = TlsAlloc();
	return CfixsTlsSlotForContext != TLS_OUT_OF_INDEXES;
}

BOOL CfixpTeardownContextTls()
{
	return TlsFree( CfixsTlsSlotForContext );
}

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */
HRESULT CfixpSetCurrentExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG MainThreadId,
	__out_opt PCFIX_EXECUTION_CONTEXT *PrevContext
	)
{
	PCFIXP_CONTEXT_INFO Info = ( PCFIXP_CONTEXT_INFO )
		TlsGetValue( CfixsTlsSlotForContext );

	ASSERT( MainThreadId != 0 );

	if ( Context == NULL )
	{
		//
		// Reset.
		//
		if ( Info != NULL )
		{
			Info->ExecutionContext->Dereference( Info->ExecutionContext );
			TlsSetValue( CfixsTlsSlotForContext, NULL );
			free( Info );
		}
	}
	else
	{
		if ( Info == NULL )
		{
			Info = malloc( sizeof( CFIXP_CONTEXT_INFO ) );
			if ( ! Info )
			{
				return E_OUTOFMEMORY;
			}

			Info->ExecutionContext = NULL;

			if ( ! TlsSetValue( CfixsTlsSlotForContext, Info ) )
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}
		}
		else
		{
			Info->ExecutionContext->Dereference( Info->ExecutionContext );
		}

		if ( PrevContext )
		{
			*PrevContext = Info->ExecutionContext;
		}

		Context->Reference( Context );
		Info->ExecutionContext	= Context;
		Info->MainThreadId		= MainThreadId;
	}

	return S_OK;
}

HRESULT CfixpGetCurrentExecutionContext(
	__out PCFIX_EXECUTION_CONTEXT *Context ,
	__out PULONG MainThreadId
	)
{
	PCFIXP_CONTEXT_INFO Info;

	if ( ! Context )
	{
		return E_INVALIDARG;
	}

	Info = ( PCFIXP_CONTEXT_INFO ) TlsGetValue( CfixsTlsSlotForContext );

	if ( Info )
	{
		*Context		= Info->ExecutionContext;
		*MainThreadId	= Info->MainThreadId;

		return S_OK;
	}
	else
	{
		return CFIX_E_UNKNOWN_THREAD;
	}
}
