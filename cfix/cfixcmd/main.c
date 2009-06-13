/*----------------------------------------------------------------------
 * Purpose:
 *		Test runner.
 *
 *		Note that all implementation code is located in jpurun.lib.
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
#include <stdlib.h>
#include <stdio.h>
#include <cfixapi.h>
#include <cfixrun.h>
#include <cdiag.h>

static VOID CfixcmdsPrintBanner()
{
	CDIAG_MODULE_VERSION Version;
	WCHAR Path[ MAX_PATH ];

	if ( 0 != GetModuleFileName( NULL, Path, _countof( Path ) ) &&
		 S_OK == CdiagGetModuleVersion( Path, &Version ) )
	{
		wprintf( 
			L"Cfix version %d.%d.%d.%d (" __CFIX_WIDE( DDKBUILDENV ) L")\n"
			L"(c) 2008 - Johannes Passing - http://cfix.sf.net/\n",
			Version.Major, 
			Version.Minor,
			Version.Revision,
			Version.Build );
	}
}

static VOID CfixcmdsPrintUsage(
	__in PCWSTR BinName
	)
{
	CfixcmdsPrintBanner();
	wprintf( 
		L"\n"
		L"Usage:\n"
		L"  %s <options> <dll or directory>\n"
		L"\n"
		L"  Test Fixture Selection Options:\n"
		L"    -r             Recurse into directories to search for test DLLs\n"
		L"    -n <fixture>   Run <fixture>\n"
		L"    -p <prefix>    Run fixtures whose name starts with <prefix>\n"
		L"\n"
		L"  Execution Options:\n"
		L"    -f             Abort immediately on first failure - no breakpoint will be triggered\n"
		L"    -u             Do not catch unhandled exceptions\n"
		L"                   (Recommended for debugging)\n"
		L"    -b             Always break on failure, even if not run in user-mode debugger\n"
		L"                   (Allows JIT debugging/using the kernel debugger after a \n"
		L"                   failure has occured - ignored when -f is used)\n"
		L"    -d             Display tests only, do not run\n"
		L"    -z             Display summary at end of testrun\n"
		L"    -Y             Pause at beginning of testrun\n"
		L"    -y             Pause at end of testrun\n"
		L"    -kern          Enable kernel mode features\n"
		//L"    -Oca           Run testcases in alphabetic order\n"
		//L"    -Ocr           Run testcases in random order\n"
		//L"    -Osa           Run fixtures in alphabetic order\n"
		//L"    -Osr           Run fixtures in random order\n"
		L"\n"
		L"  Output Options:\n"
		L"    -nologo        Do not display logo\n"
		L"    -out [<target>] Output progress messages to <target>\n"
		L"    -log [<target>] Output log messages to <target>\n"
		L"    -ts             Omit source information in stack traces\n"
		//L"    -d <dir>      Create crash dump on failure and save it in <dir>\n"
		L"\n"
		L"    Targets:\n"
		L"      debug        Print to debug console\n"
		L"      console      Print to console\n"
		L"      <path>       Output to file (UTF-8)\n"
		L"\n"
		L"      Default target is 'debug' if run in the debugger, else 'console'.\n"
		//L"\n"
		//L"  Diagnostics Options:\n"
		//L"    -dv <session> <verbosity> Control verbosity\n"
		//L"                  Available Sessions:\n"
		//L"                      testrun   progress messages\n"
		//L"                      log       log messages\n"
		//L"                      cfix      cfix diagnostics\n"
		//L"                      default   cdiag default session\n"
		//L"                  Available Verbosities:\n"
		//L"                      debug, info, warn, wrror, fatal\n"
		L"\n"
		L"  Exit codes:\n"
		L"    %d  All tests succeeded\n"
		L"    %d  No tests executed\n"
		L"    %d  At least one test failed\n"
		L"    %d  Usage failure\n"
		L"    %d  Other failures\n"
		L"\n"
		L"  Report bugs to <passing@users.sourceforge.net>\n"
		L"\n",
		BinName,
		CFIXRUN_EXIT_ALL_SUCCEEDED,
		CFIXRUN_EXIT_NONE_EXECUTED,
		CFIXRUN_EXIT_SOME_FAILED,	
		CFIXRUN_EXIT_USAGE_FAILURE,
		CFIXRUN_EXIT_FAILURE );
}


int __cdecl wmain(
	__in UINT Argc,
	__in PCWSTR *Argv
	)
{
	CFIXRUN_OPTIONS Options;
	DWORD ExitCode;

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	Options.PrintConsole = wprintf;

	if ( Argc == 0 )
	{
		return CFIXRUN_EXIT_USAGE_FAILURE;
	}
	else if ( Argc < 2 )
	{
		CfixcmdsPrintUsage( Argv[ 0 ] );
		return CFIXRUN_EXIT_USAGE_FAILURE;
	}
	else if ( ! CfixrunParseCommandLine( Argc, Argv, &Options ) )
	{
		return CFIXRUN_EXIT_USAGE_FAILURE;
	}

	if ( ! Options.NoLogo )
	{
		CfixcmdsPrintBanner();
	}

	ExitCode = CfixrunMain( &Options );

#ifdef DBG	
	_CrtDumpMemoryLeaks();
#endif

	return ExitCode;
}