/*----------------------------------------------------------------------
 * Purpose:
 *		Test runner.
 *
 *		Note that all implementation code is located in jpurun.lib.
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
			L"cfix version %d.%d.%d.%d (" __CFIX_WIDE( DDKBUILDENV ) L")\n"
			L"(C) 2008-2009 - Johannes Passing - http://www.cfix-testing.org/\n",
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
		L"                    <fixture> may in the format fixture.test, in which case only the specific test is run\n"
		L"    -p <prefix>    Run fixtures whose name starts with <prefix>\n"
		L"\n"
		L"  Execution Options:\n"
		L"    -f             Abort immediately on first failure - no breakpoint will be triggered\n"
		L"    -fsf           Short-circuit fixture - when a test case fails, skip remaining test cases of\n"
		L"                   enclosing fixture\n"
		L"                   (Default: Report failure, but continue fixture)\n"
		L"    -fss           Short-circuit testrun on failing setup - when a setup routine fails,\n"
		L"                   abort testrun\n"
		L"                   (Default: Shortcut fixture, but continue testrun)\n"
		L"    -fsr           Short-circuit testrun - when a test case fails, abort testrun - \n"
		L"                   implies -fsf and -fss\n"
		L"                   (Default: Continue testrun)\n"
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
		L"    -exe           Enable support for EXE modules\n"
		L"                   When this witch is used, the exact path to the module must be specified;\n"
		L"                   wilcards and the -r option are not supported\n"
		L"\n"
		L"  Output Options:\n"
		L"    -nologo        Do not display logo\n"
		L"    -out [<target>] Output progress messages to <target>\n"
		L"    -log [<target>] Output log messages to <target>\n"
		L"    -td             Disable stack trace capturing\n"
		L"    -ts             Omit source information in stack traces\n"
		L"\n"
		L"    Targets:\n"
		L"      debug        Print to debug console\n"
		L"      console      Print to console\n"
		L"      <path>       Output to file (UTF-8)\n"
		L"\n"
		L"      Default target is 'debug' if run in the debugger, else 'console'.\n"
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

static BOOL CfixcmdsCreatePipe(
	__out HANDLE *ReadEnd,
	__out HANDLE *WriteEnd
	)
{
	HANDLE ReadEndTemp;
	SECURITY_ATTRIBUTES SecurityAttr;
	
	SecurityAttr.nLength = sizeof( SECURITY_ATTRIBUTES );
	SecurityAttr.bInheritHandle = TRUE;
	SecurityAttr.lpSecurityDescriptor = NULL;

	//
	// Create pipe with two inheritable handles.
	//
	if ( ! CreatePipe( 
			&ReadEndTemp, 
			WriteEnd, 
			&SecurityAttr, 
			0 ) )
	{
		return FALSE;
	}

	//
	// Convert the read end into a noninheritable handle.
	//
	if ( DuplicateHandle( 
			GetCurrentProcess(), 
			ReadEndTemp, 
			GetCurrentProcess(), 
			ReadEnd, 
			0, 
			FALSE, 
			DUPLICATE_SAME_ACCESS ) )
	{
		CloseHandle( ReadEndTemp );

		return TRUE;
	}
	else
	{
		CloseHandle( ReadEndTemp );
		CloseHandle( *WriteEnd );

		return FALSE;
	}
}

static BOOL CfixcmdsRelayOutput(
	__in HANDLE ReadHandle
	)
{
	BYTE Buffer[ 512 ];
	DWORD BytesRead;
	DWORD BytesWritten;
	HANDLE StdoutHandle = GetStdHandle( STD_OUTPUT_HANDLE );

	if ( ! StdoutHandle )
	{
		return FALSE;
	}

	for ( ;; )
	{
		if ( ! ReadFile(
			ReadHandle,
			Buffer,
			sizeof( Buffer ),
			&BytesRead,
			NULL ) )
		{
			break;
		}

		if ( ! WriteFile(
			StdoutHandle,
			Buffer,
			BytesRead,
			&BytesWritten,
			NULL ) )
		{
			break;
		}
	}

	CloseHandle( StdoutHandle );
	return TRUE;
}

static DWORD CfixcmdsSpawnAndRun(
	__in PCFIXRUN_OPTIONS Options
	)
{
	WCHAR CommandLine[] = L"";
	DWORD ExitCode;
	PROCESS_INFORMATION ProcessInfo;
	STARTUPINFO StartupInfo;

	HANDLE PipeReadHandle  = NULL;
    HANDLE PipeWriteHandle = NULL;
    
	if ( GetFileAttributes( Options->InputFile ) == INVALID_FILE_ATTRIBUTES )
	{
		Options->PrintConsole( 
			L"The file %s could not be found\n",
			Options->InputFile );
		return CFIXRUN_EXIT_USAGE_FAILURE;
	}

	//
	// Inject cfixemb.dll and pass it our command line via environment
	// variables.
	//

	( VOID ) SetEnvironmentVariable(
		CFIX_EMB_INIT_ENVVAR_NAME,
		L"cfixemb.dll!CfixEmbMain" );
	( VOID ) SetEnvironmentVariable(
		CFIXRUN_EMB_CMDLINE_ENVVAR_NAME,
		GetCommandLine() );

	//
	// To support GUI children, use a pipe to relay console I/O.
	//
	if ( ! CfixcmdsCreatePipe( &PipeReadHandle, &PipeWriteHandle ) )
	{
		Options->PrintConsole( 
			L"Failed to create I/O pipes: Win32 error %d\n",
			GetLastError() );

		return CFIXRUN_EXIT_USAGE_FAILURE;
	}

	ZeroMemory( &ProcessInfo, sizeof( PROCESS_INFORMATION ) );
	ZeroMemory( &StartupInfo, sizeof( STARTUPINFO ) );
    StartupInfo.cb = sizeof( STARTUPINFO );
    
	StartupInfo.hStdOutput = PipeWriteHandle;
	StartupInfo.hStdError  = PipeWriteHandle;
	StartupInfo.dwFlags    = STARTF_USESTDHANDLES;

	if ( ! CreateProcess(
		Options->InputFile,
		CommandLine,
		NULL,
		NULL,
		TRUE,
		0,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInfo ) )
	{
		Options->PrintConsole( 
			L"The file %s could not launched: Win32 error %d\n",
			Options->InputFile,
			GetLastError() );

		ExitCode = CFIXRUN_EXIT_USAGE_FAILURE;
	}

	CloseHandle( PipeWriteHandle );

	if ( ! CfixcmdsRelayOutput( PipeReadHandle ) )
	{
		Options->PrintConsole( 
			L"Failed to relay I/O: Win32 error %d\n",
			Options->InputFile,
			GetLastError() );

		ExitCode = CFIXRUN_EXIT_USAGE_FAILURE;
	}

	( VOID ) WaitForSingleObject( ProcessInfo.hProcess, INFINITE );
	( VOID ) GetExitCodeProcess( ProcessInfo.hProcess, &ExitCode );

	CloseHandle( ProcessInfo.hThread );
	CloseHandle( ProcessInfo.hProcess );
	CloseHandle( PipeReadHandle );

	return ExitCode;
}

int __cdecl wmain(
	__in UINT Argc,
	__in PCWSTR *Argv
	)
{
	CFIXRUN_OPTIONS Options;
	DWORD ExitCode;

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );

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

	if ( Options.InputFileType == CfixrunInputRequiresSpawn )
	{
		ExitCode = CfixcmdsSpawnAndRun( &Options );
	}
	else
	{
		//
		// No 'drive not ready'-dialogs, please.
		//
		SetErrorMode( SetErrorMode( 0 ) | SEM_FAILCRITICALERRORS );

		ExitCode = CfixrunMain( &Options );
	}

#ifdef DBG	
	_CrtDumpMemoryLeaks();
#endif

	return ExitCode;
}