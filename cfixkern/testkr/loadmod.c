/*----------------------------------------------------------------------
 * Purpose:
 *		Module Loading.
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
#include <cfix.h>
#include <cfixapi.h>
#include <cfixklmsg.h>
#include "util.h"

#define TEST_SUCCESS( Expr ) CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, ( Expr ) )
#define TEST_HR( Hr, Expr ) CFIX_ASSERT_EQUALS_DWORD( ( ULONG ) ( Hr ), ( Expr ) )
#define TEST				CFIX_ASSERT

#define E_FNF HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND )
#define E_MNF HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND )

typedef enum { 
	Uninstalled, 
	Installed, 
	Loaded 
} DRIVER_STATE;

struct {
	 DRIVER_STATE CfixkrState;
	 DRIVER_STATE TestlibState;
	 HRESULT HrExpected;
} DriverLoads[] =
{
	{ Uninstalled,	Uninstalled,	S_OK },
	{ Uninstalled,	Installed,		S_OK },
	{ Uninstalled,	Loaded,			S_OK },
	{ Installed,	Uninstalled,	S_OK },
	{ Installed,	Installed,		S_OK },
	{ Installed,	Loaded,			S_OK },
	{ Loaded,		Uninstalled,	S_OK },
	{ Loaded,		Installed,		S_OK },
	{ Loaded,		Loaded,			S_OK },
};

/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */


/*----------------------------------------------------------------------
 *
 * Test cases
 *
 */

void LoadNonexistingDriver()
{
	BOOL Installed;
	BOOL Loaded;
	PCFIX_TEST_MODULE TestModule;

	TEST( ! IsDriverLoaded( L"cfixkr" ) );

	//
	// ...with cfixkr unloaded.
	//
	TEST_HR( E_MNF,	CfixklCreateTestModuleFromDriver(
		L"idonotexist.sys",
		&TestModule,
		&Installed,
		&Loaded ) );

	TEST( TestModule == NULL );

	TEST( IsDriverInstalled( L"cfixkr" ) );
	TEST( IsDriverLoaded( L"cfixkr" ) );

	//
	// ...with cfixkr loaded.
	//
	TEST_HR( E_MNF,	CfixklCreateTestModuleFromDriver(
		L"idonotexist.sys",
		&TestModule,
		&Installed,
		&Loaded ) );

	TEST( TestModule == NULL );

	TEST( IsDriverInstalled( L"cfixkr" ) );
	TEST( IsDriverLoaded( L"cfixkr" ) );
	UnloadDriver( L"cfixkr" );
}

void LoadTestDriver()
{
	PWSTR DriverPath;
	BOOL Installed;
	BOOL Loaded;
	PCFIX_TEST_MODULE TestModule1, TestModule2;
	SYSTEM_INFO SystemInfo;
	BOOL WasInstalled;

	CfixklGetNativeSystemInfo( &SystemInfo );
	switch ( SystemInfo.wProcessorArchitecture  )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		DriverPath = L"..\\i386\\testklib1.sys";
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		DriverPath = L"..\\amd64\\testklib1.sys";
		break;

	default:
		CFIX_ASSERT( !"Unsupported processor architecture" );
		return;
	}

	if ( IsDriverLoaded( L"cfixkr" ) )
	{
		UnloadDriver( L"cfixkr" );
	}
	if ( IsDriverLoaded( L"cfixkr_testklib1" ) )
	{
		UnloadDriver( L"cfixkr_testklib1" );
	}

	TEST( ! IsDriverLoaded( L"cfixkr" ) );
	TEST( ! IsDriverLoaded( L"cfixkr_testklib1" ) );

	WasInstalled = IsDriverInstalled( L"cfixkr_testklib1" );

	//
	// ...with both unloaded.
	//
	TEST_SUCCESS( CfixklCreateTestModuleFromDriver(
		DriverPath,
		&TestModule1,
		&Installed,
		&Loaded ) );

	TEST( TestModule1 );
	TEST( Installed != WasInstalled );
	TEST( Loaded );
	if ( ! TestModule1 ) return;

	//
	// This should unload testklib1.
	//
	TestModule1->Routines.Dereference( TestModule1 );
	TestModule1 = NULL;

	//
	// Windows 2000 may require a pause here.
	//
	Sleep( 1000 );

	TEST( IsDriverLoaded( L"cfixkr" ) );
	TEST( !IsDriverLoaded( L"cfixkr_testklib1" ) );

	//
	// Windows 2000 may require a pause here.
	//
	Sleep( 1000 );

	//
	// ...with cfixkr loaded.
	//
	TEST_SUCCESS( CfixklCreateTestModuleFromDriver(
		DriverPath,
		&TestModule1,
		&Installed,
		&Loaded ) );

	TEST( TestModule1 );
	TEST( Installed != WasInstalled );
	TEST( Loaded );
	if ( ! TestModule1 ) return;

	//
	// again.
	//
	TEST_SUCCESS( CfixklCreateTestModuleFromDriver(
		DriverPath,
		&TestModule2,
		&Installed,
		&Loaded ) );

	TEST( TestModule2 );
	TEST( ! Installed );
	TEST( ! Loaded );
	if ( ! TestModule2 ) return;

	TestModule2->Routines.Dereference( TestModule2 );
	TEST( IsDriverLoaded( L"cfixkr_testklib1" ) );

	TestModule1->Routines.Dereference( TestModule1 );
	TEST( ! IsDriverLoaded( L"cfixkr_testklib1" ) );

	TEST( IsDriverLoaded( L"cfixkr" ) );
	TEST( ! IsDriverLoaded( L"cfixkr_testklib1" ) );

	UnloadDriver( L"cfixkr" );
}

void LoadCfixkrTwice()
{
	PWSTR DriverPath;
	HRESULT Hr;
	BOOL Installed;
	BOOL Loaded;
	SYSTEM_INFO SystemInfo;
	PCFIX_TEST_MODULE TestModule;

	if ( IsDriverLoaded( L"cfixkr" ) )
	{
		UnloadDriver( L"cfixkr" );
	}

	CfixklGetNativeSystemInfo( &SystemInfo );
	switch ( SystemInfo.wProcessorArchitecture  )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		DriverPath = L"..\\i386\\cfixkr32.sys";
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		DriverPath = L"..\\amd64\\cfixkr64.sys";
		break;

	default:
		CFIX_ASSERT( !"Unsupported processor architecture" );
		return;
	}

	//
	// cfixkr loaded implicitly and explicitly. Sine cfixkr is not a
	// test driver, this will fail.
	//
	Hr = CfixklCreateTestModuleFromDriver(
		DriverPath,
		&TestModule,
		&Installed,
		&Loaded );
	TEST( CFIXKL_E_UNKNOWN_LOAD_ADDRESS == Hr ||
		  HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) == Hr );

	UnloadDriver( L"cfixkr" );
}

CFIX_BEGIN_FIXTURE( LoadModule )
	CFIX_FIXTURE_SETUP( UnloadAllCfixDrivers )
	CFIX_FIXTURE_ENTRY( LoadNonexistingDriver )
	CFIX_FIXTURE_ENTRY( LoadTestDriver )
	CFIX_FIXTURE_ENTRY( LoadCfixkrTwice )
CFIX_END_FIXTURE()