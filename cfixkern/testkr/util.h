#pragma once

/*----------------------------------------------------------------------
 *	Purpose:
 *		Test utility routines.
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

#define TEST CFIX_ASSERT

extern VOID CfixklGetNativeSystemInfo(
	__out LPSYSTEM_INFO SystemInfo
	);

extern BOOL CfixklIsWow64Process(
	__in HANDLE Process,
	__out PBOOL Wow64Process
	);

DWORD LoadDriver(
	__in PCWSTR Name,
	__in PCWSTR DriverPath 
	);

void UnloadDriver(
	__in PCWSTR Name
	);

BOOL IsDriverInstalled(
	__in PCWSTR Name
	);

BOOL IsDriverLoaded(
	__in PCWSTR Name
	);

void UninstallDriver(
	__in PCWSTR Name
	);

void UnloadAllCfixDrivers();

ULONGLONG GetSomeLoadAddress(
	__in HANDLE Dev
	);

DWORD LoadReflector();