/*----------------------------------------------------------------------
 * Purpose:
 *		PE assessment test.
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

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"cdiag.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );

	TEST( 0 == _wcsicmp( Mod->Name,  L"cdiag" ) );
	TEST( 0 == Mod->FixtureCount );

	Mod->Routines.Dereference( Mod );
}

static void TestCurrentExecutable()
{
	PCFIX_TEST_MODULE Mod;

	TEST_HR( CfixCreateTestModule( GetModuleHandle( NULL ), &Mod ) );

	TEST( 0 == Mod->FixtureCount );

	Mod->Routines.Dereference( Mod );
}

static void TestDllWithDuplicateSetup()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib3.dll" ) );

	TEST( CFIX_E_DUP_SETUP_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithDuplicateBefore()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib7.dll" ) );

	TEST( CFIX_E_DUP_BEFORE_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithDuplicateAfter()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib8.dll" ) );

	TEST( CFIX_E_DUP_AFTER_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithDuplicateTeardown()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib2.dll" ) );

	TEST( CFIX_E_DUP_TEARDOWN_ROUTINE == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithEmptyFixture()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
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

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib5.dll" ) );

	TEST( CFIX_E_FIXTURE_NAME_TOO_LONG == CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

static void TestDllWithValidFixture()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib4.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );
	TEST( 0 == _wcsicmp( Mod->Name,  L"testlib4" ) );
	TEST( 2 == Mod->FixtureCount );
	
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->Name, L"SampleFixture1" ) );
	TEST( Mod->Fixtures[ 0 ]->ApiType == CfixApiTypeBase );
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

	TEST( Mod->Fixtures[ 0 ]->ApiType == CfixApiTypeBase );
	TEST( Mod->Fixtures[ 1 ]->SetupRoutine );
	TEST( Mod->Fixtures[ 1 ]->TeardownRoutine );
	TEST( Mod->Fixtures[ 1 ]->BeforeRoutine == 0 );
	TEST( Mod->Fixtures[ 1 ]->AfterRoutine == 0 );
	#pragma warning( pop )

	Mod->Routines.Dereference( Mod );
}

static void TestApiVersionMacros()
{
	TEST_EQ( 0xBBBB000A, CFIX_PE_API_MAKEAPIVERSION( 0xAAAA, 0xBBBB ) );
	TEST_EQ( 0xBBBBCCCA, CFIX_PE_API_MAKEAPIVERSION_EX( 0xAAAA, 0xBBBB, 0xCCCC ) );
}

static void TestDllWithValidFixtureFlags()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib9.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );
	TEST( 0 == _wcsicmp( Mod->Name,  L"testlib9" ) );
	TEST( 3 == Mod->FixtureCount );
	
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->Name, L"CppFixtureWithFlags" ) );
	TEST( Mod->Fixtures[ 0 ]->ApiType == CfixApiTypeCc );
	TEST( Mod->Fixtures[ 0 ]->Flags == CFIX_FIXTURE_USES_ANONYMOUS_THREADS );
	TEST( Mod->Fixtures[ 0 ]->TestCaseCount == 1 );
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 0 ]->TestCases[ 0 ].Name, L"Method01" ) );

	TEST( 0 == _wcsicmp( Mod->Fixtures[ 1 ]->Name, L"FixtureWithFlags" ) );
	TEST( Mod->Fixtures[ 1 ]->ApiType == CfixApiTypeBase );
	TEST( Mod->Fixtures[ 1 ]->Flags == CFIX_FIXTURE_USES_ANONYMOUS_THREADS );
	TEST( Mod->Fixtures[ 1 ]->TestCaseCount == 1 );
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 1 ]->TestCases[ 0 ].Name, L"Test01" ) );

	TEST( 0 == _wcsicmp( Mod->Fixtures[ 2 ]->Name, L"FixtureWithNoFlags" ) );
	TEST( Mod->Fixtures[ 2 ]->ApiType == CfixApiTypeBase );
	TEST( Mod->Fixtures[ 2 ]->Flags == 0 );
	TEST( Mod->Fixtures[ 2 ]->TestCaseCount == 1 );
	TEST( 0 == _wcsicmp( Mod->Fixtures[ 2 ]->TestCases[ 0 ].Name, L"Test01" ) );

	Mod->Routines.Dereference( Mod );
}


static void TestDllWithInvalidFlagsCausesLoadFailure()
{
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib10.dll" ) );

	CFIX_ASSERT_HRESULT( 
		CFIX_E_INVALID_FIXTURE_FLAG, 
		CfixCreateTestModuleFromPeImage( Path, &Mod ) );
}

CFIX_BEGIN_FIXTURE(PeLoading)
	CFIX_FIXTURE_ENTRY(TestNonExistingDll)
	CFIX_FIXTURE_ENTRY(TestDllWithNoTestExports)
	CFIX_FIXTURE_ENTRY(TestCurrentExecutable)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateSetup)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateTeardown)
	CFIX_FIXTURE_ENTRY(TestDllWithEmptyFixture)
	CFIX_FIXTURE_ENTRY(TestDllWithNameTooLong)
	CFIX_FIXTURE_ENTRY(TestDllWithValidFixture)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateBefore)
	CFIX_FIXTURE_ENTRY(TestDllWithDuplicateAfter)
	CFIX_FIXTURE_ENTRY(TestApiVersionMacros)
	CFIX_FIXTURE_ENTRY(TestDllWithValidFixtureFlags)
	CFIX_FIXTURE_ENTRY(TestDllWithInvalidFlagsCausesLoadFailure)
CFIX_END_FIXTURE()