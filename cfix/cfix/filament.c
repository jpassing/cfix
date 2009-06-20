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

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */
BOOL CfixpSetupFilamentTls()
{
	CfixsTlsSlotForFilament = TlsAlloc();
	return CfixsTlsSlotForFilament != TLS_OUT_OF_INDEXES;
}

BOOL CfixpTeardownFilamentTls()
{
	return TlsFree( CfixsTlsSlotForFilament );
}

/*----------------------------------------------------------------------
 * 
 * Internal.
 *
 */

VOID CfixpInitializeFilament(
	__in PCFIX_EXECUTION_CONTEXT ExecutionContext,
	__in ULONG MainThreadId,
	__out PCFIXP_FILAMENT Filament
	)
{
	Filament->ExecutionContext = ExecutionContext;
	Filament->MainThreadId = MainThreadId;
}

HRESULT CfixpSetCurrentFilament(
	__in PCFIXP_FILAMENT NewFilament,
	__out_opt PCFIXP_FILAMENT *Prev
	)
{
	HRESULT Hr;
	PCFIXP_FILAMENT OldFilament;

	( VOID ) CfixpGetCurrentFilament( &OldFilament );

	if ( OldFilament != NULL )
	{
		OldFilament->ExecutionContext->Dereference( OldFilament->ExecutionContext );
	}

	if ( NewFilament != NULL )
	{
		NewFilament->ExecutionContext->Reference( NewFilament->ExecutionContext );
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
