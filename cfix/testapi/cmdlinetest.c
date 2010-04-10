/*----------------------------------------------------------------------
 * Purpose:
 *		Testrun.
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
#include <cfixrun.h>
#include <Shellapi.h>

static BOOL ParseCommandLine(
	__in PCWSTR CmdLine,
	__out PCFIXRUN_OPTIONS Options
	)
{
	int Argc;
	PWSTR *Argv = CommandLineToArgvW( CmdLine, &Argc );
	return CfixrunParseCommandLine( Argc, Argv, Options );
}

void TestCmdLineParser()
{
	CFIXRUN_OPTIONS Options;

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest foo.dll", &Options ) );
	TEST( CfixrunInputDynamicallyLoadable == Options.InputFileType );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.dll" ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest -f -d -b -u /y /nologo -z foo.dll", &Options ) );
	TEST( CfixrunInputDynamicallyLoadable == Options.InputFileType );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.dll" ) );
	TEST( Options.AbortOnFirstFailure );
	TEST( Options.DisplayOnly );
	TEST( Options.DoNotCatchUnhandledExceptions );
	TEST( Options.AlwaysBreakOnFailure );
	TEST( Options.Summary );
	TEST( Options.PauseAtEnd );
	TEST( Options.NoLogo );
	TEST( ! Options.DisableStackTraces );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ! ParseCommandLine( L"runtest -exe -out debug /td foo.dll", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ! ParseCommandLine( L"runtest -exe -out debug /td -y foo.exe", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ! ParseCommandLine( L"runtest -exe -out debug /td -Y foo.exe", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ! ParseCommandLine( L"runtest -exe -out debug /td -r foo.exe", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest -exe -out debug /td -log \"console\" foo.exe", &Options ) );
	TEST( CfixrunInputRequiresSpawn == Options.InputFileType );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.exe" ) );
	TEST( ! Options.EnableKernelFeatures );
	TEST( Options.InputFileType == CfixrunInputRequiresSpawn );
	TEST( Options.DisableStackTraces );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest -kern -fsr -fss -out debug -out b foo.dll", &Options ) );
	TEST( CfixrunInputDynamicallyLoadable == Options.InputFileType );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.dll" ) );
	TEST( ! Options.OmitSourceInfoInStackTrace );
	TEST( Options.EnableKernelFeatures );
	TEST( Options.ShortCircuitRunOnFailure );
	TEST( Options.ShortCircuitRunOnSetupFailure );
	TEST( Options.EventDll == NULL );
	TEST( Options.EventDllOptions == NULL );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest -Y -ts -r -n a /fsf -p b foo.dll", &Options ) );
	TEST( Options.RecursiveSearch );
	TEST( CfixrunInputDynamicallyLoadable == Options.InputFileType );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.dll" ) );
	TEST( 0 == wcscmp( Options.Fixture, L"a" ) );
	TEST( 0 == wcscmp( Options.FixturePrefix, L"b" ) );
	TEST( Options.OmitSourceInfoInStackTrace );
	TEST( Options.PauseAtBeginning );
	TEST( Options.ShortCircuitFixtureOnFailure );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST ( ParseCommandLine( L"runtest -r", &Options ) );
	TEST( 0 == wcscmp( Options.InputFile, L"-r" ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST ( ! ParseCommandLine( L"runtest foo.dll foo.dll", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST ( ! ParseCommandLine( L"runtest -out foo.dll", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST ( ! ParseCommandLine( L"runtest -out -r foo.dll", &Options ) );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest -eventdll ev.dll foo.dll", &Options ) );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.dll" ) );
	TEST( 0 == wcscmp( Options.EventDll, L"ev.dll" ) );
	TEST( Options.EventDllOptions == NULL );

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	TEST( ParseCommandLine( L"runtest -eventdll ev.dll -eventdlloptions \"a b\" foo.dll", &Options ) );
	TEST( 0 == wcscmp( Options.InputFile, L"foo.dll" ) );
	TEST( 0 == wcscmp( Options.EventDll, L"ev.dll" ) );
	TEST( 0 == wcscmp( Options.EventDllOptions, L"a b" ) );
}

CFIX_BEGIN_FIXTURE(CmdLineParser)
	CFIX_FIXTURE_ENTRY(TestCmdLineParser)
CFIX_END_FIXTURE()