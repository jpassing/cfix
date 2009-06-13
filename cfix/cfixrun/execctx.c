/*----------------------------------------------------------------------
 * Purpose:
 *		Execution Context.
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
#include "cfixrunp.h"
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

//
// N.B. As of CFIX_EXECUTION_CONTEXT 1.1, some of the thread logic
// implemented here is unnecessary in case there is only a single
// main thread.
//

//
// Per thread information. Used to track whether tests succeed or fail.
//
typedef struct _EXEC_THREAD_STATE
{
	//
	// Counters for current test case - reset for each test case.
	//
	LONG FailureCount;
	LONG InconclusiveCount;
	
	PCFIX_TEST_CASE CurrentTestCase;
	PCFIX_FIXTURE CurrentFixture;

	//
	// States are shared among threads if a testcase spawns child
	// threads - thus it is reference counted.
	//
	LONG ReferenceCount;

	BOOL FirstTestCaseBegun;
} EXEC_THREAD_STATE, *PEXEC_THREAD_STATE;

typedef struct _EXEC_CONTEXT
{
	CFIX_EXECUTION_CONTEXT Base;

	volatile LONG ReferenceCount;
	PCFIXRUN_STATE State;

	CFIXRUN_STATISTICS Statistics;
} EXEC_CONTEXT, *PEXEC_CONTEXT;

static DWORD CfixrunsCurrentExecutionStateSlot = TLS_OUT_OF_INDEXES;
static DWORD CfixrunsCurrentExecutionStateSlotUsageCount = 0;

/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */

static VOID CfixrunsDereferenceCurrentExecutionState()
{
	PEXEC_THREAD_STATE State = ( PEXEC_THREAD_STATE ) 
		TlsGetValue( CfixrunsCurrentExecutionStateSlot );

	if ( State && 0 == InterlockedDecrement( &State->ReferenceCount ) )
	{
		TlsSetValue( CfixrunsCurrentExecutionStateSlot, NULL );
		free( State );
	}
}

static VOID CfixrunsReferenceExecutionState(
	__in PEXEC_THREAD_STATE State 
	)
{
	InterlockedIncrement( &State->ReferenceCount );
}

static VOID CfixrunsSetCurrentExecutionState(
	__in PEXEC_THREAD_STATE State 
	)
{
	TlsSetValue( CfixrunsCurrentExecutionStateSlot, State );
}

static PEXEC_THREAD_STATE CfixrunsGetCurrentExecutionState(
	__in BOOL Create
	)
{
	PEXEC_THREAD_STATE State = ( PEXEC_THREAD_STATE ) 
		TlsGetValue( CfixrunsCurrentExecutionStateSlot );

	if ( ! State && Create )
	{
		State = malloc( sizeof( EXEC_THREAD_STATE ) );
		if ( State )
		{
			ZeroMemory( State, sizeof( EXEC_THREAD_STATE ) );
			State->ReferenceCount = 1;
			CfixrunsSetCurrentExecutionState( State );
		}
	}

	return State;
}

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */

static VOID CfixrunsDeleteExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	if ( Context )
	{
		free( Context );
	}

	if ( --CfixrunsCurrentExecutionStateSlotUsageCount == 0 )
	{
		TlsFree( CfixrunsCurrentExecutionStateSlot );
		CfixrunsCurrentExecutionStateSlot = TLS_OUT_OF_INDEXES;
	}
}

static VOID CfixrunsExecCtxReference(
	__in PCFIX_EXECUTION_CONTEXT This
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	InterlockedIncrement( &Context->ReferenceCount );
}

static VOID CfixrunsExecCtxDereference(
	__in PCFIX_EXECUTION_CONTEXT This
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	if ( 0 == InterlockedDecrement( &Context->ReferenceCount ) )
	{
		CfixrunsDeleteExecutionContext( This );
	}
}

static CFIX_REPORT_DISPOSITION CfixrunsExecCtxQueryDefaultDisposition(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in CFIX_EVENT_TYPE EventType
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;

	UNREFERENCED_PARAMETER( MainThreadId );

	switch ( EventType )
	{
	case CfixEventFailedAssertion:
		if ( Context->State->Options->AbortOnFirstFailure )
		{
			return CfixAbort;
		}
		else if ( Context->State->Options->AlwaysBreakOnFailure )
		{
			return CfixBreakAlways;
		}
		else
		{
			return CfixBreak;
		}

	case CfixEventUncaughtException:
		if ( Context->State->Options->DoNotCatchUnhandledExceptions )
		{
			return CfixBreak;
		}
		else
		{
			return Context->State->Options->AbortOnFirstFailure
				? CfixAbort
				: CfixContinue;
		}

	case CfixEventInconclusiveness:	
		//
		// Ignored anyway.
		//
		return CfixContinue;

	case CfixEventLog:
		//
		// Ignored anyway.
		//
		return CfixContinue;

	default:
		ASSERT( !"Unknown event type!" );
		return CfixContinue;
	}
}

static CFIX_REPORT_DISPOSITION CfixrunsExecCtxReportEvent(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	PCWSTR FixtureName;
	PCWSTR ModuleName;
	PCWSTR TestCaseName;

	WCHAR Buffer[ 256 ] = { 0 };

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to create current execution context state. Aborting" );
		return CfixAbort;
	}

	//
	// The fixture must have already been set when the first
	// report comes in.
	//
	ASSERT( CurrentState->CurrentFixture );
	if ( ! CurrentState->CurrentFixture )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Internal inconsistency detected. Aborting." );
		return CfixAbort;
	}

	FixtureName  = CurrentState->CurrentFixture->Name;
	ModuleName   = CurrentState->CurrentFixture->Module->Name;

	//
	// A testcase may not be available if Setup/Teardown is currently
	// being done.
	//
	if ( CurrentState->CurrentTestCase )
	{
		TestCaseName = CurrentState->CurrentTestCase->Name;
	}
	else if ( CurrentState->FirstTestCaseBegun )
	{
		TestCaseName = L"[Teardown]";
	}
	else
	{
		//
		// Implicit Assumption here: The suite has at least one testcase.
		//
		TestCaseName = L"[Setup]";
	}

	switch ( Event->Type )
	{
	case CfixEventFailedAssertion:
		CfixrunpOutputTestEvent(
			Context->State->ProgressSession,
			CfixrunTestFailure,
			FixtureName,
			TestCaseName,
			Event->Info.FailedAssertion.Expression,
			ModuleName,
			Event->Info.FailedAssertion.Routine,
			Event->Info.FailedAssertion.File,
			Event->Info.FailedAssertion.Line,
			Event->Info.FailedAssertion.LastError,
			&Event->StackTrace,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Routines.GetInformationStackFrame 
				: NULL );

		InterlockedIncrement( &CurrentState->FailureCount );
		break;

	case CfixEventUncaughtException:
		( VOID ) StringCchPrintf(
			Buffer,
			_countof( Buffer ),
			L"Unhandled Exception 0x%08X at %p",
			Event->Info.UncaughtException.ExceptionRecord.ExceptionCode,
			Event->Info.UncaughtException.ExceptionRecord.ExceptionAddress );

		InterlockedIncrement( &CurrentState->FailureCount );

		CfixrunpOutputTestEvent(
			Context->State->ProgressSession,
			CfixrunTestFailure,
			FixtureName,
			TestCaseName,
			Buffer,
			ModuleName,
			NULL,
			NULL,
			0,
			0,
			&Event->StackTrace,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Routines.GetInformationStackFrame 
				: NULL );
		break;

	case CfixEventInconclusiveness:	
		CfixrunpOutputTestEvent(
			Context->State->ProgressSession,
			CfixrunTestInconclusive,
			FixtureName,
			TestCaseName,
			Event->Info.Inconclusiveness.Message,
			ModuleName,
			NULL,
			NULL,
			0,
			0,
			&Event->StackTrace,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Routines.GetInformationStackFrame 
				: NULL );

		InterlockedIncrement( &CurrentState->InconclusiveCount );
		break;

	case CfixEventLog:
		CfixrunpOutputTestEvent(
			Context->State->ProgressSession,
			CfixrunLog,
			FixtureName,
			TestCaseName,
			Event->Info.Log.Message,
			ModuleName,
			NULL,
			NULL,
			0,
			0,
			&Event->StackTrace,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Routines.GetInformationStackFrame 
				: NULL );
		break;
	}

	return CfixrunsExecCtxQueryDefaultDisposition(
		This,
		MainThreadId,
		Event->Type );
}

//
// BEFORE
//
static HRESULT CfixrunsExecCtxBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in PCFIX_FIXTURE Fixture
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( TRUE );

	UNREFERENCED_PARAMETER( MainThreadId );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to create current execution context state." );
		return E_UNEXPECTED;
	}

	CurrentState->CurrentFixture = Fixture;
	return S_OK;
}

static HRESULT CfixrunsExecCtxBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in PCFIX_TEST_CASE TestCase
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( MainThreadId );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to create current execution context state." );
		return E_UNEXPECTED;
	}

	CurrentState->CurrentTestCase		= TestCase;
	CurrentState->FirstTestCaseBegun	= TRUE;

	return S_OK;
}

//
// AFTER
//
static VOID CfixrunsExecCtxAfterFixtureFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in PCFIX_FIXTURE Fixture,
	__in BOOL RanToCompletion
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( MainThreadId );
	UNREFERENCED_PARAMETER( Fixture );
	UNREFERENCED_PARAMETER( RanToCompletion );
	ASSERT( CurrentState );

	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to obtain current execution context state." );
		return;
	}

	InterlockedIncrement( &Context->Statistics.Fixtures );

	CfixrunsDereferenceCurrentExecutionState();
}


static VOID CfixrunsExecCtxAfterTestCaseFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in PCFIX_TEST_CASE TestCase,
	__in BOOL RanToCompletion
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( MainThreadId );
	UNREFERENCED_PARAMETER( TestCase );
	UNREFERENCED_PARAMETER( RanToCompletion );
	ASSERT( CurrentState );

	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to obtain current execution context state." );
		return;
	}


	//
	// Was is a success or failure?
	//
	if ( CurrentState->FailureCount == 0 && CurrentState->InconclusiveCount == 0 )
	{
		//
		// Success -> report.
		//
		CfixrunpOutputTestEvent(
			Context->State->ProgressSession,
			CfixrunTestSuccess,
			CurrentState->CurrentFixture->Name,
			CurrentState->CurrentTestCase->Name,
			NULL,
			CurrentState->CurrentFixture->Module->Name,
			NULL,
			NULL,
			0,
			0,
			NULL,
			NULL );

		InterlockedIncrement( &Context->Statistics.SucceededTestCases );
	}
	else if ( CurrentState->FailureCount > 0 )
	{
		//
		// Failure, errors have already been reported.
		//
		InterlockedIncrement( &Context->Statistics.FailedTestCases );
	}
	else if ( CurrentState->InconclusiveCount > 0 )
	{
		//
		// Failure, errors have already been reported.
		//
		InterlockedIncrement( &Context->Statistics.InconclusiveTestCases );
	}

	InterlockedIncrement( &Context->Statistics.TestCases );

	CurrentState->InconclusiveCount = 0;
	CurrentState->FailureCount		= 0;
	CurrentState->CurrentTestCase	= NULL;
}

VOID CfixrunsExecCtxBeforeChildThreadStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in_opt PVOID Context
	)
{
	PEXEC_THREAD_STATE ParentState = ( PEXEC_THREAD_STATE ) Context;

	UNREFERENCED_PARAMETER( MainThreadId );
	UNREFERENCED_PARAMETER( This );
	ASSERT( CfixIsValidContext( This ) );
	
	//
	// AddRef state as it will be shared by 2 threads.
	//
	CfixrunsReferenceExecutionState( ParentState );

	//
	// Assign state of parent to child.
	//
	CfixrunsSetCurrentExecutionState( ParentState );
}

VOID CfixrunsExecCtxAfterChildThreadFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in_opt PVOID Context
	)
{
	UNREFERENCED_PARAMETER( MainThreadId );
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Context );

	CfixrunsDereferenceCurrentExecutionState();
}

static HRESULT CfixrunsExecCtxCreateChildThread(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in ULONG MainThreadId,
	__out PVOID *ContextForChild
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( MainThreadId );
	ASSERT( ContextForChild );
	ASSERT( CurrentState && CurrentState->CurrentTestCase );
	if ( ! CurrentState || ! CurrentState->CurrentTestCase )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to obtain current execution context state/test case." );
		return CFIX_E_UNKNOWN_THREAD;
	}
	
	*ContextForChild = CurrentState;
	return S_OK;
}

static VOID CfixrunsExecCtxOnUnhandledException(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in ULONG MainThreadId,
	__in PEXCEPTION_POINTERS ExcpPointers
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( MainThreadId );
	UNREFERENCED_PARAMETER( ExcpPointers );
}

/*----------------------------------------------------------------------
 *
 *
 *
 */
HRESULT CfixrunpCreateExecutionContext(
	__in PCFIXRUN_STATE State,
	__out PCFIX_EXECUTION_CONTEXT *Context
	)
{
	PEXEC_CONTEXT NewContext;

	if ( ! State || ! Context )
	{
		return E_INVALIDARG;
	}

	if ( CfixrunsCurrentExecutionStateSlot == TLS_OUT_OF_INDEXES )
	{
		CfixrunsCurrentExecutionStateSlot = TlsAlloc();
		if ( CfixrunsCurrentExecutionStateSlot == TLS_OUT_OF_INDEXES )
		{
			return HRESULT_FROM_WIN32( CfixrunsCurrentExecutionStateSlot );
		}
	}
	CfixrunsCurrentExecutionStateSlotUsageCount++;

	NewContext = malloc( sizeof( EXEC_CONTEXT ) );
	if ( ! NewContext )
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory( NewContext, sizeof( EXEC_CONTEXT ) );

	NewContext->State						= State;
	NewContext->ReferenceCount				= 1;
	NewContext->Base.Version				= CFIX_TEST_CONTEXT_VERSION;
	NewContext->Base.ReportEvent			= CfixrunsExecCtxReportEvent;
	NewContext->Base.QueryDefaultDisposition= CfixrunsExecCtxQueryDefaultDisposition;
	NewContext->Base.BeforeFixtureStart		= CfixrunsExecCtxBeforeFixtureStart;
	NewContext->Base.AfterFixtureFinish		= CfixrunsExecCtxAfterFixtureFinish;
	NewContext->Base.BeforeTestCaseStart	= CfixrunsExecCtxBeforeTestCaseStart;
	NewContext->Base.AfterTestCaseFinish	= CfixrunsExecCtxAfterTestCaseFinish;
	NewContext->Base.CreateChildThread		= CfixrunsExecCtxCreateChildThread;
	NewContext->Base.BeforeChildThreadStart	= CfixrunsExecCtxBeforeChildThreadStart;
	NewContext->Base.AfterChildThreadFinish	= CfixrunsExecCtxAfterChildThreadFinish;
	NewContext->Base.OnUnhandledException	= CfixrunsExecCtxOnUnhandledException;
	NewContext->Base.Reference				= CfixrunsExecCtxReference;
	NewContext->Base.Dereference			= CfixrunsExecCtxDereference;

	*Context = &NewContext->Base;

	return S_OK;
}

VOID CfixrunpGetStatisticsExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT This,
	__out PCFIXRUN_STATISTICS Statistics
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;

	if ( Context && Statistics )
	{
		CopyMemory( Statistics, &Context->Statistics, sizeof( CFIXRUN_STATISTICS ) );
	}
}