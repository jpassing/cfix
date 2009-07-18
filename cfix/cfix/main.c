/*----------------------------------------------------------------------
 * Purpose:
 *		DllMain routine
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
#include "cfixp.h"

HMODULE CfixpModule;

BOOL APIENTRY DllMain( 
	__in HMODULE Module,
	__in DWORD Reason,
	__in LPVOID Reserved
)
{
	UNREFERENCED_PARAMETER( Reserved );
	
	if ( Reason ==  DLL_PROCESS_ATTACH )
	{
		CfixpModule = Module;
	
#ifdef DBG	
//		_CrtSetBreakAlloc( 27 );
#endif
		if ( ! CfixpSetupStackTraceCapturing() )
		{
			return FALSE;
		}

		if ( ! CfixpSetupFilamentTls() )
		{
			CfixpTeardownStackTraceCapturing();
			return FALSE;
		}

		return TRUE;
	}
	else if ( Reason ==  DLL_PROCESS_DETACH )
	{
#ifdef DBG	
		_CrtDumpMemoryLeaks();
#endif
		CfixpTeardownStackTraceCapturing();
		return CfixpTeardownFilamentTls();
	}
	else
	{
		return TRUE;
	}
}


