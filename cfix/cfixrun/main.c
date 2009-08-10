/*----------------------------------------------------------------------
 * Purpose:
 *		Main routine.
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
#include <conio.h>
#include "cfixrunp.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

static HRESULT CfixrunsMainWorker(
	__in PCFIXRUN_STATE State,
	__out PDWORD ExitCode
	)
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	ULONG FixtureCount;

	ASSERT( ExitCode );

	if ( State->Options->InputFile == NULL ||
		 wcslen( State->Options->InputFile ) == 0 )
	{
		*ExitCode = CFIXRUN_EXIT_USAGE_FAILURE;
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

	//
	// Search modules and construct a composite action.
	//
	
	if ( State->Options->DisplayOnly )
	{
		Hr = CfixrunpAssembleDisplayAction( State, &Action, &FixtureCount );
	}
	else
	{
		Hr = CfixrunpAssembleExecutionAction( State, &Action, &FixtureCount );
	}
		
	if ( FAILED( Hr ) )
	{
		*ExitCode = CFIXRUN_EXIT_FAILURE;
	}
	else if ( FixtureCount == 0 )
	{
		State->Options->PrintConsole( 
			L"No matching test modules/fixtures found\n" );

		if ( ! State->Options->EnableKernelFeatures )
		{
			State->Options->PrintConsole( 
				L"Note: Kernel mode tests were not included in the search - \n"
				L"      Use the -kern switch to search for kernel mode tests as well.\n" );
		}

		*ExitCode = CFIXRUN_EXIT_NONE_EXECUTED;
	}
	else
	{
		PCFIX_EXECUTION_CONTEXT ExecCtx;
		Hr = CfixrunpCreateExecutionContext( State, &ExecCtx );
		if ( SUCCEEDED( Hr ) )
		{
			CFIXRUN_STATISTICS Statistics;

			//
			// Let's rock!
			//
			Hr = Action->Run( Action, ExecCtx );

			//
			// Fetch statistics.
			//
			CfixrunpGetStatisticsExecutionContext( ExecCtx, &Statistics );

			if ( Statistics.TestCases == 0 )
			{
				*ExitCode = CFIXRUN_EXIT_NONE_EXECUTED;
			}
			else if ( Statistics.FailedTestCases == 0 )
			{
				*ExitCode = CFIXRUN_EXIT_ALL_SUCCEEDED;
			}
			else 
			{
				*ExitCode = CFIXRUN_EXIT_SOME_FAILED;
			}

			if ( State->Options->Summary )
			{
				State->Options->PrintConsole( 
					L"\n\n"
					L"%8d Fixtures\n"
					L"%8d Test cases\n"
					L"    %8d succeeded\n"
					L"    %8d failed\n"
					L"    %8d inconclusive\n\n",
					Statistics.Fixtures,
					Statistics.TestCases,
					Statistics.SucceededTestCases,
					Statistics.FailedTestCases,
					Statistics.InconclusiveTestCases );
			}

			ExecCtx->Dereference( ExecCtx );
		}

		Action->Dereference( Action );
	}

	return Hr;
}

static VOID CfixrunsOutputConsole(
	__in PCWSTR Text 
	)
{
	wprintf( L"%s", Text );
}

static VOID CfixrunsOutputConsoleAndDebug(
	__in PCWSTR Text 
	)
{
	wprintf( L"%s", Text );
	OutputDebugString( Text );
}

static HRESULT CfixrunsCreateOutputHandler(
	__in CDIAG_SESSION_HANDLE Session,
	__in CFIXRUN_OUTPUT_TARGET Target,
	__in PCWSTR FileName,
	__out PCDIAG_HANDLER *Handler
	)
{
	if ( Target == CfixrunTargetDebug ||
		 Target == CfixrunTargetConsole )
	{
		CDIAG_OUTPUT_ROUTINE Routine;
		if ( Target == CfixrunTargetDebug )
		{
			Routine = CfixrunsOutputConsoleAndDebug;
		}
		else
		{
			Routine = CfixrunsOutputConsole;
		}

		return CdiagCreateOutputHandler(
			Session,
			Routine,
			Handler );
	}
	else if ( Target == CfixrunTargetFile )
	{
		//
		// File.
		//
		return CdiagCreateTextFileHandler(
			Session,
			FileName,
			CdiagEncodingUtf8,
			Handler );
	}
	else
	{
		//
		// No handler.
		//
		*Handler = NULL;
		return S_OK;
	}
}

DWORD CfixrunMain(
	__in PCFIXRUN_OPTIONS Options
	)
{
	//
	// Initialize state.
	//
	CFIXRUN_STATE State;
	PCDIAG_HANDLER LogHandler = NULL;
	PCDIAG_HANDLER ProgressHandler = NULL;
	HRESULT Hr;
	DWORD ExitCode = CFIXRUN_EXIT_FAILURE;

	ZeroMemory( &State, sizeof( CFIXRUN_STATE ) );

	State.Options = Options;
	State.Formatter = NULL;

	ASSERT( Options->PrintConsole );

	//
	// Resolver.
	//
	Hr = CdiagCreateMessageResolver( &State.Resolver );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	State.Resolver->RegisterMessageDll(
		State.Resolver,
		L"cdiag.dll",
		0,
		0 );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	State.Resolver->RegisterMessageDll(
		State.Resolver,
		L"cfix.dll",
		0,
		0 );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	State.Resolver->RegisterMessageDll(
		State.Resolver,
		L"cfixkl.dll",
		0,
		0 );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	//
	// Formatter.
	//
	Hr = CfixrunpCreateFormatter(
		State.Resolver,
		State.Options->OmitSourceInfoInStackTrace ?
			0 : CFIXRUNP_FORMATTER_SHOW_STACKTRACE_SOURCE_INFORMATION,
		&State.Formatter );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}
	
	//
	// Log Session.
	//
	Hr = CdiagCreateSession( 
		State.Formatter,
		State.Resolver,
		&State.LogSession );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	Hr = CfixrunsCreateOutputHandler(
		State.LogSession,
		State.Options->LogOutputTarget,
		State.Options->LogOutputTargetName,
		&LogHandler );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	if ( LogHandler )
	{
		Hr = CdiagSetInformationSession(
			State.LogSession,
			CdiagSessionDefaultHandler,
			0,
			LogHandler );
		if ( FAILED( Hr ) )
		{
			goto Cleanup;
		}
	}

	//
	// Progress Session.
	//
	Hr = CdiagCreateSession( 
		State.Formatter,
		State.Resolver,
		&State.ProgressSession );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	Hr = CfixrunsCreateOutputHandler(
		State.ProgressSession,
		State.Options->ProgressOutputTarget,
		State.Options->ProgressOutputTargetName,
		&ProgressHandler );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	if ( ProgressHandler )
	{
		Hr = CdiagSetInformationSession(
			State.ProgressSession,
			CdiagSessionDefaultHandler,
			0,
			ProgressHandler );
		if ( FAILED( Hr ) )
		{
			goto Cleanup;
		}
	}

	if ( Options->PauseAtBeginning )
	{
		Options->PrintConsole( L"Press any key to start testrun...\n" );
		( VOID ) _getch();
	}

	//
	// Ready to run.
	//
	Hr = CfixrunsMainWorker( &State, &ExitCode );
	if ( FAILED( Hr ) )
	{
		WCHAR ErrMsg[ 256 ];
		if ( FAILED( State.Resolver->ResolveMessage(
			State.Resolver,
			Hr,
			CDIAG_MSGRES_FALLBACK_TO_DEFAULT | CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,
			NULL,
			_countof( ErrMsg ),
			ErrMsg ) ) )
		{
			VERIFY( SUCCEEDED( StringCchPrintf( 
				ErrMsg, 
				_countof( ErrMsg ), 
				L"Error 0x%08X",
				Hr ) ) );
		}

		Options->PrintConsole( L"%s\n", ErrMsg );
	}

Cleanup:
	if ( ProgressHandler )
	{
		ProgressHandler->Dereference( ProgressHandler );
	}

	if ( LogHandler )
	{
		LogHandler->Dereference( LogHandler );
	}

	if ( State.LogSession )
	{
		VERIFY( SUCCEEDED( CdiagDereferenceSession( State.LogSession ) ) );
	}

	if ( State.ProgressSession )
	{
		VERIFY( SUCCEEDED( CdiagDereferenceSession( State.ProgressSession ) ) );
	}

	if ( State.Resolver )
	{
		State.Resolver->Dereference( State.Resolver );
	}

	if ( State.Formatter )
	{
		State.Formatter->Dereference( State.Formatter );
	}

	if ( Options->PauseAtEnd )
	{
		Options->PrintConsole( L"Finished. Press any key to continue...\n" );
		( VOID ) _getch();
	}

	_CrtDumpMemoryLeaks();

	return ExitCode;
}