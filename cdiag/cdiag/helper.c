/*----------------------------------------------------------------------
 * Purpose:
 *		Helper routines
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

#include "cdiagp.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

BOOL CdiagpIsWhitespaceOnly(
	__in PCWSTR String
	)
{
	WCHAR ch = UNICODE_NULL;
	BOOL wsOnly = TRUE;

	_ASSERTE( String );
	
	while ( ( ch = *String++ ) != UNICODE_NULL && wsOnly )
	{
		wsOnly &= ( ch == L' ' || 
				    ch == L'\t' ||
				    ch == L'\n' ||
				    ch == L'\r' );
	}
	
	return wsOnly;
}

BOOL CdiagpIsStringValid(
	__in PCWSTR String,
	__in SIZE_T MinLengthInclusive,
	__in SIZE_T MaxLengthInclusive,
	__in BOOL AllowWhitespaceOnly
	)
{
	size_t size = 0;

	_ASSERTE( MinLengthInclusive );
	_ASSERTE( MaxLengthInclusive );
	_ASSERTE( MaxLengthInclusive > MinLengthInclusive );

	return String != NULL &&
		( AllowWhitespaceOnly || ! CdiagpIsWhitespaceOnly( String ) ) &&
		SUCCEEDED( StringCchLength( String, MaxLengthInclusive + 1, &size ) ) &&
		size >= MinLengthInclusive;
}
