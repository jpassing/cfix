/*----------------------------------------------------------------------
 * Purpose:
 *		DllMain routine
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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
#include "internal.h"


BOOL APIENTRY DllMain( 
	__in HMODULE Module,
	__in DWORD Reason,
	__in LPVOID Reserved
)
{
	UNREFERENCED_PARAMETER( Module );
	UNREFERENCED_PARAMETER( Reserved );

	if ( Reason ==  DLL_PROCESS_ATTACH )
	{
#ifdef DBG	
//		_CrtSetBreakAlloc( 27 );
#endif

		return CfixpSetupTls();
	}
	else if ( Reason ==  DLL_PROCESS_DETACH )
	{
#ifdef DBG	
		_CrtDumpMemoryLeaks();
#endif

		return CfixpTeardownTls();;
	}
	else
	{
		return TRUE;
	}
}


