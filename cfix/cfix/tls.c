/*----------------------------------------------------------------------
 * Purpose:
 *		Thread local/Test local storage.
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

#include <cfix.h>
#include "cfixp.h"

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */

CFIXAPI VOID CfixPeSetValue(
	__in ULONG Tag,
	__in_opt PVOID Value
	)
{
	PCFIXP_FILAMENT Filament;
	HRESULT Hr;
	
	Hr = CfixpGetCurrentFilament( &Filament, NULL );
	if ( SUCCEEDED( Hr ) )
	{
		if ( Tag == CFIX_TAG_RESERVED_FOR_CC )
		{
			Filament->Storage.CcSlot = Value;
		}
		else if ( Tag == 0 )
		{
			Filament->Storage.DefaultSlot = Value;
		}
		else
		{
			//
			// Invalid parameter.
			//
		}
	}
	else
	{
		ASSERT( !"No current filament available" );
	}
}

CFIXAPI PVOID CfixPeGetValue(
	__in ULONG Tag
	)
{
	PCFIXP_FILAMENT Filament;
	HRESULT Hr;
	
	Hr = CfixpGetCurrentFilament( &Filament, NULL );
	if ( SUCCEEDED( Hr ) )
	{
		if ( Tag == CFIX_TAG_RESERVED_FOR_CC )
		{
			return Filament->Storage.CcSlot;
		}
		else if ( Tag == 0 )
		{
			return Filament->Storage.DefaultSlot;
		}
	}
	else
	{
		ASSERT( !"No current filament available" );
	}

	//
	// Invalid parameter.
	//
	return NULL;
}