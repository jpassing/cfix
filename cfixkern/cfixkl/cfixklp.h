#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Internal header file. 
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

#include <cfixapi.h>
#include <cfixpe.h>
#include <crtdbg.h>
#include <cfixklmsg.h>
#include <cfixkrio.h>

#define ASSERT _ASSERTE

#ifdef DBG
#define VERIFY ASSERT
#else
#define VERIFY( x ) ( ( VOID ) ( x ) )
#endif

#define NOP ( ( VOID ) 0 )

/*++
	Routine Description:	
		Start a driver, installing it first if necessary.

	Parameters:
		DriverPath	- path to binary.
		DriverName  - driver/service name to use.
		DisplyName  - Name to display in GUI.
		Installed	- indicates whether the driver had to be installed
		Loaded		- indicates whether the driver had to be loaded first.
		DriverHandle- SC Handle. Use for CfixklpStopDriverAndCloseHandle.
--*/		
HRESULT CfixklpStartDriver(
	__in PCWSTR DriverPath,
	__in PCWSTR DriverName,
	__in PCWSTR DisplyName,
	__out PBOOL Installed,
	__out PBOOL Loaded,
	__out SC_HANDLE *DriverHandle
	);

/*++
	Routine Description:	
		Stop a driver.
--*/
HRESULT CfixklpStopDriverAndCloseHandle(
	__in SC_HANDLE DriverHandle,
	__in BOOL Uninstall
	);

/*++
	Routine Description:	
		Start cfixkr.
--*/
HRESULT CfixklpStartCfixKernelReflector();

/*----------------------------------------------------------------------
 *
 * Compatibility Routines.
 *
 */
VOID CfixklGetNativeSystemInfo(
	__out LPSYSTEM_INFO SystemInfo
	);

BOOL CfixklIsWow64Process(
	__in HANDLE Process,
	__out PBOOL Wow64Process
	);

/*----------------------------------------------------------------------
 *
 * IOCTL Wrappers.
 *
 */

/*++
	Routine Description:
		Query information about a module.

	Parameters:
		ReflectorHandle		Handle to reflector device.
		DriverPath			Image path of driver to query.
		DriverBaseAddress	Load/Base address of driver.
		Response			Response. Free with 
							CfixklpFreeQueryModuleResponse.
--*/
HRESULT CfixklpQueryModule(
	__in HANDLE ReflectorHandle,
	__in PCWSTR DriverPath,
	__out ULONGLONG *DriverBaseAddress,
	__out PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE *Response
	);

/*++
	Routine Description:
		Free resources allocated by CfixklpQueryModule.
--*/
VOID CfixklpFreeQueryModuleResponse(
	__in PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE Response
	);

/*++
	Routine Description:
		Request reflector to call a specified routine.

	Parameters:
		FixtureKey				Fixture containing routine to run.
		RoutineKey				Routine to run.
		RoutineRanToCompletion	Indicates whether the routine has run in 
								its entirety or has been aborted by an 
								exception.
		AbortRun				See discussion for
								CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE.
		TlsValue				*TlsValue should be 0 for the first call
								of a fixture. For subsequent calls, pass the value
								TlsValue returned by the previous call.
--*/
HRESULT CfixklpCallRoutine(
	__in HANDLE ReflectorHandle,
	__in ULONGLONG DriverBaseAddress,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in USHORT FixtureKey,
	__in USHORT RoutineKey,
	__out PBOOL RoutineRanToCompletion,
	__out PBOOL AbortRun,
	__inout PULONGLONG TlsValue
	);


/*----------------------------------------------------------------------
 *
 * Initialization.
 *
 */

BOOL CfixklpSetupTestTls();
VOID CfixklpTeardownTestTls();