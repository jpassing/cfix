/*----------------------------------------------------------------------
 * Purpose:
 *		Current execution context management.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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
#include <windows.h>

//
// Slot for holding a PCFIX_EXECUTION_CONTEXT during
// testcase execution.
//
static DWORD CfixsTlsSlotForContext = TLS_OUT_OF_INDEXES;

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */
BOOL CfixpSetupTls()
{
	CfixsTlsSlotForContext = TlsAlloc();
	return CfixsTlsSlotForContext != TLS_OUT_OF_INDEXES;
}

BOOL CfixpTeardownTls()
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
	__out_opt PCFIX_EXECUTION_CONTEXT *PrevContext
	)
{
	if ( PrevContext )
	{
		*PrevContext = ( PCFIX_EXECUTION_CONTEXT ) 
			TlsGetValue( CfixsTlsSlotForContext );
	}

	return TlsSetValue( CfixsTlsSlotForContext, Context )
		? S_OK
		: HRESULT_FROM_WIN32( GetLastError() );
}

HRESULT CfixpGetCurrentExecutionContext(
	__out PCFIX_EXECUTION_CONTEXT *Context 
	)
{
	if ( ! Context )
	{
		return E_INVALIDARG;
	}

	*Context = ( PCFIX_EXECUTION_CONTEXT ) 
		TlsGetValue( CfixsTlsSlotForContext );

	if ( *Context )
	{
		return S_OK;
	}
	else
	{
		return CFIX_E_UNKNOWN_THREAD;
	}
}