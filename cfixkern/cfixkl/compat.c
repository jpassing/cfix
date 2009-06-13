/*----------------------------------------------------------------------
 *	Purpose:
 *		Downlevel Windows compatibility.
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
#include "cfixklp.h"

typedef VOID ( * GETNATIVESYSTEMINFO_ROUTINE )(
	__out LPSYSTEM_INFO SystemInfo
	);

typedef BOOL ( * ISWOW64PROCESS_ROUTINE )(
	__in HANDLE hProcess,
	__out PBOOL Wow64Process
	);

VOID CfixklGetNativeSystemInfo(
	__out LPSYSTEM_INFO SystemInfo
	)
{
	HMODULE Kernel32Module;
	GETNATIVESYSTEMINFO_ROUTINE Routine;
	
	Kernel32Module = GetModuleHandle( L"kernel32" );
	ASSERT( Kernel32Module != NULL );

	Routine = ( GETNATIVESYSTEMINFO_ROUTINE ) GetProcAddress( 
		Kernel32Module,
		"GetNativeSystemInfo" );

	if ( Routine != NULL )
	{
		//
		// Windows XP or above.
		//
		( Routine )( SystemInfo );
	}
	else
	{
		//
		// Windows 2000 or below - native system info equals the
		// system info.
		//
		GetSystemInfo( SystemInfo );
	}
}

BOOL CfixklIsWow64Process(
	__in HANDLE Process,
	__out PBOOL Wow64Process
	)
{
	HMODULE Kernel32Module;
	ISWOW64PROCESS_ROUTINE Routine;
	
	Kernel32Module = GetModuleHandle( L"kernel32" );
	ASSERT( Kernel32Module != NULL );

	Routine = ( ISWOW64PROCESS_ROUTINE ) GetProcAddress( 
		Kernel32Module,
		"IsWow64Process" );

	if ( Routine != NULL )
	{
		//
		// Windows XP or above.
		//
		return ( Routine )( Process, Wow64Process );
	}
	else
	{
		//
		// Windows 2000 or below - cannot be WOW64.
		//
		*Wow64Process = FALSE;
		return TRUE;
	}
}