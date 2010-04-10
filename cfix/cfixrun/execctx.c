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
	
	//
	// States are shared among threads if a testcase spawns child
	// threads - thus it is reference counted.
	//
	LONG ReferenceCount;
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
	__in CFIX_EVENT_TYPE EventType
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;

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
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( ThreadId );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		Context->State->Options->PrintConsole(
			L"Unable to create current execution context state. Aborting" );
		return CfixAbort;
	}

	return CfixrunsExecCtxQueryDefaultDisposition(
		This,
		Event->Type );
}

//
// BEFORE
//
static HRESULT CfixrunsExecCtxBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_FIXTURE Fixture
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( TRUE );

	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Fixture );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		Context->State->Options->PrintConsole(
			L"Unable to create current execution context state." );
		return E_UNEXPECTED;
	}

	return S_OK;
}

static HRESULT CfixrunsExecCtxBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TEST_CASE TestCase
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( TestCase );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		Context->State->Options->PrintConsole(
			L"Unable to create current execution context state." );
		return E_UNEXPECTED;
	}

	return S_OK;
}

//
// AFTER
//
static VOID CfixrunsExecCtxAfterFixtureFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_FIXTURE Fixture,
	__in BOOL RanToCompletion
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( Fixture );
	UNREFERENCED_PARAMETER( RanToCompletion );
	ASSERT( CurrentState );

	if ( ! CurrentState )
	{
		Context->State->Options->PrintConsole(
			L"Unable to obtain current execution context state." );
		return;
	}

	InterlockedIncrement( &Context->Statistics.Fixtures );

	CfixrunsDereferenceCurrentExecutionState();
}


static VOID CfixrunsExecCtxAfterTestCaseFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TEST_CASE TestCase,
	__in BOOL RanToCompletion
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( TestCase );
	UNREFERENCED_PARAMETER( RanToCompletion );
	ASSERT( CurrentState );

	if ( ! CurrentState )
	{
		Context->State->Options->PrintConsole(
			L"Unable to obtain current execution context state." );
		return;
	}

	//
	// Was is a success or failure?
	//
	if ( CurrentState->FailureCount == 0 && CurrentState->InconclusiveCount == 0 )
	{
		//
		// Success.
		//
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
}

VOID CfixrunsExecCtxBeforeChildThreadStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in_opt PVOID Context
	)
{
	PEXEC_THREAD_STATE ParentState = ( PEXEC_THREAD_STATE ) Context;

	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( This );
	ASSERT( CfixIsValidContext( This ) );

	ASSERT( ParentState != NULL );
	__assume( ParentState != NULL );
	
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
	__in PCFIX_THREAD_ID ThreadId, 
	__in_opt PVOID Context
	)
{
	UNREFERENCED_PARAMETER( ThreadId );
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Context );

	CfixrunsDereferenceCurrentExecutionState();
}

static HRESULT CfixrunsExecCtxCreateChildThread(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PCFIX_THREAD_ID ThreadId, 
	__out PVOID *ContextForChild
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_THREAD_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	UNREFERENCED_PARAMETER( ThreadId );
	ASSERT( ContextForChild );
	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		Context->State->Options->PrintConsole(
			L"Unable to obtain current execution context state/test case." );
		return CFIX_E_UNKNOWN_THREAD;
	}
	
	*ContextForChild = CurrentState;
	return S_OK;
}

static VOID CfixrunsExecCtxOnUnhandledException(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PEXCEPTION_POINTERS ExcpPointers
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( ThreadId );
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