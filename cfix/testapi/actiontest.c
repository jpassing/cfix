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

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

typedef struct _TEST_EXECUTUTION_CONTEXT
{
	CFIX_EXECUTION_CONTEXT Base;

	ULONG RefCount;

	ULONG ExpectedMainThreadId;

	UINT ReportEventCalls;
	UINT BeforeFixtureStartCalls;
	UINT AfterFixtureFinishCalls;
	UINT BeforeTestCaseStartCalls;
	UINT AfterTestCaseFinishCalls;
	UINT OnUnhandledExceptionCalls;
	BOOL FixtureRanToCompletion;
	BOOL CaseRanToCompletion;
	CFIX_REPORT_DISPOSITION Disp;
	UINT Events[ CfixEventLog + 1 ];
} TEST_EXECUTUTION_CONTEXT, *PTEST_EXECUTUTION_CONTEXT;

static CFIX_REPORT_DISPOSITION CtxQueryDefaultDisposition(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in CFIX_EVENT_TYPE EventType
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( EventType );
	CFIX_ASSERT( !"Do not call me" );
	return CfixAbort;
}

static CFIX_REPORT_DISPOSITION CtxReportEvent(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	WCHAR Buffer[ 200 ];

	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );

	Ctx->ReportEventCalls++;
	Ctx->Events[ Event->Type ]++;

	switch ( Event->Type )
	{
	case CfixEventLog:
		( VOID ) StringCchPrintf( 
			Buffer,
			_countof( Buffer ),
			L"Log: %s\n", 
			Event->Info.Log.Message );
		OutputDebugString( Buffer );
		break;

	case CfixEventInconclusiveness:
		( VOID ) StringCchPrintf( 
			Buffer,
			_countof( Buffer ),
			L"Inconclusive: %s\n", 
			Event->Info.Inconclusiveness.Message );
		OutputDebugString( Buffer );
		break;

	case CfixEventFailedAssertion:
		( VOID ) StringCchPrintf( 
			Buffer,
			_countof( Buffer ),
			L"FailedAssertion: %s\n", 
			Event->Info.FailedAssertion.Expression );
		OutputDebugString( Buffer );
		break;

	case CfixEventUncaughtException:
		TEST( Event->Info.UncaughtException.ExceptionRecord.ExceptionCode == 'excp' );
		
		( VOID ) StringCchPrintf( 
			Buffer,
			_countof( Buffer ),
			L"UncaughtException: %x\n", 
			Event->Info.UncaughtException.ExceptionRecord.ExceptionCode );
		OutputDebugString( Buffer );
		break;
	}

	return Ctx->Disp;
}

static HRESULT CtxBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_FIXTURE Fixture
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Fixture );
	
	Ctx->BeforeFixtureStartCalls++;

	return S_OK;
}

static VOID CtxAfterFixtureFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_FIXTURE Fixture,
	__in BOOL RanToCompletion
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Fixture );
	UNREFERENCED_PARAMETER( RanToCompletion );
	
	Ctx->AfterFixtureFinishCalls++;

	Ctx->FixtureRanToCompletion = RanToCompletion;
}

static HRESULT CtxBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TEST_CASE TestCase
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( TestCase );
	
	Ctx->BeforeTestCaseStartCalls++;

	return S_OK;
}

static VOID CtxAfterTestCaseFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TEST_CASE TestCase,
	__in BOOL RanToCompletion
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( TestCase );
	UNREFERENCED_PARAMETER( RanToCompletion );
	
	Ctx->AfterTestCaseFinishCalls++;
	Ctx->CaseRanToCompletion = RanToCompletion;
}

static HRESULT CtxCreateChildThread(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PCFIX_THREAD_ID ThreadId, 
	__out PVOID *Context
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Context );
	return S_OK;
}

static VOID CtxBeforeChildThreadStart(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PCFIX_THREAD_ID ThreadId, 
	__out PVOID *Context
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Context );
}

static VOID CtxAfterChildThreadFinish(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PCFIX_THREAD_ID ThreadId, 
	__out PVOID *Context
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Context );
}

static VOID CtxOnUnhandledException(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PEXCEPTION_POINTERS ExcpPointers
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	
	CFIX_ASSERT( Ctx->ExpectedMainThreadId == ThreadId->MainThreadId );
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( ExcpPointers );
	Ctx->OnUnhandledExceptionCalls++;
}

static VOID CtxReference(
	__in struct _CFIX_EXECUTION_CONTEXT *This
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	Ctx->RefCount++;
}

static VOID CtxDereference(
	__in struct _CFIX_EXECUTION_CONTEXT *This
	)
{
	PTEST_EXECUTUTION_CONTEXT Ctx = ( PTEST_EXECUTUTION_CONTEXT ) This;
	Ctx->RefCount--;
}

#define TEXT_EXECUTION_CONTEXT_INITIALIZER								\
	{																	\
		CFIX_TEST_CONTEXT_VERSION,										\
		CtxReportEvent,													\
		CtxQueryDefaultDisposition,										\
		CtxBeforeFixtureStart,											\
		CtxAfterFixtureFinish,											\
		CtxBeforeTestCaseStart,											\
		CtxAfterTestCaseFinish,											\
		CtxCreateChildThread,											\
		CtxBeforeChildThreadStart,										\
		CtxAfterChildThreadFinish,										\
		CtxOnUnhandledException,										\
		CtxReference,													\
		CtxDereference													\
	}

//----------------------------------------------------------------------

static BOOLEAN GetFixture( 
	__in PCWSTR ModuleName,
	__in PCWSTR FixtureName,
	__out PCFIX_TEST_MODULE *Module,
	__out PCFIX_FIXTURE *Fixture
	)
{
	PCFIX_TEST_MODULE Mod;
	UINT Index;
	WCHAR Path[ MAX_PATH ];

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, ModuleName ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );
	for ( Index = 0; Index < Mod->FixtureCount; Index++ )
	{
		if ( 0 == wcscmp( Mod->Fixtures[ Index ]->Name, FixtureName ) )
		{
			*Module = Mod;
			*Fixture = Mod->Fixtures[ Index ];
			
			return TRUE;
		}
	}

	Mod->Routines.Dereference( Mod );
	return FALSE;
}

static void CreateSequenceActionForFixture(
	__in PCFIX_FIXTURE Fixture,
	__in ULONG RepeatCount,
	__out PCFIX_ACTION *Action
	)
{
	PCFIX_ACTION SequenceAction;
	PCFIX_ACTION TsexecAction;
	UINT Added = 0;
	
	TEST_HR( CfixCreateSequenceAction( &SequenceAction ) );

	TEST_HR( CfixCreateFixtureExecutionAction(
		Fixture,
		CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_RUN_ON_SETUP_FAILURE,
		( ULONG ) -1, // All tests.
		&TsexecAction ) );

	for ( Added = 0; Added < RepeatCount; Added++ )
	{
		TEST_HR( CfixAddEntrySequenceAction(
			SequenceAction,
			TsexecAction ) );
	}

	TsexecAction->Reference( TsexecAction );
	TsexecAction->Dereference( TsexecAction );
	TsexecAction->Dereference( TsexecAction );

	*Action = SequenceAction;
}

/*----------------------------------------------------------------------
 * SequenceActionEventHandling
 */

static void TestSequenceOfSetupAndTearDown()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"JustSetupAndTearDown", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixBreak;

		TEST_HR( Action->Run( Action, &Ctx.Base ) );

		TEST( Ctx.ReportEventCalls						== 0 * Runs );
		TEST( Ctx.BeforeFixtureStartCalls				== 1 * Runs );
		TEST( Ctx.AfterFixtureFinishCalls				== 1 * Runs );
		TEST( Ctx.BeforeTestCaseStartCalls				== 0 * Runs );
		TEST( Ctx.AfterTestCaseFinishCalls				== 0 * Runs );
		TEST( Ctx.OnUnhandledExceptionCalls				== 0 * Runs );

		if ( Runs > 0 )
		{
			TEST( Ctx.FixtureRanToCompletion );
			TEST( ! Ctx.CaseRanToCompletion );
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}

static void TestSequenceOfSetupSuccessfulTestsAndTearDown()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"SetupTwoSuccTestsAndTearDown", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixBreak;

		TEST_HR( Action->Run( Action, &Ctx.Base ) );

		TEST( Ctx.ReportEventCalls						== 2 * Runs );
		TEST( Ctx.BeforeFixtureStartCalls				== 1 * Runs );
		TEST( Ctx.AfterFixtureFinishCalls				== 1 * Runs );
		TEST( Ctx.BeforeTestCaseStartCalls				== 2 * Runs );
		TEST( Ctx.AfterTestCaseFinishCalls				== 2 * Runs );
		TEST( Ctx.OnUnhandledExceptionCalls				== 0 * Runs );
		
		TEST( Ctx.Events[ CfixEventLog ]				== 2 * Runs );

		if ( Runs > 0 )
		{
			TEST( Ctx.FixtureRanToCompletion );
			TEST( Ctx.CaseRanToCompletion );
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}

static void TestSequenceOfSetupSucFailInconcThrowingTestsAndTearDown()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"SetupSucFailInconThrowTearDown", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixContinue;

		TEST_HR( Action->Run( Action, &Ctx.Base ) );

		TEST( Ctx.Events[ CfixEventLog ]				== 3 * Runs );
		TEST( Ctx.ReportEventCalls						== 6 * Runs );
		TEST( Ctx.BeforeFixtureStartCalls				== 1 * Runs );
		TEST( Ctx.AfterFixtureFinishCalls				== 1 * Runs );
		TEST( Ctx.BeforeTestCaseStartCalls				== 4 * Runs );
		TEST( Ctx.AfterTestCaseFinishCalls				== 4 * Runs );
		TEST( Ctx.OnUnhandledExceptionCalls				== 1 * Runs );
												 
		TEST( Ctx.Events[ CfixEventInconclusiveness ]	== 1 * Runs );
		TEST( Ctx.Events[ CfixEventFailedAssertion ]	== 1 * Runs );
		TEST( Ctx.Events[ CfixEventUncaughtException ]	== 1 * Runs );

		if ( Runs > 0 )
		{
			TEST( Ctx.FixtureRanToCompletion );
			TEST( ! Ctx.CaseRanToCompletion );	// Throw failed.
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}

static void TestSequenceOfTestsFailingWithUnhandledException()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"AbortMeBecauseOfUnhandledExcp", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		UINT EffectiveRuns = Runs > 0 ? 1 : 0;
		HRESULT Hr;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixAbort;
		
		Hr = Action->Run( Action, &Ctx.Base );
		TEST( Hr == ( Runs > 0 ? CFIX_E_TESTRUN_ABORTED : S_OK ) );

		//
		// Note: 1st run should abort, so at most 1 run must have been
		// performed (see EffectiveRuns).
		//
		TEST( Ctx.ReportEventCalls						== 1 * EffectiveRuns );
		TEST( Ctx.BeforeFixtureStartCalls				== 1 * EffectiveRuns );
		TEST( Ctx.AfterFixtureFinishCalls				== 1 * EffectiveRuns );
		TEST( Ctx.BeforeTestCaseStartCalls				== 1 * EffectiveRuns );
		TEST( Ctx.AfterTestCaseFinishCalls				== 1 * EffectiveRuns );
		TEST( Ctx.OnUnhandledExceptionCalls				== 1 * EffectiveRuns );
															   
		TEST( Ctx.Events[ CfixEventUncaughtException ]	== 1 * EffectiveRuns );

		if ( Runs > 0 )
		{
			TEST( ! Ctx.FixtureRanToCompletion );
			TEST( ! Ctx.CaseRanToCompletion );
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}

static void TestSequenceOfTestsFailingWithAssertions()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"AbortMeBecauseOfFailure", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		HRESULT Hr;
		UINT EffectiveRuns = Runs > 0 ? 1 : 0;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixAbort;

		Hr = Action->Run( Action, &Ctx.Base );
		TEST( Hr == ( Runs > 0 ? CFIX_E_TESTRUN_ABORTED : S_OK ) );

		//
		// Note: 1st run should abort, so at most 1 run must have been
		// performed (see EffectiveRuns).
		//
		TEST( Ctx.ReportEventCalls						== 2 * EffectiveRuns );
		TEST( Ctx.BeforeFixtureStartCalls				== 1 * EffectiveRuns );
		TEST( Ctx.AfterFixtureFinishCalls				== 1 * EffectiveRuns );
		TEST( Ctx.BeforeTestCaseStartCalls				== 1 * EffectiveRuns );
		TEST( Ctx.AfterTestCaseFinishCalls				== 1 * EffectiveRuns );
		TEST( Ctx.OnUnhandledExceptionCalls				== 0 * EffectiveRuns );
															 
		TEST( Ctx.Events[ CfixEventFailedAssertion ]	== 1 * EffectiveRuns );

		if ( Runs > 0 )
		{
			TEST( ! Ctx.FixtureRanToCompletion );
			TEST( ! Ctx.CaseRanToCompletion );
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}

static void TestSequenceOfFailingTeardown()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"FailTeardown", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixContinue;

		TEST_HR( Action->Run( Action, &Ctx.Base ) );

		TEST( Ctx.ReportEventCalls							== 1 * Runs );
		TEST( Ctx.BeforeFixtureStartCalls					== 1 * Runs );
		TEST( Ctx.AfterFixtureFinishCalls					== 1 * Runs );
		TEST( Ctx.BeforeTestCaseStartCalls					== 0 * Runs );
		TEST( Ctx.AfterTestCaseFinishCalls					== 0 * Runs );
		TEST( Ctx.OnUnhandledExceptionCalls					== 1 * Runs );
		
		TEST( Ctx.Events[ CfixEventFailedAssertion   ]		== 0 * Runs );
		TEST( Ctx.Events[ CfixEventUncaughtException ]		== 1 * Runs );

		if ( Runs > 0 )
		{
			TEST( ! Ctx.FixtureRanToCompletion );
			TEST( ! Ctx.CaseRanToCompletion );
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}


static void TestSequenceOfFailingSetupLeadsToTeardownsBeingSkipped()
{
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;
	ULONG Runs;

	if ( IsDebuggerPresent() )
	{
		CFIX_INCONCLUSIVE( L"Cannot be run in debugger" );
	}

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"FailSetupDoNotCallTeardown", 
		&Module, 
		&Fixture ) );

	for ( Runs = 0; Runs < 3; Runs++ )
	{
		TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;

		//
		// Create and run sequence with [Runs] entries.
		//
		PCFIX_ACTION Action;

		HRESULT ExpectedHr;

		CreateSequenceActionForFixture( Fixture, Runs, &Action );

		Ctx.ExpectedMainThreadId = GetCurrentThreadId();
		Ctx.Disp = CfixBreak;

		ExpectedHr = Runs > 0 ? CFIX_E_SETUP_ROUTINE_FAILED : S_OK;

		TEST_RETURN( ExpectedHr, Action->Run( Action, &Ctx.Base ) );

		if ( Runs > 0 )
		{
			TEST( Ctx.Events[ CfixEventFailedAssertion ]	== 1 );

			TEST( Ctx.ReportEventCalls						== 1 );
			TEST( Ctx.BeforeFixtureStartCalls				== 1 );
			TEST( Ctx.AfterFixtureFinishCalls				== 1 );
		}
		else
		{
			TEST( Ctx.Events[ CfixEventFailedAssertion ]	== 0 );

			TEST( Ctx.ReportEventCalls						== 0 );
			TEST( Ctx.BeforeFixtureStartCalls				== 0 );
			TEST( Ctx.AfterFixtureFinishCalls				== 0 );
		}

		TEST( Ctx.BeforeTestCaseStartCalls					== 0 );
		TEST( Ctx.AfterTestCaseFinishCalls					== 0 );
		TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	
		if ( Runs > 0 )
		{
			TEST( ! Ctx.FixtureRanToCompletion );
			TEST( ! Ctx.CaseRanToCompletion );
		}

		Action->Reference( Action );
		Action->Dereference( Action );
		Action->Dereference( Action );

		TEST( Ctx.RefCount == 0 );
	}

	Module->Routines.Dereference( Module );
}

/*----------------------------------------------------------------------
 * BasicEventHandling
 */

static void TestEqualsReportedAndRunCompleted()
{
	PCFIX_ACTION Action;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"TestValidEquals", 
		&Module, 
		&Fixture ) );

	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixContinue;

	TEST_HR( Action->Run( Action, &Ctx.Base ) );

	TEST( Ctx.ReportEventCalls						== 0 );
	TEST( Ctx.BeforeFixtureStartCalls				== 1 );
	TEST( Ctx.AfterFixtureFinishCalls				== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls				== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls				== 1 );
	TEST( Ctx.OnUnhandledExceptionCalls				== 0 );
	
	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestFailingEqualsReportedAndRunCompleted()
{
	PCFIX_ACTION Action;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"TestInvalidEquals", 
		&Module, 
		&Fixture ) );

	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixContinue;

	TEST_HR( Action->Run( Action, &Ctx.Base ) );

	TEST( Ctx.ReportEventCalls						== 1 );
	TEST( Ctx.BeforeFixtureStartCalls				== 1 );
	TEST( Ctx.AfterFixtureFinishCalls				== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls				== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls				== 1 );
	TEST( Ctx.OnUnhandledExceptionCalls				== 0 );
	
	TEST( Ctx.Events[ CfixEventFailedAssertion ]	== 1 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestCppAssertionsReported()
{
	PCFIX_ACTION Action;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"TestCpp", 
		&Module, 
		&Fixture ) );

	//
	// Create and run sequence with [Runs] entries.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixContinue;

	TEST_HR( Action->Run( Action, &Ctx.Base ) );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );
	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	
	TEST( Ctx.Events[ CfixEventFailedAssertion   ]		== 1 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

/*----------------------------------------------------------------------
 * RegisteredThreadEventHandling.
 */

static void TestAbortAfterFailingAssertOnRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"AssertOnRegisteredThread", 
		&Module, 
		&Fixture ) );

	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestLogTwiceOnRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"LogOnRegisteredThread", 
		&Module, 
		&Fixture ) );

	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 2 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 2 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestAbortAfterInconclusiveOnRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"InconclusiveOnRegisteredThread", 
		&Module, 
		&Fixture ) );

	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 0 );	// !
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestAbortAfterUnhandledExceptionOnRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"ThrowOnRegisteredThread", 
		&Module, 
		&Fixture ) );

	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 1 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 0 );	// !
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 1 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}


/*----------------------------------------------------------------------
 * AutoRegisteredThreadEventHandling.
 */

static void TestAbortAfterFailingAssertOnAutoRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"AssertOnAnonymousThreadWithAnonThreadSupport", 
		&Module, 
		&Fixture ) );

	//
	// Allow anonymous threads.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestBreakAfterFailingAssertOnAutoRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	if ( IsDebuggerPresent() )
	{
		CFIX_INCONCLUSIVE( L"Cannot be run in debugger" );
	}

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"AssertOnAnonymousThreadWithAnonThreadSupport", 
		&Module, 
		&Fixture ) );

	//
	// Allow anonymous threads.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixBreak;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestLogTwiceOnAutoRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"LogOnAnonymousThreadWithAnonThreadSupport", 
		&Module, 
		&Fixture ) );

	//
	// Allow anonymous threads.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 2 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 2 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestAbortAfterInconclusiveOnAutoRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"InconclusiveOnAnonymousThreadWithAnonThreadSupport", 
		&Module, 
		&Fixture ) );

	//
	// Allow anonymous threads.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestAutoJoinOfAfterFailingAssertOnAutoRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"AssertOnAnonymousThreadWithoutJoin", 
		&Module, 
		&Fixture ) );

	//
	// Allow anonymous threads.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

static void TestAutoJoinAfterInconclusiveOnAutoRegisteredThread()
{
	PCFIX_ACTION Action;
	HRESULT Hr;
	TEST_EXECUTUTION_CONTEXT Ctx = TEXT_EXECUTION_CONTEXT_INITIALIZER;
	PCFIX_FIXTURE Fixture;
	PCFIX_TEST_MODULE Module;

	CFIX_ASSERT( GetFixture( 
		L"testlib6.dll", 
		L"InconclusiveOnAnonymousThreadWithoutJoin", 
		&Module, 
		&Fixture ) );

	//
	// Allow anonymous threads.
	//
	CreateSequenceActionForFixture( Fixture, 1, &Action );

	Ctx.ExpectedMainThreadId = GetCurrentThreadId();
	Ctx.Disp = CfixAbort;

	Hr = Action->Run( Action, &Ctx.Base );

	//
	// N.B. Only the failing thread, not the testcase is aborted.
	//
	TEST( Hr == S_OK );

	TEST( Ctx.ReportEventCalls							== 1 );
	TEST( Ctx.BeforeFixtureStartCalls					== 1 );
	TEST( Ctx.AfterFixtureFinishCalls					== 1 );
	TEST( Ctx.BeforeTestCaseStartCalls					== 1 );
	TEST( Ctx.AfterTestCaseFinishCalls					== 1 );

	TEST( Ctx.OnUnhandledExceptionCalls					== 0 );
	TEST( Ctx.Events[ CfixEventFailedAssertion	 ]		== 0 );
	TEST( Ctx.Events[ CfixEventUncaughtException ]		== 0 );
	TEST( Ctx.Events[ CfixEventInconclusiveness	 ]		== 1 );
	TEST( Ctx.Events[ CfixEventLog				 ]		== 0 );

	TEST( Ctx.FixtureRanToCompletion );
	TEST( Ctx.CaseRanToCompletion );

	Action->Reference( Action );
	Action->Dereference( Action );
	Action->Dereference( Action );

	TEST( Ctx.RefCount == 0 );

	Module->Routines.Dereference( Module );
}

CFIX_BEGIN_FIXTURE(SequenceActionEventHandling)
	CFIX_FIXTURE_ENTRY(TestSequenceOfSetupAndTearDown)
	CFIX_FIXTURE_ENTRY(TestSequenceOfSetupSuccessfulTestsAndTearDown)
	CFIX_FIXTURE_ENTRY(TestSequenceOfSetupSucFailInconcThrowingTestsAndTearDown)
	CFIX_FIXTURE_ENTRY(TestSequenceOfTestsFailingWithUnhandledException)
	CFIX_FIXTURE_ENTRY(TestSequenceOfTestsFailingWithAssertions)
	CFIX_FIXTURE_ENTRY(TestSequenceOfFailingTeardown)
	CFIX_FIXTURE_ENTRY(TestSequenceOfFailingSetupLeadsToTeardownsBeingSkipped)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(BasicEventHandling)
	CFIX_FIXTURE_ENTRY(TestEqualsReportedAndRunCompleted)
	CFIX_FIXTURE_ENTRY(TestFailingEqualsReportedAndRunCompleted)
	CFIX_FIXTURE_ENTRY(TestCppAssertionsReported)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(RegisteredThreadEventHandling)
	CFIX_FIXTURE_ENTRY(TestAbortAfterFailingAssertOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestLogTwiceOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestAbortAfterInconclusiveOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestAbortAfterUnhandledExceptionOnRegisteredThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(AutoRegisteredThreadEventHandling)
	CFIX_FIXTURE_ENTRY(TestAbortAfterFailingAssertOnAutoRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestBreakAfterFailingAssertOnAutoRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestLogTwiceOnAutoRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestAbortAfterInconclusiveOnAutoRegisteredThread)

	CFIX_FIXTURE_ENTRY(TestAutoJoinOfAfterFailingAssertOnAutoRegisteredThread)
	CFIX_FIXTURE_ENTRY(TestAutoJoinAfterInconclusiveOnAutoRegisteredThread)
CFIX_END_FIXTURE()