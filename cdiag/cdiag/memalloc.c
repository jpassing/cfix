/*----------------------------------------------------------------------
 * Purpose:
 *		Memory allocation wrapper routines
 *
 * Copyright:
 *		2007-2009 Johannes Passing (passing at users.sourceforge.net)
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

#include <stdlib.h>
#include "cdiagp.h"

#ifdef _DEBUG

//
// In debug builds use CRT allocator s.t. we can track leaks.
//

PVOID CdiagpMalloc( 
	__in SIZE_T Size,
	__in BOOL Zero
	)
{
	PVOID Mem = malloc( Size );
	if ( Mem && Zero )
	{
		ZeroMemory( Mem, Size );
	}

	return Mem;
}

VOID CdiagpFree( 
	__in PVOID Ptr
	)
{
	free( Ptr );
}

#else

PVOID CdiagpMalloc( 
	__in SIZE_T Size,
	__in BOOL Zero
	)
{
	return HeapAlloc( 
		GetProcessHeap(), 
		Zero ? HEAP_ZERO_MEMORY : 0, 
		Size );
}

VOID CdiagpFree( 
	__in PVOID Ptr
	)
{
	HeapFree( GetProcessHeap(), 0, Ptr );
}
#endif