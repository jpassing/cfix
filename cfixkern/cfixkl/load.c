/*----------------------------------------------------------------------
 *	Purpose:
 *		Driver Loading.
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
#include <stdlib.h>
#include <shlwapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

/*----------------------------------------------------------------------
 *
 * Statics.
 *
 */

static HRESULT CfixklsOpenDriver(
	__in PCWSTR DriverPath,
	__in PCWSTR DriverName,
	__in PCWSTR DisplyName,
	__in PBOOL Installed,
	__out SC_HANDLE *DriverHandle
	)
{
	PWSTR FilePart;
	WCHAR FullPath[ MAX_PATH ];
	HRESULT Hr;
	SC_HANDLE Scm;

	ASSERT( DriverPath );
	ASSERT( DriverName );
	ASSERT( DisplyName );
	ASSERT( Installed );
	ASSERT( DriverHandle );

	*Installed		= FALSE;
	*DriverHandle	= NULL;

	//
	// DriverPath may be relative - convert to full path.
	//
	if ( 0 == GetFullPathName( DriverPath, 
		_countof( FullPath ), 
		FullPath, 
		&FilePart ) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	if ( INVALID_FILE_ATTRIBUTES == GetFileAttributes( FullPath ) )
	{
		return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
	}

	//
	// Connect to SCM.
	//
	Scm = OpenSCManager( 
		NULL, 
		NULL, 
		SC_MANAGER_CREATE_SERVICE );
	if ( Scm == NULL )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	*DriverHandle = CreateService(
		Scm,
		DriverName,
		DisplyName,
		SERVICE_START | SERVICE_STOP | DELETE,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		FullPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL );
	if ( *DriverHandle != NULL )
	{
		*Installed = TRUE;
		Hr = S_OK;
	}
	else
	{
		DWORD Err = GetLastError();
		if ( ERROR_SERVICE_EXISTS == Err )
		{
			//
			// Try to open it.
			//
			*DriverHandle = OpenService( 
				Scm, 
				DriverName, 
				SERVICE_START | SERVICE_STOP | DELETE );
			if ( *DriverHandle != NULL )
			{
				Hr = S_OK;
			}
			else
			{
				Hr = HRESULT_FROM_WIN32( GetLastError() );
			}
		}
		else
		{
			Hr = HRESULT_FROM_WIN32( Err );
		}
	}

	VERIFY( CloseServiceHandle( Scm ) );
	return Hr;
}

static HRESULT CfixklsFindReflectorImage(
	__in SIZE_T PathCch,
	__out_ecount( PathCch ) PWSTR Path
	)
{
	PWSTR ReflectorImageName;
	PWSTR ReflectorImageNameWithDirectory;
	HMODULE OwnModule;
	SYSTEM_INFO SystemInfo;
	
	WCHAR CfixkrModulePath[ MAX_PATH ];
	WCHAR OwnModulePath[ MAX_PATH ];

	//
	// Starting point is the directory this module was loaded from.
	//
	OwnModule = GetModuleHandle( L"cfixkl" );
	ASSERT( OwnModule != NULL );
	if ( 0 == GetModuleFileName(
		OwnModule,
		OwnModulePath,
		_countof( OwnModulePath ) ) )
	{
		return  HRESULT_FROM_WIN32( GetLastError() );
	}

	if ( ! PathRemoveFileSpec( OwnModulePath ) )
	{
		return CFIXKL_E_CFIXKR_NOT_FOUND;
	}

	//
	// Get system information to decide which driver (32/64) we need
	// to load - due to WOW64 the driver image bitness may not be the
	// same as the bitness of this module.
	//
	CfixklGetNativeSystemInfo( &SystemInfo );
	switch ( SystemInfo.wProcessorArchitecture )
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		ReflectorImageName = L"cfixkr64.sys";
		ReflectorImageNameWithDirectory = L"..\\amd64\\cfixkr64.sys";
		break;

	case PROCESSOR_ARCHITECTURE_INTEL:
		ReflectorImageName = L"cfixkr32.sys";
		ReflectorImageNameWithDirectory = L"..\\i386\\cfixkr32.sys";
		break;

	default:
		return E_UNEXPECTED;
	}

	//
	// Try .\cfixkrXX.sys
	//
	if ( ! PathCombine( CfixkrModulePath, OwnModulePath, ReflectorImageName ) )
	{
		return CFIXKL_E_CFIXKR_NOT_FOUND;
	}

	if ( INVALID_FILE_ATTRIBUTES != GetFileAttributes( CfixkrModulePath ) )
	{
		return StringCchCopy(
			Path,
			PathCch,
			CfixkrModulePath );
	}

	//
	// Try ..\<arch>\cfixkrXX.sys.
	//
	if ( ! PathCombine( CfixkrModulePath, OwnModulePath, ReflectorImageNameWithDirectory ) )
	{
		return CFIXKL_E_CFIXKR_NOT_FOUND;
	}

	if ( INVALID_FILE_ATTRIBUTES != GetFileAttributes( CfixkrModulePath ) )
	{
		return StringCchCopy(
			Path,
			PathCch,
			CfixkrModulePath );
	}

	return CFIXKL_E_CFIXKR_NOT_FOUND;
}
/*----------------------------------------------------------------------
 *
 * Internals.
 *
 */
HRESULT CfixklpStartDriver(
	__in PCWSTR DriverPath,
	__in PCWSTR DriverName,
	__in PCWSTR DisplyName,
	__out PBOOL Installed,
	__out PBOOL Loaded,
	__out SC_HANDLE *DriverHandle
	)
{
	HRESULT Hr;

	ASSERT( DriverPath );
	ASSERT( DriverName );
	ASSERT( DisplyName );
	ASSERT( Installed );
	ASSERT( Loaded );
	ASSERT( DriverHandle );

	*Installed	= FALSE;
	*Loaded		= FALSE;

	Hr = CfixklsOpenDriver(
		DriverPath,
		DriverName,
		DisplyName,
		Installed,
		DriverHandle );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}
	
	if ( StartService( *DriverHandle, 0, NULL ) )
	{
		*Loaded = TRUE;
		Hr = S_OK;
	}
	else
	{
		DWORD Err = GetLastError();
		if ( Err == ERROR_SERVICE_ALREADY_RUNNING ||
			 Err == ERROR_FILE_NOT_FOUND )
		{
			//
			// StartService may return ERROR_FILE_NOT_FOUND if 
			// driver already started and in use.
			//

			*Loaded = FALSE;
			Hr = S_OK;
		}
		else 
		{
			if ( Err == ERROR_MORE_DATA )
			{
				Hr = CFIX_E_FIXTURE_NAME_TOO_LONG;
			}
			else
			{
				Hr = HRESULT_FROM_WIN32( Err );
			}
			
			VERIFY( CloseServiceHandle( *DriverHandle ) );
			*DriverHandle = NULL;
		}
	}

	return Hr;
}

HRESULT CfixklpStopDriverAndCloseHandle(
	__in SC_HANDLE DriverHandle,
	__in BOOL Uninstall
	)
{
	HRESULT Hr;
	SERVICE_STATUS Status;

	ASSERT( DriverHandle );

	if ( ControlService( DriverHandle,  SERVICE_CONTROL_STOP, &Status ) )
	{
		Hr = S_OK;
	}
	else
	{
		DWORD Error = GetLastError();
		if ( Error == ERROR_SERVICE_NOT_ACTIVE )
		{
			//
			// Someone else stopped the driver, that is ok.
			//
			Hr = S_OK;
		}
		else
		{
			Hr = HRESULT_FROM_WIN32( Error );
		}
	}

	if ( SUCCEEDED( Hr ) && Uninstall )
	{
		if ( ! DeleteService( DriverHandle ) )
		{
			Hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}

	VERIFY( CloseServiceHandle( DriverHandle ) );
	return Hr;
}

HRESULT CfixklpStartCfixKernelReflector()
{
	SC_HANDLE DriverHandle;
	HRESULT Hr;
	BOOL Installed;
	BOOL Loaded;
	
	WCHAR CfixkrModulePath[ MAX_PATH ];

	Hr = CfixklsFindReflectorImage(
		_countof( CfixkrModulePath ),
		CfixkrModulePath );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	Hr = CfixklpStartDriver(
		CfixkrModulePath,
		L"cfixkr",
		L"Cfix Kernel Reflector",
		&Installed,
		&Loaded,
		&DriverHandle );
	if ( SUCCEEDED( Hr ) )
	{
		VERIFY( CloseServiceHandle( DriverHandle ) );
	}
	else if ( E_ACCESSDENIED == Hr )
	{
		Hr = CFIXKL_E_CFIXKR_START_DENIED;
	}
	else 
	{
		NOP;
	}

	return Hr;
}