/*----------------------------------------------------------------------
 *	Purpose:
 *		Test utility routines.
 *
 *	Copyright:
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
#include <cfix.h>
#include <util.h>
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

DWORD LoadDriver(
	__in PCWSTR Name,
	__in PCWSTR DriverFileName 
	)
{
	SC_HANDLE ScMgr, Service;
	WCHAR DriverPath[ MAX_PATH ];
	WCHAR FullPath[ MAX_PATH ];
	PWSTR FilePart;
	DWORD RetVal;
	SYSTEM_INFO SystemInfo;

	//
	// Prepend architecture-specific directory.
	//
	GetNativeSystemInfo( &SystemInfo );
	switch ( SystemInfo.wProcessorArchitecture  )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		TEST( SUCCEEDED( StringCchPrintf(
			DriverPath,
			_countof( DriverPath ),
			L"..\\i386\\%s",
			DriverFileName ) ) );
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		TEST( SUCCEEDED( StringCchPrintf(
			DriverPath,
			_countof( DriverPath ),
			L"..\\amd64\\%s",
			DriverFileName ) ) );
		break;

	default:
		CFIX_ASSERT( !"Unsupported processor architecture" );
		return ERROR_FUNCTION_FAILED;
	}

	ScMgr = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS  );
	TEST( ScMgr != NULL );

	TEST( GetFullPathName( DriverPath, _countof( FullPath ), FullPath, &FilePart ) );

	// CFIX_LOG( L"Attempting to load %s", FullPath );

	Service = CreateService(
		ScMgr,
		Name,
		Name,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		FullPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL );
	TEST( Service || GetLastError() == ERROR_SERVICE_EXISTS );

	if ( ERROR_SUCCESS == GetLastError() )
	{
		// Fine.
	}
	else if ( GetLastError() == ERROR_SERVICE_EXISTS )
	{
		Service = OpenService( ScMgr, Name, SERVICE_ALL_ACCESS );
		TEST( Service ); 
	}
	else
	{
		TEST( !"Unexpected SCM failure" );
	}

	if ( ! StartService( Service, 0, NULL ) )
	{
		RetVal = GetLastError();
	}
	else
	{
		RetVal = ERROR_SUCCESS;
	}

	TEST( CloseServiceHandle( ScMgr ) );
	TEST( CloseServiceHandle( Service ) );

	return RetVal;
}

void UnloadDriver(
	__in PCWSTR Name
	)
{
	SC_HANDLE ScMgr, Service;
	SERVICE_STATUS Status;

	ScMgr = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	TEST( ScMgr );

	Service = OpenService( ScMgr, Name, SERVICE_ALL_ACCESS );
	TEST( Service );

	TEST( ControlService( Service, SERVICE_CONTROL_STOP, &Status ) );

	TEST( CloseServiceHandle( ScMgr ) );
	TEST( CloseServiceHandle( Service ) );
}

BOOL IsDriverInstalled(
	__in PCWSTR Name
	)
{
	BOOL Installed;
	SC_HANDLE ScMgr, Service;

	ScMgr = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	TEST( ScMgr );

	Service = OpenService( ScMgr, Name, SERVICE_ALL_ACCESS );
	Installed = ( Service != NULL );
	
	TEST( CloseServiceHandle( ScMgr ) );

	if ( Service != NULL )
	{
		TEST( CloseServiceHandle( Service ) );
	}

	return Installed;
}

BOOL IsDriverLoaded(
	__in PCWSTR Name
	)
{
	BOOL Loaded;
	SC_HANDLE ScMgr, Service;
	SERVICE_STATUS Status;

	ScMgr = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	TEST( ScMgr );

	Service = OpenService( ScMgr, Name, SERVICE_ALL_ACCESS );
	TEST( Service != NULL || GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST );
	if ( Service != NULL )
	{
		TEST( QueryServiceStatus( Service, &Status ) );

		Loaded = ( Status.dwCurrentState == SERVICE_RUNNING );
		TEST( CloseServiceHandle( Service ) );
	}
	else
	{
		Loaded = FALSE;
	}
	TEST( CloseServiceHandle( ScMgr ) );

	return Loaded;
}

//void UninstallDriver(
//	__in PCWSTR Name
//	)
//{
//	SC_HANDLE ScMgr, Service;
//
//	ScMgr = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
//	TEST( ScMgr );
//
//	Service = OpenService( ScMgr, Name, SERVICE_ALL_ACCESS );
//	TEST( Service );
//
//	TEST( DeleteService( Service ) );
//
//	TEST( CloseServiceHandle( ScMgr ) );
//	TEST( CloseServiceHandle( Service ) );
//}

void UnloadAllCfixDrivers()
{
	if ( IsDriverLoaded( L"cfixkr" ) )
	{
		UnloadDriver( L"cfixkr" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib0" ) )
	{
		UnloadDriver( L"cfixkr_testklib0" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib1" ) )
	{
		UnloadDriver( L"cfixkr_testklib1" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib2" ) )
	{
		UnloadDriver( L"cfixkr_testklib2" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib3" ) )
	{
		UnloadDriver( L"cfixkr_testklib3" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib4" ) )
	{
		UnloadDriver( L"cfixkr_testklib4" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib5" ) )
	{
		UnloadDriver( L"cfixkr_testklib5" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib6" ) )
	{
		UnloadDriver( L"cfixkr_testklib6" );
	}
}

DWORD LoadReflector()
{
	SYSTEM_INFO SystemInfo;

	GetNativeSystemInfo( &SystemInfo );
	switch ( SystemInfo.wProcessorArchitecture  )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		return LoadDriver( L"cfixkr", L"cfixkr32.sys" );

	case PROCESSOR_ARCHITECTURE_AMD64:
		return LoadDriver( L"cfixkr", L"cfixkr64.sys" );

	default:
		CFIX_ASSERT( !"Unsupported processor architecture" );
		return ERROR_FUNCTION_FAILED;
	}
}