/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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
#include <shlwapi.h>
#include <cfixapi.h>
#include "test.h"

/*----------------------------------------------------------------------
 *
 * Test declaration.
 *
 */
typedef struct _RUN
{
	ULONG TsExecFlags;

	//
	// Bitmask of routines that should fail.
	//
	ULONG FailingRoutines;
	CFIX_REPORT_DISPOSITION Disposition;

	//
	// Bitmask of tests that must have run.
	//
	ULONG RoutinesRun;

	HRESULT HrExpected;

	struct
	{
		ULONG BeforeFixtureCalls;
		ULONG AfterFixtureCalls;
		ULONG BeforeTestCalls;
		ULONG AfterTestCalls;
		ULONG Logs;
		ULONG FailedAssertions;
		ULONG FixturesRunToCompletion;
		ULONG TestCasesRunToCompletion;
	} Events;
} RUN, *PRUN;

#define _FSF	CFIX_FIXTURE_EXECUTION_SHORTCUT_FIXTURE_ON_FAILURE
#define _FSR	CFIX_FIXTURE_EXECUTION_SHORTCUT_RUN_ON_FAILURE
#define _FSS	CFIX_FIXTURE_EXECUTION_SHORTCUT_RUN_ON_SETUP_FAILURE

#define E_RF	CFIX_E_TEST_ROUTINE_FAILED
#define E_SRF	CFIX_E_SETUP_ROUTINE_FAILED
#define E_TRF	CFIX_E_TEARDOWN_ROUTINE_FAILED
#define E_BRF	CFIX_E_BEFORE_ROUTINE_FAILED
#define E_ARF	CFIX_E_AFTER_ROUTINE_FAILED

RUN Runs[] = 
{
	// All succeed.
	{ 0				, 0	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ 0				, 0	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSR			, 0	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSR			, 0	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSS			, 0	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSS			, 0	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSF			, 0	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSF			, 0	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSF|_FSS		, 0	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },
	{ _FSF|_FSS		, 0	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 10, 0, 3, 4 } },

	// A.setup fails.
	{ 0				, 1	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ 0				, 1	, CfixBreak		, 4093	, S_OK	, { 3, 3, 4, 4, 8, 1, 2, 4 } },
	{ _FSF			, 1	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF			, 1	, CfixBreak		, 4093	, S_OK	, { 3, 3, 4, 4, 8, 1, 2, 4 } },
	{ _FSR			, 1	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSR			, 1	, CfixBreak		, 1		, E_SRF	, { 1, 1, 0, 0, 0, 1, 0, 0 } },
	{ _FSS			, 1	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSS			, 1	, CfixBreak		, 1		, E_SRF	, { 1, 1, 0, 0, 0, 1, 0, 0 } },
	{ _FSS|_FSF		, 1	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSS|_FSF		, 1	, CfixBreak		, 1		, E_SRF	, { 1, 1, 0, 0, 0, 1, 0, 0 } },

	// A.teardown fails.
	{ 0				, 2	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ 0				, 2	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 2, 4 } },
	{ _FSS			, 2	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSS			, 2	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 2, 4 } },
	{ _FSF			, 2	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF			, 2	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 2, 4 } },
	{ _FSF|_FSS		, 2	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF|_FSS		, 2	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 2, 4 } },
	{ _FSR			, 2	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSR			, 2	, CfixBreak		, 3		, E_TRF	, { 1, 1, 0, 0, 1, 1, 0, 0 } },

	// B.Test< 8 > fails.
	{ 0				, 8	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ 0				, 8	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 3 } },
	{ _FSS			, 8	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSS			, 8	, CfixBreak		, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 3 } },
	{ _FSF			, 8	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF			, 8	, CfixBreak		, 4079	, S_OK	, { 3, 3, 3, 3, 8, 1, 2, 2 } },
	{ _FSF|_FSS		, 8	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF|_FSS		, 8	, CfixBreak		, 4079	, S_OK	, { 3, 3, 3, 3, 8, 1, 2, 2 } },
	{ _FSR			, 8	, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSR			, 8	, CfixBreak		, 47	, E_RF	, { 2, 2, 1, 1, 4, 1, 1, 0 } },

	// C.Test< 128 > fails.
	{ 0				, 128, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ 0				, 128, CfixBreak	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 3 } },
	{ _FSS			, 128, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSS			, 128, CfixBreak	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 3 } },
	{ _FSF			, 128, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF			, 128, CfixBreak	, 3839	, S_OK	, { 3, 3, 3, 3, 8, 1, 2, 2 } },
	{ _FSF|_FSS		, 128, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSF|_FSS		, 128, CfixBreak	, 3839	, S_OK	, { 3, 3, 3, 3, 8, 1, 2, 2 } },
	{ _FSR			, 128, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 9, 1, 3, 4 } },
	{ _FSR			, 128, CfixBreak	, 3839	, E_RF	, { 3, 3, 3, 3, 8, 1, 2, 2 } },
	
	// C.BeforeAfter< 512 > fails.
	{ 0				, 512, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ 0				, 512, CfixBreak	, 2687	, S_OK	, { 3, 3, 4, 4, 8,  2, 3, 2 } },
	{ _FSS			, 512, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSS			, 512, CfixBreak	, 2687	, S_OK	, { 3, 3, 4, 4, 8,  2, 3, 2 } },
	{ _FSF			, 512, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSF			, 512, CfixBreak	, 2687	, S_OK	, { 3, 3, 3, 3, 8,  1, 2, 2 } },
	{ _FSF|_FSS		, 512, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSF|_FSS		, 512, CfixBreak	, 2687	, S_OK	, { 3, 3, 3, 3, 8,  1, 2, 2 } },
	{ _FSR			, 512, CfixContinue	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSR			, 512, CfixBreak	, 2687	, E_BRF	, { 3, 3, 3, 3, 8,  1, 2, 2 } },

	// C.BeforeAfter< 1024 > fails.
	{ 0				, 1024, CfixContinue, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ 0				, 1024, CfixBreak	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 2 } },
	{ _FSS			, 1024, CfixContinue, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSS			, 1024, CfixBreak	, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 2 } },
	{ _FSF			, 1024, CfixContinue, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSF			, 1024, CfixBreak	, 3839	, S_OK	, { 3, 3, 3, 3, 9 , 1, 2, 2 } },
	{ _FSF|_FSS		, 1024, CfixContinue, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSF|_FSS		, 1024, CfixBreak	, 3839	, S_OK	, { 3, 3, 3, 3, 9 , 1, 2, 2 } },
	{ _FSR			, 1024, CfixContinue, 4095	, S_OK	, { 3, 3, 4, 4, 10, 2, 3, 4 } },
	{ _FSR			, 1024, CfixBreak	, 3839	, E_ARF	, { 3, 3, 3, 3, 9 , 1, 2, 2 } },
};

static PRUN  CurrentRun = NULL;
static ULONG ActualRoutinesRun;
static ULONG ActualBeforeFixtureCalls;
static ULONG ActualAfterFixtureCalls;
static ULONG ActualBeforeTestCalls;
static ULONG ActualAfterTestCalls;
static ULONG ActualLogs;
static ULONG ActualFailedAssertions;
static ULONG ActualFixturesRunToCompletion;
static ULONG ActualTestCasesRunToCompletion;

#include <cfixapi.h>

/*----------------------------------------------------------------------
 *
 * ExecContext.
 *
 */
static CFIX_REPORT_DISPOSITION CtxQueryDefaultDisposition(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in CFIX_EVENT_TYPE EventType
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( EventType );

	CFIX_ASSERT( !"Do not call me" );
	return CfixContinue;
}

static CFIX_REPORT_DISPOSITION CtxReportEvent(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	UNREFERENCED_PARAMETER( This );

	switch ( Event->Type )
	{
	case CfixEventLog:
		ActualLogs++;
		break;

	case CfixEventInconclusiveness:
		CFIX_ASSERT( !"Unexpedted" );
		break;

	case CfixEventFailedAssertion:
		ActualFailedAssertions++;
		break;

	case CfixEventUncaughtException:
		CFIX_ASSERT( !"Unexpedted" );
		break;

	default:
		CFIX_ASSERT( !"Unexpedted" );
	}

	return CurrentRun->Disposition;
}

static VOID CtxBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_FIXTURE Fixture
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Fixture );

	ActualBeforeFixtureCalls++;
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

	ActualAfterFixtureCalls++;

	if ( RanToCompletion )
	{
		ActualFixturesRunToCompletion++;
	}
}

static VOID CtxBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_TEST_CASE TestCase
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( TestCase );
	
	ActualBeforeTestCalls++;	
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

	ActualAfterTestCalls++;

	if ( RanToCompletion )
	{
		ActualTestCasesRunToCompletion++;
	}
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
	CFIX_ASSERT( !"Unexpedted" );
}

/*----------------------------------------------------------------------
 *
 * Communication with test routines.
 *
 */
BOOL ShouldFail( ULONG Id )
{
	if ( CurrentRun != NULL )
	{
		return ( CurrentRun->FailingRoutines & Id ) != 0;
	}
	else
	{
		return FALSE;
	}
}

VOID NotifyRunning( ULONG Id )
{
	ActualRoutinesRun |= Id;
}

/*----------------------------------------------------------------------
 *
 * Test driver.
 *
 */

#define TEST CFIX_ASSERT
#define TEST_HR( expr ) CFIX_ASSERT_EQUALS_DWORD( S_OK, ( expr ) )
#define TEST_RETURN( Expected, Expr ) \
	CFIX_ASSERT_EQUALS_DWORD( ( DWORD ) Expected, ( DWORD ) ( Expr ) )

static void TestTsExecution()
{
	CFIX_EXECUTION_CONTEXT Ctx = {
		CFIX_TEST_CONTEXT_VERSION,
		CtxReportEvent,
		CtxQueryDefaultDisposition,
		CtxBeforeFixtureStart,
		CtxAfterFixtureFinish,
		CtxBeforeTestCaseStart,
		CtxAfterTestCaseFinish,
		CtxCreateChildThread,
		CtxOnUnhandledException
	};

	HRESULT Hr;
	UINT FixIndex = 0;
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];
	UINT RunIndex = 0;
	PCFIX_ACTION SeqAction;

	//
	// Prepare run.
	//
	TEST( GetModuleFileName( GetModuleHandle( NULL ), Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testtsx.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );

	for ( RunIndex = 0; RunIndex < _countof( Runs ); RunIndex++ )
	{
		CFIX_LOG( L"Run #%d", RunIndex );

		CurrentRun = &Runs[ RunIndex ];
		ActualRoutinesRun = 0;
		ActualBeforeFixtureCalls = 0;
		ActualAfterFixtureCalls = 0;
		ActualBeforeTestCalls = 0;
		ActualAfterTestCalls = 0;
		ActualLogs = 0;
		ActualFailedAssertions = 0;
		ActualFixturesRunToCompletion = 0;
		ActualTestCasesRunToCompletion = 0;

		TEST_HR( CfixCreateSequenceAction( &SeqAction ) );
		
		for ( FixIndex = 0; FixIndex < Mod->FixtureCount; FixIndex++ )
		{
			if ( 0 != wcscmp( Mod->Fixtures[ FixIndex ]->Name, L"TestTsExecution" ) )
			{
				PCFIX_ACTION TsexecAction;
				
				TEST_HR( CfixCreateFixtureExecutionAction(
					Mod->Fixtures[ FixIndex ],
					CurrentRun->TsExecFlags,
					&TsexecAction ) );

				TEST_HR( CfixAddEntrySequenceAction(
						SeqAction,
						TsexecAction ) );

				TsexecAction->Dereference( TsexecAction );
			}
		}

		//
		// Run.
		//
		Hr = SeqAction->Run( SeqAction, &Ctx );
		TEST_RETURN( CurrentRun->HrExpected, Hr );

		SeqAction->Dereference( SeqAction );

		//
		// Check.
		//
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->RoutinesRun						, ActualRoutinesRun );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.BeforeFixtureCalls 		, ActualBeforeFixtureCalls );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.AfterFixtureCalls			, ActualAfterFixtureCalls );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.BeforeTestCalls			, ActualBeforeTestCalls );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.AfterTestCalls				, ActualAfterTestCalls );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.Logs						, ActualLogs );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.FailedAssertions			, ActualFailedAssertions );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.FixturesRunToCompletion	, ActualFixturesRunToCompletion );
		CFIX_ASSERT_EQUALS_DWORD( CurrentRun->Events.TestCasesRunToCompletion	, ActualTestCasesRunToCompletion );
	}

	Mod->Routines.Dereference( Mod );
}

CFIX_BEGIN_FIXTURE( TestTsExecution )
	CFIX_FIXTURE_ENTRY( TestTsExecution)
CFIX_END_FIXTURE()