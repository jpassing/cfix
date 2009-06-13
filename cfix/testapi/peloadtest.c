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
#include "test.h"

static void TestNonExistingDll()
{
	PCFIX_TEST_MODULE Mod;
	TEST( HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND ) == 
		CfixCreateTestModuleFromPeImage( L"0000.dll", &Mod ) );
}

static void TestDllWithNoTestExports()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"cdiag.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );

	TEST( 0 == _wcsicmp( Mod->Name,  L"cdiag" ) );
	TEST( 0 == Mod->FixtureCount );

	Mod->Routines.Dereference( Mod );
}

static void TestDllWithDuplicateSetup()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib3.dll" ) );

	TEST( CFIX_E_DUP_SETUP_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithDuplicateBefore()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib7.dll" ) );

	TEST( CFIX_E_DUP_BEFORE_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithDuplicateAfter()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib8.dll" ) );

	TEST( CFIX_E_DUP_AFTER_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithDuplicateTeardown()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib2.dll" ) );

	TEST( CFIX_E_DUP_TEARDOWN_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithEmptyFixture()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib1.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );
	TEST( 0 == _wcsicmp( Mod->Name,  L"testlib1" ) );
	TEST( 1 == Mod->FixtureCount );
	
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->Name, L"SampleFixture" ) );
	TEST( 0 == Mod->Fixtures[ 0 ]->SetupRoutine );
	TEST( 0 == Mod->Fixtures[ 0 ]->TeardownRoutine );
	TEST( 0 == Mod->Fixtures[ 0 ]->TestCaseCount );
	TEST( Mod->Fixtures[ 0 ]->Module == Mod );

	Mod->Routines.Dereference( Mod );
}

static void TestDllWithNameTooLong()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib5.dll" ) );

	TEST( CFIX_E_FIXTURE_NAME_TOO_LONG == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithValidFixture()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib4.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );
	TEST( 0 == _wcsicmp( Mod->Name,  L"testlib4" ) );
	TEST( 2 == Mod->FixtureCount );
	
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->Name, L"SampleFixture1" ) );
	TEST( Mod->Fixtures[ 0 ]->SetupRoutine );
	TEST( Mod->Fixtures[ 0 ]->TeardownRoutine );
	TEST( Mod->Fixtures[ 0 ]->BeforeRoutine );
	TEST( Mod->Fixtures[ 0 ]->AfterRoutine );
	TEST( 2 == Mod->Fixtures[ 0 ]->TestCaseCount );
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Name, L"Test01" ) );
	TEST( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Fixture == Mod->Fixtures[ 0 ] );
	TEST( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Routine );

	#pragma warning( push )
	#pragma warning( disable : 6385 )
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->TestCases[ 1 ].Name, L"Test02" ) );
	TEST( Mod->Fixtures[ 0 ]->TestCases[ 1 ].Routine );

	TEST( Mod->Fixtures[ 1 ]->SetupRoutine );
	TEST( Mod->Fixtures[ 1 ]->TeardownRoutine );
	TEST( Mod->Fixtures[ 1 ]->BeforeRoutine == 0 );
	TEST( Mod->Fixtures[ 1 ]->AfterRoutine == 0 );
	#pragma warning( pop )

	Mod->Routines.Dereference( Mod );
}

CFIX_BEGIN_FIXTURE(PeLoading)
	CFIX_FIXTURE_ENTRY(TestNonExistingDll)
	CFIX_FIXTURE_ENTRY(TestDllWithNoTestExports)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateSetup)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateTeardown)
	CFIX_FIXTURE_ENTRY(TestDllWithEmptyFixture)
	CFIX_FIXTURE_ENTRY(TestDllWithNameTooLong)
	CFIX_FIXTURE_ENTRY(TestDllWithValidFixture)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateBefore)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateAfter)
CFIX_END_FIXTURE()