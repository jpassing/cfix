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

static UINT CtxReportEventCalls = 0;
static UINT CtxBeforeFixtureStartCalls = 0;
static UINT CtxAfterFixtureFinishCalls = 0;
static UINT CtxBeforeTestCaseStartCalls = 0;
static UINT CtxAfterTestCaseFinishCalls = 0;
static UINT CtxOnUnhandledExceptionCalls = 0;
static BOOL FixtureRanToCompletion = FALSE;
static BOOL CaseRanToCompletion = FALSE;
static CFIX_REPORT_DISPOSITION Disp;
static UINT Events[ CfixEventLog + 1 ];

static CFIX_REPORT_DISPOSITION CtxReportEvent(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	UNREFERENCED_PARAMETER( This );
	CtxReportEventCalls++;
	Events[ Event->Type ]++;

	switch ( Event->Type )
	{
	case CfixEventLog:
		wprintf( L"Log: %s\n", Event->Info.Log.Message );
		break;

	case CfixEventInconclusivess:
		wprintf( L"Inconclusive: %s\n", Event->Info.Inconclusiveness.Message );
		break;

	case CfixEventFailedAssertion:
		wprintf( L"FailedAssertion: %s\n", Event->Info.FailedAssertion.Expression );
		break;

	case CfixEventUncaughtException:
		TEST( Event->Info.UncaughtException.ExceptionRecord.ExceptionCode == 'excp' );
		wprintf( L"UncaughtException: %x\n", Event->Info.UncaughtException.ExceptionRecord.ExceptionCode );
		break;
	}

	return Disp;
}

static VOID CtxBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_FIXTURE Fixture
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Fixture );
	CtxBeforeFixtureStartCalls++;
}

static VOID CtxAfterFixtureFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_FIXTURE Fixture,
	__in BOOL RanToCompletion
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Fixture );
	UNREFERENCED_PARAMETER( RanToCompletion );
	CtxAfterFixtureFinishCalls++;
	FixtureRanToCompletion = RanToCompletion;
}

static VOID CtxBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_TEST_CASE TestCase
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( TestCase );
	CtxBeforeTestCaseStartCalls++;
}

static VOID CtxAfterTestCaseFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_TEST_CASE TestCase,
	__in BOOL RanToCompletion
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( TestCase );
	UNREFERENCED_PARAMETER( RanToCompletion );
	CtxAfterTestCaseFinishCalls++;
	CaseRanToCompletion = RanToCompletion;
}

static HANDLE CtxCreateChildThread(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PSECURITY_ATTRIBUTES ThreadAttributes,
	__in SIZE_T StackSize,
	__in PTHREAD_START_ROUTINE StartAddress,
	__in PVOID UserParameter,
	__in DWORD CreationFlags,
	__in PDWORD ThreadId
	)
{
	UNREFERENCED_PARAMETER( This );

	return CreateThread(
		ThreadAttributes,
		StackSize,
		StartAddress,
		UserParameter,
		CreationFlags,
		ThreadId );
}

static VOID CtxOnUnhandledException(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PEXCEPTION_POINTERS ExcpPointers
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( ExcpPointers );
	CtxOnUnhandledExceptionCalls++;
}

void TestActionRun()
{
	CFIX_EXECUTION_CONTEXT Ctx = {
		CFIX_TEST_CONTEXT_VERSION,
		CtxReportEvent,
		CtxBeforeFixtureStart,
		CtxAfterFixtureFinish,
		CtxBeforeTestCaseStart,
		CtxAfterTestCaseFinish,
		CtxCreateChildThread,
		CtxOnUnhandledException
	};

	PCFIX_TEST_MODULE Mod;
	UINT Index = 0;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib6.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );

	for ( Index = 0; Index < Mod->FixtureCount; Index++ )
	{
		PCFIX_ACTION TsexecAction;
		UINT Runs;

		TEST_HR( CfixCreateFixtureExecutionAction(
			Mod->Fixtures[ Index ],
			&TsexecAction ) );
		for ( Runs = 0; Runs < 3; Runs++ )
		{
			//
			// Create and run sequence with [Runs] entries.
			//
			PCFIX_ACTION Action;
			UINT Added = 0;
			TEST_HR( CfixCreateSequenceAction( &Action ) );

			for ( Added = 0; Added < Runs; Added++ )
			{
				TEST_HR( CfixAddEntrySequenceAction(
					Action,
					TsexecAction ) );
			}

			CtxReportEventCalls = 0;
			CtxBeforeFixtureStartCalls = 0;
			CtxAfterFixtureFinishCalls = 0;
			CtxBeforeTestCaseStartCalls = 0;
			CtxAfterTestCaseFinishCalls = 0;
			CtxOnUnhandledExceptionCalls = 0;

			FixtureRanToCompletion = FALSE;
			CaseRanToCompletion = FALSE;
			ZeroMemory( Events, sizeof( Events ) );

			//
			// Results depend on testcase.
			//
			if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"JustSetupAndTearDown" ) )
			{
				Disp = CfixBreak;

				TEST_HR( Action->Run( Action, &Ctx ) );

				TEST( CtxReportEventCalls						== 0 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 0 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 0 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );

				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( ! CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"SetupTwoSuccTestsAndTearDown" ) )
			{
				Disp = CfixBreak;

				TEST_HR( Action->Run( Action, &Ctx ) );

				TEST( CtxReportEventCalls						== 2 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 2 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 2 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
				
				TEST( Events[ CfixEventLog ]					== 2 * Runs );

				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"SetupSucFailInconThrowTearDown" ) )
			{
				Disp = CfixContinue;

				TEST_HR( Action->Run( Action, &Ctx ) );

				if ( IsDebuggerPresent() )
				{
					TEST( Events[ CfixEventLog ]				== 3 * Runs );
					TEST( CtxReportEventCalls					== 6 * Runs );
				}
				else
				{
					// TC is aborted, so one log fewer.
					TEST( Events[ CfixEventLog ]				== 2 * Runs );
					TEST( CtxReportEventCalls					== 5 * Runs );
				}
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 4 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 4 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 1 * Runs );
														 
				TEST( Events[ CfixEventInconclusivess ]		== 1 * Runs );
				TEST( Events[ CfixEventFailedAssertion ]		== 1 * Runs );
				TEST( Events[ CfixEventUncaughtException ]	== 1 * Runs );

				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"AbortMeBecauseOfUnhandledExcp" ) )
			{
				HRESULT Hr;
				UINT EffectiveRuns = Runs > 0 ? 1 : 0;

				Disp = CfixAbort;
				
				Hr = Action->Run( Action, &Ctx );
				TEST( Hr == ( Runs > 0 ? CFIX_E_TESTRUN_ABORTED : S_OK ) );

				//
				// Note: 1st run should abort, so at most 1 run must have been
				// performed (see EffectiveRuns).
				//
				TEST( CtxReportEventCalls						== 1 * EffectiveRuns );
				TEST( CtxBeforeFixtureStartCalls				== 1 * EffectiveRuns );
				TEST( CtxAfterFixtureFinishCalls				== 1 * EffectiveRuns );
				TEST( CtxBeforeTestCaseStartCalls				== 1 * EffectiveRuns );
				TEST( CtxAfterTestCaseFinishCalls				== 1 * EffectiveRuns );
				TEST( CtxOnUnhandledExceptionCalls				== 1 * EffectiveRuns );
																	   
				TEST( Events[ CfixEventUncaughtException ]	== 1 * EffectiveRuns );

				if ( Runs > 0 )
				{
					TEST( ! FixtureRanToCompletion );
					TEST( ! CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"AbortMeBecauseOfFailure" ) )
			{
				HRESULT Hr;
				UINT EffectiveRuns = Runs > 0 ? 1 : 0;

				Disp = CfixAbort;

				Hr = Action->Run( Action, &Ctx );
				TEST( Hr == ( Runs > 0 ? CFIX_E_TESTRUN_ABORTED : S_OK ) );

				//
				// Note: 1st run should abort, so at most 1 run must have been
				// performed (see EffectiveRuns).
				//
				TEST( CtxReportEventCalls						== 2 * EffectiveRuns );
				TEST( CtxBeforeFixtureStartCalls				== 1 * EffectiveRuns );
				TEST( CtxAfterFixtureFinishCalls				== 1 * EffectiveRuns );
				TEST( CtxBeforeTestCaseStartCalls				== 1 * EffectiveRuns );
				TEST( CtxAfterTestCaseFinishCalls				== 1 * EffectiveRuns );
				TEST( CtxOnUnhandledExceptionCalls				== 0 * EffectiveRuns );
																	 
				TEST( Events[ CfixEventFailedAssertion ]		== 1 * EffectiveRuns );

				if ( Runs > 0 )
				{
					TEST( ! FixtureRanToCompletion );
					TEST( ! CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"TestValidEquals" ) )
			{
				Disp = CfixContinue;

				TEST_HR( Action->Run( Action, &Ctx ) );

				TEST( CtxReportEventCalls						== 0 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 1 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 1 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
				
				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"TestInvalidEquals" ) )
			{
				Disp = CfixContinue;

				TEST_HR( Action->Run( Action, &Ctx ) );

				TEST( CtxReportEventCalls						== 1 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 1 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 1 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
				
				TEST( Events[ CfixEventFailedAssertion ]		== 1 * Runs );

				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"FailSetupDoNotCallTeardown" ) )
			{
				if ( IsDebuggerPresent() )
				{
					//
					// Cannot be run in debugger.
					//
				}
				else
				{
					HRESULT ExpectedHr = Runs > 0 ? CFIX_E_SETUP_ROUTINE_FAILED : S_OK;
					Disp = CfixContinue;

					TEST_RETURN( ExpectedHr, Action->Run( Action, &Ctx ) );

					if ( Runs > 0 )
					{
						TEST( Events[ CfixEventFailedAssertion ]		== 1 );
	
						TEST( CtxReportEventCalls						== 1 );
						TEST( CtxBeforeFixtureStartCalls				== 1 );
						TEST( CtxAfterFixtureFinishCalls				== 1 );
					}
					else
					{
						TEST( Events[ CfixEventFailedAssertion ]		== 0 );
	
						TEST( CtxReportEventCalls						== 0 );
						TEST( CtxBeforeFixtureStartCalls				== 0 );
						TEST( CtxAfterFixtureFinishCalls				== 0 );
					}

					TEST( CtxBeforeTestCaseStartCalls				== 0 );
					TEST( CtxAfterTestCaseFinishCalls				== 0 );
					TEST( CtxOnUnhandledExceptionCalls				== 0 );
				
					if ( Runs > 0 )
					{
						TEST( ! FixtureRanToCompletion );
						TEST( ! CaseRanToCompletion );
					}
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"FailTeardown" ) )
			{
				Disp = CfixContinue;

				TEST_HR( Action->Run( Action, &Ctx ) );

				TEST( CtxReportEventCalls						== 1 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 0 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 0 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 1 * Runs );
				
				TEST( Events[ CfixEventFailedAssertion   ]		== 0 * Runs );
				TEST( Events[ CfixEventUncaughtException ]		== 1 * Runs );

				if ( Runs > 0 )
				{
					TEST( ! FixtureRanToCompletion );
					TEST( ! CaseRanToCompletion );
				}
			}
			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"TestCpp" ) )
			{
				Disp = CfixContinue;

				TEST_HR( Action->Run( Action, &Ctx ) );

				TEST( CtxReportEventCalls						== 1 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 1 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 1 * Runs );
				TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
				
				TEST( Events[ CfixEventFailedAssertion   ]		== 1 * Runs );

				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( CaseRanToCompletion );
				}
			}

			//
			// Threads tests. These fictures must fail as their child threads fail.
			//

			else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"AssertOnRegisteredThread" ) || 
					  0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"LogOnRegisteredThread" ) ||
					  0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"InconclusiveOnRegisteredThread" ) ||
					  0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"ThrowOnRegisteredThread" ) )
			{
				HRESULT Hr;

				Disp = CfixAbort;

				Hr = Action->Run( Action, &Ctx );

				//
				// N.B. Only the failing thread, not the testcase is aborted.
				//
				TEST( Hr == S_OK );

				TEST( CtxReportEventCalls						== 1 * Runs );
				TEST( CtxBeforeFixtureStartCalls				== 1 * Runs );
				TEST( CtxAfterFixtureFinishCalls				== 1 * Runs );
				TEST( CtxBeforeTestCaseStartCalls				== 1 * Runs );
				TEST( CtxAfterTestCaseFinishCalls				== 1 * Runs );
								
				if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"AssertOnRegisteredThread" ) )
				{
					TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
					TEST( Events[ CfixEventFailedAssertion	 ]		== 1 * Runs );
					TEST( Events[ CfixEventUncaughtException ]		== 0 * Runs );
					TEST( Events[ CfixEventInconclusivess	 ]		== 0 * Runs );
					TEST( Events[ CfixEventLog				 ]		== 0 * Runs );
				}
				else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"LogOnRegisteredThread" ) )
				{
					TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
					TEST( Events[ CfixEventFailedAssertion	 ]		== 0 * Runs );
					TEST( Events[ CfixEventUncaughtException ]		== 0 * Runs );
					TEST( Events[ CfixEventInconclusivess	 ]		== 0 * Runs );
					TEST( Events[ CfixEventLog				 ]		== 1 * Runs );
				}
				else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"InconclusiveOnRegisteredThread" ) )
				{
					TEST( CtxOnUnhandledExceptionCalls				== 0 * Runs );
					TEST( Events[ CfixEventFailedAssertion	 ]		== 0 * Runs );
					TEST( Events[ CfixEventUncaughtException ]		== 0 * Runs );
					TEST( Events[ CfixEventInconclusivess	 ]		== 1 * Runs );
					TEST( Events[ CfixEventLog				 ]		== 0 * Runs );
				}
				else if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, L"ThrowOnRegisteredThread" ) )
				{
					TEST( CtxOnUnhandledExceptionCalls				== 1 * Runs );
					TEST( Events[ CfixEventFailedAssertion	 ]		== 0 * Runs );
					TEST( Events[ CfixEventUncaughtException ]		== 1 * Runs );
					TEST( Events[ CfixEventInconclusivess	 ]		== 0 * Runs );
					TEST( Events[ CfixEventLog				 ]		== 0 * Runs );
				}

				if ( Runs > 0 )
				{
					TEST( FixtureRanToCompletion );
					TEST( CaseRanToCompletion );
				}
			}
			else
			{
				TEST( !"???" );
			}

			Action->Reference( Action );
			Action->Dereference( Action );
			Action->Dereference( Action );
		}

		TsexecAction->Reference( TsexecAction );
		TsexecAction->Dereference( TsexecAction );
		TsexecAction->Dereference( TsexecAction );
	}
	TEST( Index == 14 );

	Mod->Routines.Dereference( Mod );
}

CFIX_BEGIN_FIXTURE(ActionRun)
	CFIX_FIXTURE_ENTRY(TestActionRun)
CFIX_END_FIXTURE()