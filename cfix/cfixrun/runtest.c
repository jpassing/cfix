/*----------------------------------------------------------------------
 * Purpose:
 *		Fixtures Display.
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
#include "internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>

typedef struct _SEARCH_CONTEXT
{
	PCFIX_ACTION SequenceAction;
	PCFIXRUN_STATE State;
} SEARCH_CONTEXT, *PSEARCH_CONTEXT;

static HRESULT CfixrunsAddFixturesOfDllToSequenceAction(
	__in PCWSTR Path,
	__in PVOID Context,
	__in BOOL SearchPerformed
	)
{
	PSEARCH_CONTEXT SearchCtx = ( PSEARCH_CONTEXT ) Context;
	PCFIX_TEST_MODULE TestModule;
	HRESULT Hr;

	ASSERT( CfixIsValidAction( SearchCtx->SequenceAction ) );
	
	Hr = CfixCreateTestModuleFromPeImage( Path, &TestModule );
	if ( FAILED( Hr ) )
	{
		if ( SearchPerformed )
		{
			//
			// Nevermind, this is probably just one of many DLLs.
			//
			CfixrunpOutputLogMessage(
				SearchCtx->State->LogSession,
				JpdiagInfoSeverity,
				L"Failed to load module %s: 0x%08X\n",
				Path,
				Hr );
			return S_OK;
		}
		else
		{
			//
			// DLL was specified explicitly -> fail.
			//
			return Hr;
		}
	}

	CfixrunpOutputLogMessage(
		SearchCtx->State->LogSession,
		JpdiagInfoSeverity,
		L"Loaded module %s\n",
		Path );

	if ( TestModule->FixtureCount > 0 )
	{
		UINT Fixture;

		for ( Fixture = 0; Fixture < TestModule->FixtureCount; Fixture++ )
		{
			PCFIX_ACTION TsexecAction;
			PCWSTR FixtureName = TestModule->Fixtures[ Fixture ]->Name;

			//
			// Apply filters.
			//
			if ( SearchCtx->State->Options->Fixture )
			{
				if ( 0 != _wcsicmp( SearchCtx->State->Options->Fixture, FixtureName ) )
				{
					//
					// Skip.
					//
					continue;
				}
			}
			else if ( SearchCtx->State->Options->FixturePrefix )
			{
				if ( FixtureName != StrStrI( FixtureName, SearchCtx->State->Options->FixturePrefix ) )
				{
					//
					// Skip.
					//
					continue;
				}
			}

			Hr = CfixCreateFixtureExecutionAction(
				TestModule->Fixtures[ Fixture ],
				&TsexecAction );
			if ( FAILED( Hr ) )
			{
				CfixrunpOutputLogMessage(
					SearchCtx->State->LogSession,
					JpdiagErrorSeverity,
					L"Failed to create fixture execution action: 0x%08X\n",
					Hr );
				break;
			}
			else
			{
				//
				// Add to sequence
				//
				Hr = CfixAddEntrySequenceAction(
					SearchCtx->SequenceAction,
					TsexecAction );
				if ( FAILED( Hr ) )
				{
					CfixrunpOutputLogMessage(
						SearchCtx->State->LogSession,
						JpdiagErrorSeverity,
						L"Failed to enqueue fixture execution action: 0x%08X\n",
						Hr );
					break;
				}

				TsexecAction->Dereference( TsexecAction );
			}
		}
	}

	TestModule->Routines.Dereference( TestModule );
	return S_OK;
}

HRESULT CfixrunpRunFixtures( 
	__in PCFIXRUN_STATE State,
	__out PDWORD ExitCode
	)
{
	PCFIX_ACTION SequenceAction;
	SEARCH_CONTEXT SearchCtx;

	HRESULT Hr;
	
	if ( ! State || ! ExitCode )
	{
		return E_INVALIDARG;
	}

	*ExitCode = CFIXRUN_EXIT_FAILURE;
	
	Hr = CfixCreateSequenceAction( &SequenceAction );
	if ( FAILED( Hr ) )
	{
		CfixrunpOutputLogMessage(
			State->LogSession,
			JpdiagErrorSeverity,
			L"Failed to create sequence action: 0x%08X\n",
			Hr );
		return Hr;
	}

	//
	// Search DLLs and and populate seuqence.
	//
	SearchCtx.SequenceAction = SequenceAction;
	SearchCtx.State = State;
	Hr = CfixrunSearchDlls(
		State->Options->DllOrDirectory,
		State->Options->RecursiveSearch,
		CfixrunsAddFixturesOfDllToSequenceAction,
		&SearchCtx );
	if ( SUCCEEDED( Hr ) )
	{
		PCFIX_EXECUTION_CONTEXT ExecCtx;
		Hr = JurunpCreateExecutionContext( State, &ExecCtx );
		if ( SUCCEEDED( Hr ) )
		{
			CFIXRUN_STATISTICS Statistics;

			//
			// Let's rock!
			//
			Hr = SequenceAction->Run( SequenceAction, ExecCtx );

			//
			// Fetch statistics.
			//
			JurunpGetStatisticsExecutionContext( ExecCtx, &Statistics );

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
				wprintf( 
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

			JurunpDeleteExecutionContext( ExecCtx );
		}
	}

	SequenceAction->Dereference( SequenceAction );

	return Hr;
}