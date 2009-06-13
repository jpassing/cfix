/*----------------------------------------------------------------------
 * Purpose:
 *		Testrun.
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
	CFIXRUN_OUTPUT_TARGET DefOutputProgress;

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	Options.PrintConsole = wprintf;

	if ( IsDebuggerPresent() )
	{
		DefOutputProgress = CfixrunTargetDebug;
	}
	else
	{
		DefOutputProgress = CfixrunTargetConsole;
	}

	TEST( ParseCommandLine( L"runtest foo.dll", &Options ) );
	TEST( 0 == wcscmp( Options.DllOrDirectory, L"foo.dll" ) );

	TEST( ParseCommandLine( L"runtest -f -d -b -u /y /nologo -z foo.dll", &Options ) );
	TEST( 0 == wcscmp( Options.DllOrDirectory, L"foo.dll" ) );
	TEST( Options.AbortOnFirstFailure );
	TEST( Options.DisplayOnly );
	TEST( Options.DoNotCatchUnhandledExceptions );
	TEST( Options.AlwaysBreakOnFailure );
	TEST( Options.Summary );
	TEST( Options.PauseAtEnd );
	TEST( Options.NoLogo );

	TEST( DefOutputProgress == Options.ProgressOutputTarget );
	TEST( CfixrunTargetNone == Options.LogOutputTarget );

	TEST( ParseCommandLine( L"runtest -out debug -log \"console\" foo.dll", &Options ) );
	TEST( 0 == wcscmp( Options.DllOrDirectory, L"foo.dll" ) );
	TEST( 0 == wcscmp( Options.ProgressOutputTargetName, L"debug" ) );
	TEST( 0 == wcscmp( Options.LogOutputTargetName, L"console" ) );
	TEST( ! Options.EnableKernelFeatures );

	TEST( ParseCommandLine( L"runtest -kern -out debug -out b foo.dll", &Options ) );
	TEST( 0 == wcscmp( Options.DllOrDirectory, L"foo.dll" ) );
	TEST( Options.ProgressOutputTarget == CfixrunTargetFile );
	TEST( 0 == wcscmp( Options.ProgressOutputTargetName, L"b" ) );
	TEST( ! Options.OmitSourceInfoInStackTrace );
	TEST( Options.EnableKernelFeatures );

	TEST( ParseCommandLine( L"runtest -Y -ts -r -n a -p b foo.dll", &Options ) );
	TEST( Options.RecursiveSearch );
	TEST( 0 == wcscmp( Options.DllOrDirectory, L"foo.dll" ) );
	TEST( 0 == wcscmp( Options.Fixture, L"a" ) );
	TEST( 0 == wcscmp( Options.FixturePrefix, L"b" ) );
	TEST( Options.OmitSourceInfoInStackTrace );
	TEST( Options.PauseAtBeginning );

	TEST ( ParseCommandLine( L"runtest -r", &Options ) );
	TEST( 0 == wcscmp( Options.DllOrDirectory, L"-r" ) );

	TEST ( ! ParseCommandLine( L"runtest foo.dll foo.dll", &Options ) );
	TEST ( ! ParseCommandLine( L"runtest -out foo.dll", &Options ) );
	TEST ( ! ParseCommandLine( L"runtest -out -r foo.dll", &Options ) );
}

CFIX_BEGIN_FIXTURE(CmdLineParser)
	CFIX_FIXTURE_ENTRY(TestCmdLineParser)
CFIX_END_FIXTURE()