/*----------------------------------------------------------------------
 * Purpose:
 *		PE assessment test.
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
#include <cfix.h>
#include <cfixapi.h>
#include <stdlib.h>
#include <shlwapi.h>
#include "util.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define TEST CFIX_ASSERT
#define TEST_OK( expr ) CFIX_ASSERT_EQUALS_DWORD( S_OK, ( expr ) )

static WCHAR DriverDirectory[ MAX_PATH ];

static void DrvSetup()
{
	SYSTEM_INFO SystemInfo;

	TEST( GetModuleFileName( 
		GetModuleHandle( NULL ), 
		DriverDirectory, 
		_countof( DriverDirectory ) ) );
	TEST( PathRemoveFileSpec( DriverDirectory ) );

	GetNativeSystemInfo( &SystemInfo );
	switch ( SystemInfo.wProcessorArchitecture  )
	{
	case PROCESSOR_ARCHITECTURE_INTEL:
		TEST( PathAppend( DriverDirectory, L"..\\i386" ) );
		break;

	case PROCESSOR_ARCHITECTURE_AMD64:
		TEST( PathAppend( DriverDirectory, L"..\\amd64" ) );
		break;

	default:
		CFIX_ASSERT( !"Unsupported processor architecture" );
		break;
	}
}

static void TestNonExistingDrv()
{
	PCFIX_TEST_MODULE Mod;
	TEST( HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND ) == 
		CfixklCreateTestModuleFromDriver( L"0000.sys", &Mod, NULL, NULL ) );
}

static void TestDrvWithNoTestExports()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( SUCCEEDED( StringCchCopy( Path, _countof( Path ), DriverDirectory ) ) );
	TEST( PathAppend( Path, L"testklib0.sys" ) );

	TEST_OK( CfixklCreateTestModuleFromDriver( Path, &Mod, NULL, NULL ) );

	TEST( 0 == _wcsicmp( Mod->Name,  L"testklib0" ) );
	TEST( 0 == Mod->FixtureCount );

	Mod->Routines.Dereference( Mod );
}

static void TestDrvWithDuplicateSetup()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( SUCCEEDED( StringCchCopy( Path, _countof( Path ), DriverDirectory ) ) );
	TEST( PathAppend( Path, L"testklib4.sys" ) );

	TEST( CFIX_E_DUP_SETUP_ROUTINE == 
		CfixklCreateTestModuleFromDriver( Path, &Mod, NULL, NULL ) );
}

static void TestDrvWithDuplicateTeardown()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( SUCCEEDED( StringCchCopy( Path, _countof( Path ), DriverDirectory ) ) );
	TEST( PathAppend( Path, L"testklib3.sys" ) );

	TEST( CFIX_E_DUP_TEARDOWN_ROUTINE == 
		CfixklCreateTestModuleFromDriver( Path, &Mod, NULL, NULL ) );
}

static void TestDrvWithEmptyFixture()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( SUCCEEDED( StringCchCopy( Path, _countof( Path ), DriverDirectory ) ) );
	TEST( PathAppend( Path, L"testklib2.sys" ) );

	TEST_OK( CfixklCreateTestModuleFromDriver( Path, &Mod, NULL, NULL ) );
	TEST( 0 == _wcsicmp( Mod->Name,  L"testklib2" ) );
	TEST( 1 == Mod->FixtureCount );
	
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->Name, L"X" ) );
	TEST( 0 == Mod->Fixtures[ 0 ]->SetupRoutine );
	TEST( 0 == Mod->Fixtures[ 0 ]->TeardownRoutine );
	TEST( 0 == Mod->Fixtures[ 0 ]->TestCaseCount );
	TEST( Mod->Fixtures[ 0 ]->Module == Mod );

	Mod->Routines.Dereference( Mod );
}

static void TestDrvWithNameTooLong()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( SUCCEEDED( StringCchCopy( Path, _countof( Path ), DriverDirectory ) ) );
	TEST( PathAppend( Path, L"testklib6.sys" ) );

	TEST( CFIX_E_FIXTURE_NAME_TOO_LONG == 
		CfixklCreateTestModuleFromDriver( Path, &Mod, NULL, NULL ) );
}

static void TestDrvWithValidFixture()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( SUCCEEDED( StringCchCopy( Path, _countof( Path ), DriverDirectory ) ) );
	TEST( PathAppend( Path, L"testklib5.sys" ) );

	TEST_OK( CfixklCreateTestModuleFromDriver( Path, &Mod, NULL, NULL ) );
	TEST( 0 == _wcsicmp( Mod->Name,  L"testklib5" ) );
	TEST( 2 == Mod->FixtureCount );
	
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->Name, L"SampleFixture1" ) );
	TEST( Mod->Fixtures[ 0 ]->SetupRoutine );
	TEST( Mod->Fixtures[ 0 ]->TeardownRoutine );
	TEST( 2 == Mod->Fixtures[ 0 ]->TestCaseCount );
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Name, L"Test01" ) );
	TEST( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Fixture == Mod->Fixtures[ 0 ] );
	TEST( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Routine );

	#pragma warning( suppress : 6385 )
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->TestCases[ 1 ].Name, L"Test02" ) );
	TEST( Mod->Fixtures[ 0 ]->TestCases[ 1 ].Routine );

	Mod->Routines.Dereference( Mod );
}

void DrvTeardown()
{
	UnloadDriver( L"cfixkr" );
}

CFIX_BEGIN_FIXTURE( DrvLoading )
	CFIX_FIXTURE_ENTRY( TestNonExistingDrv )
	CFIX_FIXTURE_ENTRY( TestDrvWithNoTestExports )
	CFIX_FIXTURE_ENTRY( TestDrvWithDuplicateSetup )
	CFIX_FIXTURE_ENTRY( TestDrvWithDuplicateTeardown )
	CFIX_FIXTURE_ENTRY( TestDrvWithEmptyFixture )
	CFIX_FIXTURE_ENTRY( TestDrvWithNameTooLong )
	CFIX_FIXTURE_ENTRY( TestDrvWithValidFixture )
	CFIX_FIXTURE_SETUP( DrvSetup )
	CFIX_FIXTURE_TEARDOWN( DrvTeardown )
CFIX_END_FIXTURE()