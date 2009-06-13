/*----------------------------------------------------------------------
 * Purpose:
 *		Execution Context.
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
#include "cfixrunp.h"
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

//
// Per thread information. Used to track whether tests succeed or fail.
//
typedef struct _EXEC_STATE
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
} EXEC_STATE, *PEXEC_STATE;

typedef struct _EXEC_CONTEXT
{
	CFIX_EXECUTION_CONTEXT Base;

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
	PEXEC_STATE State = ( PEXEC_STATE ) 
		TlsGetValue( CfixrunsCurrentExecutionStateSlot );

	if ( State && 0 == InterlockedDecrement( &State->ReferenceCount ) )
	{
		TlsSetValue( CfixrunsCurrentExecutionStateSlot, NULL );
		free( State );
	}
}

static VOID CfixrunsReferenceExecutionState(
	__in PEXEC_STATE State 
	)
{
	InterlockedIncrement( &State->ReferenceCount );
}

static VOID CfixrunsSetCurrentExecutionState(
	__in PEXEC_STATE State 
	)
{
	TlsSetValue( CfixrunsCurrentExecutionStateSlot, State );
}

static PEXEC_STATE CfixrunsGetCurrentExecutionState(
	__in BOOL Create
	)
{
	PEXEC_STATE State = ( PEXEC_STATE ) 
		TlsGetValue( CfixrunsCurrentExecutionStateSlot );

	if ( ! State && Create )
	{
		State = malloc( sizeof( EXEC_STATE ) );
		if ( State )
		{
			ZeroMemory( State, sizeof( EXEC_STATE ) );
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
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

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

	ASSERT( CurrentState->CurrentTestCase );
	ASSERT( CurrentState->CurrentFixture );

	switch ( Event->Type )
	{
	case CfixEventFailedAssertion:
		CfixrunpOutputTestEvent(
			Context->State->ProgressSession,
			CfixrunTestFailure,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Name 
				: NULL,
			CurrentState->CurrentTestCase 
				? CurrentState->CurrentTestCase->Name 
				: NULL,
			Event->Info.FailedAssertion.Expression,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Name 
				: NULL,
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
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Name 
				: NULL,
			CurrentState->CurrentTestCase
				? CurrentState->CurrentTestCase->Name 
				: NULL,
			Buffer,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Name 
				: NULL,
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
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Name 
				: NULL,
			CurrentState->CurrentTestCase
				? CurrentState->CurrentTestCase->Name 
				: NULL,
			Event->Info.Inconclusiveness.Message,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Name 
				: NULL,
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
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Name 
				: NULL,
			CurrentState->CurrentTestCase
				? CurrentState->CurrentTestCase->Name 
				: NULL,
			Event->Info.Log.Message,
			CurrentState->CurrentFixture
				? CurrentState->CurrentFixture->Module->Name 
				: NULL,
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
		Event->Type );
}

//
// BEFORE
//
static VOID CfixrunsExecCtxBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_FIXTURE Fixture
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_STATE CurrentState = CfixrunsGetCurrentExecutionState( TRUE );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to create current execution context state." );
		return;
	}

	CurrentState->CurrentFixture = Fixture;
}

static VOID CfixrunsExecCtxBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_TEST_CASE TestCase
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

	ASSERT( CurrentState );
	if ( ! CurrentState )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to create current execution context state." );
		return;
	}

	CurrentState->CurrentTestCase = TestCase;
}

//
// AFTER
//
static VOID CfixrunsExecCtxAfterFixtureFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_FIXTURE Fixture,
	__in BOOL RanToCompletion
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

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
	__in PCFIX_TEST_CASE TestCase,
	__in BOOL RanToCompletion
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );

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

typedef struct _EXEC_THREAD_START_PARAMETERS
{
	PEXEC_STATE Parent;
	PTHREAD_START_ROUTINE StartAddress;
	PVOID UserParameter;
} EXEC_THREAD_START_PARAMETERS, *PEXEC_THREAD_START_PARAMETERS;

static DWORD CfixrunsExecCtxThreadStart( PVOID PvParams )
{	
	PEXEC_THREAD_START_PARAMETERS Params =
		( PEXEC_THREAD_START_PARAMETERS ) PvParams;
	DWORD ExitCode;

	CfixrunsSetCurrentExecutionState( Params->Parent );
	ExitCode = ( Params->StartAddress )( Params->UserParameter );
	CfixrunsDereferenceCurrentExecutionState();

	free( Params );
	return ExitCode;
}

static HANDLE CfixrunsExecCtxCreateChildThread(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PSECURITY_ATTRIBUTES ThreadAttributes,
	__in SIZE_T StackSize,
	__in PTHREAD_START_ROUTINE StartAddress,
	__in PVOID UserParameter,
	__in DWORD CreationFlags,
	__in PDWORD ThreadId
	)
{
	PEXEC_CONTEXT Context = ( PEXEC_CONTEXT ) This;
	PEXEC_STATE CurrentState = CfixrunsGetCurrentExecutionState( FALSE );
	HANDLE Thread;
	PEXEC_THREAD_START_PARAMETERS Params;

	Params = malloc( sizeof( EXEC_THREAD_START_PARAMETERS ) );
	if ( ! Params )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

	ASSERT( CurrentState && CurrentState->CurrentTestCase );
	if ( ! CurrentState || ! CurrentState->CurrentTestCase )
	{
		CfixrunpOutputLogMessage(
			Context->State->LogSession,
			CdiagFatalSeverity,
			L"Unable to obtain current execution context state/test case." );
		SetLastError( ( DWORD ) CFIX_E_UNKNOWN_THREAD );
		return NULL;
	}
	
	//
	// AddRef state as it will be shared by 2 threads.
	//
	CfixrunsReferenceExecutionState( CurrentState );

	Params->Parent			= CurrentState;
	Params->StartAddress		= StartAddress;
	Params->UserParameter	= UserParameter;

	Thread = CreateThread(
		ThreadAttributes,
		StackSize,
		CfixrunsExecCtxThreadStart,
		Params,
		CreationFlags,
		ThreadId );

	if ( NULL == Thread )
	{
		DWORD LastErr = GetLastError();
		CfixrunsDereferenceCurrentExecutionState();
		SetLastError( LastErr );
	}

	return Thread;
}

static VOID CfixrunsExecCtxOnUnhandledException(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PEXCEPTION_POINTERS ExcpPointers
	)
{
	UNREFERENCED_PARAMETER( This );
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
	NewContext->Base.Version				= CFIX_TEST_CONTEXT_VERSION;
	NewContext->Base.ReportEvent			= CfixrunsExecCtxReportEvent;
	NewContext->Base.QueryDefaultDisposition= CfixrunsExecCtxQueryDefaultDisposition;
	NewContext->Base.BeforeFixtureStart		= CfixrunsExecCtxBeforeFixtureStart;
	NewContext->Base.AfterFixtureFinish		= CfixrunsExecCtxAfterFixtureFinish;
	NewContext->Base.BeforeTestCaseStart	= CfixrunsExecCtxBeforeTestCaseStart;
	NewContext->Base.AfterTestCaseFinish	= CfixrunsExecCtxAfterTestCaseFinish;
	NewContext->Base.CreateChildThread		= CfixrunsExecCtxCreateChildThread;
	NewContext->Base.OnUnhandledException	= CfixrunsExecCtxOnUnhandledException;

	*Context = &NewContext->Base;

	return S_OK;
}

VOID CfixrunpDeleteExecutionContext(
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