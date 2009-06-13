/*----------------------------------------------------------------------
 * Purpose:
 *		Thread local/Test local storage.
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

#include <cfix.h>
#include "cfixp.h"

static DWORD CfixsDefaultTlsSlot = TLS_OUT_OF_INDEXES;
static DWORD CfixsReservedForCcTlsSlot = TLS_OUT_OF_INDEXES;

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */
BOOL CfixpSetupTestTls()
{
	CfixsDefaultTlsSlot	= TlsAlloc();
	if ( CfixsDefaultTlsSlot == TLS_OUT_OF_INDEXES )
	{
		return FALSE;
	}

	CfixsReservedForCcTlsSlot = TlsAlloc();
	if ( CfixsReservedForCcTlsSlot == TLS_OUT_OF_INDEXES )
	{
		TlsFree( CfixsDefaultTlsSlot );
		return FALSE;
	}

	return TRUE;
}

BOOL CfixpTeardownTestTls()
{
	if ( CfixsDefaultTlsSlot != TLS_OUT_OF_INDEXES )
	{
		TlsFree( CfixsDefaultTlsSlot );
	}

	if ( CfixsReservedForCcTlsSlot != TLS_OUT_OF_INDEXES )
	{
		TlsFree( CfixsReservedForCcTlsSlot );
	}

	return TRUE;
}

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */

CFIXAPI VOID CfixPeSetValue(
	__in ULONG Tag,
	__in PVOID Value
	)
{
	if ( Tag == CFIX_TAG_RESERVED_FOR_CC )
	{
		TlsSetValue( CfixsReservedForCcTlsSlot, Value );
	}
	else if ( Tag == 0 )
	{
		TlsSetValue( CfixsDefaultTlsSlot, Value );
	}
	else
	{
		//
		// Invalid parameter.
		//
	}
}

CFIXAPI PVOID CfixPeGetValue(
	__in ULONG Tag
	)
{
	if ( Tag == CFIX_TAG_RESERVED_FOR_CC )
	{
		return TlsGetValue( CfixsReservedForCcTlsSlot );
	}
	else if ( Tag == 0 )
	{
		return TlsGetValue( CfixsDefaultTlsSlot );
	}
	else
	{
		//
		// Invalid parameter.
		//
		return NULL;
	}
}