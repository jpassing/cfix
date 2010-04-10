/*----------------------------------------------------------------------
 * Purpose:
 *		Test all cmdline options.
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
#include <stdlib.h>
#include <cfixrun.h>
#include "test.h"

WCHAR SampleTestDllPath[ MAX_PATH ];

static struct
{
	PCWSTR DllOrDirectory;
	BOOL DisplayOnly;
	DWORD ExpectedExitCode;
} DllOrDirectory[] = 
{
	{ SampleTestDllPath,	FALSE,	CFIXRUN_EXIT_ALL_SUCCEEDED },
	{ SampleTestDllPath,	TRUE,	CFIXRUN_EXIT_NONE_EXECUTED },
	{ L"idonotexist",		FALSE, 	CFIXRUN_EXIT_FAILURE },
	{ L"",					FALSE, 	CFIXRUN_EXIT_USAGE_FAILURE },
	{ NULL,					FALSE, 	CFIXRUN_EXIT_USAGE_FAILURE }
};

static struct 
{
	PCWSTR FixtureName;
	PCWSTR FixturePrefix;
	DWORD ExpectedExitCode;
} FixtureNames[] = 
{
	{ L"SampleFixture1", NULL,	CFIXRUN_EXIT_ALL_SUCCEEDED },
	{ NULL, L"SampleFixture1",	CFIXRUN_EXIT_ALL_SUCCEEDED },
	{ NULL, L"Sample",			CFIXRUN_EXIT_ALL_SUCCEEDED },
	{ L"???", NULL, 			CFIXRUN_EXIT_NONE_EXECUTED },
	{ NULL, L"???", 			CFIXRUN_EXIT_NONE_EXECUTED },
	{ NULL, NULL,				CFIXRUN_EXIT_ALL_SUCCEEDED }
};

static int __cdecl PrintNop(
		__in_z __format_string const wchar_t * _Format, 
		... 
		)
{
	UNREFERENCED_PARAMETER( _Format );
	return 0;
}

void TestOptions()
{
	CFIXRUN_OPTIONS Options;
	UINT DllIndex, FixtureIndex;

	// 
	// Construct path to some DLL.
	//
	TEST( GetModuleFileName( 
		ModuleHandle,
		SampleTestDllPath,
		_countof( SampleTestDllPath ) ) );
	TEST( PathRemoveFileSpec( SampleTestDllPath ) );
	TEST( PathAppend( SampleTestDllPath, L"testlib4.dll" ) );

	for ( DllIndex = 0; DllIndex < _countof( DllOrDirectory ); DllIndex++ )
	for ( FixtureIndex = 0; FixtureIndex < _countof( FixtureNames ); FixtureIndex++ )
	{
		DWORD Exit;
		ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
		Options.PrintConsole = PrintNop;

		Options.InputFileType	= CfixrunInputDynamicallyLoadable;
		Options.InputFile		= DllOrDirectory[ DllIndex ].DllOrDirectory;
		Options.Fixture			= FixtureNames[ FixtureIndex ].FixtureName;
		Options.FixturePrefix	= FixtureNames[ FixtureIndex ].FixturePrefix;

		Options.RecursiveSearch					= TRUE;
		Options.AbortOnFirstFailure				= TRUE;
		Options.DoNotCatchUnhandledExceptions	= TRUE;
		Options.DisplayOnly						= DllOrDirectory[ DllIndex ].DisplayOnly;
		Options.Summary							= FALSE;
		Options.AlwaysBreakOnFailure			= TRUE;

		Exit = CfixrunMain( &Options );

		TEST( Exit == DllOrDirectory[ DllIndex ].ExpectedExitCode ||
			  Exit == FixtureNames[ FixtureIndex ].ExpectedExitCode );
	}
	
}

CFIX_BEGIN_FIXTURE(OptionsTest)
	CFIX_FIXTURE_ENTRY(TestOptions)
CFIX_END_FIXTURE()