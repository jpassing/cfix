/*----------------------------------------------------------------------
 * Purpose:
 *		Event Emitting Execution Context Proxy.
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

#define CFIXAPI

#include <cfixevnt.h>
#include "cfixp.h"
#include <stdlib.h>

typedef struct _CFIXP_EVENT_EMITTING_PROXY
{
	CFIX_EXECUTION_CONTEXT Base;
	PCFIX_EXECUTION_CONTEXT TargetExecContext;
	PCFIX_EVENT_SINK EventSink;

	BOOL FirstTestCaseBegun;
	PCFIX_TEST_CASE CurrentTestCase;
	PCFIX_FIXTURE CurrentFixture;

	volatile LONG ReferenceCount;
} CFIXP_EVENT_EMITTING_PROXY, *PCFIXP_EVENT_EMITTING_PROXY;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */

static VOID CfixsEventEmittingProxyReference(
	__in PCFIX_EXECUTION_CONTEXT This
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	InterlockedIncrement( &Context->ReferenceCount );
}

static VOID CfixsEventEmittingProxyDereference(
	__in PCFIX_EXECUTION_CONTEXT This
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	if ( 0 == InterlockedDecrement( &Context->ReferenceCount ) )
	{
		Context->TargetExecContext->Dereference( Context->TargetExecContext );
		Context->EventSink->Dereference( Context->EventSink );

		free( Context );
	}
}

static CFIX_REPORT_DISPOSITION CfixsEventEmittingProxyQueryDefaultDisposition(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in CFIX_EVENT_TYPE EventType
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	return Context->TargetExecContext->QueryDefaultDisposition( 
		Context->TargetExecContext,
		EventType );
}

static CFIX_REPORT_DISPOSITION CfixsEventEmittingProxyReportEvent(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	PCWSTR TestCaseName;

	ASSERT( Context );
	ASSERT( Context->CurrentFixture != NULL );
	__assume( Context->CurrentFixture != NULL );

	//
	// A testcase may not be available if Setup/Teardown is currently
	// being done.
	//
	if ( Context->CurrentTestCase )
	{
		TestCaseName = Context->CurrentTestCase->Name;
	}
	else if ( Context->FirstTestCaseBegun )
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

	Context->EventSink->ReportEvent(
		Context->EventSink,
		ThreadId,
		Context->CurrentFixture->Module->Name,
		Context->CurrentFixture->Name,
		TestCaseName,
		Event );

	return Context->TargetExecContext->ReportEvent(
		Context->TargetExecContext,
		ThreadId,
		Event );
}

static HRESULT CfixsEventEmittingProxyBeforeFixtureStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_FIXTURE Fixture
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->FirstTestCaseBegun = FALSE;
	Context->CurrentFixture = Fixture;

	Context->EventSink->BeforeFixtureStart(
		Context->EventSink,
		ThreadId,
		Fixture->Module->Name,
		Fixture->Name );

	return Context->TargetExecContext->BeforeFixtureStart(
		Context->TargetExecContext,
		ThreadId,
		Fixture );
}

static HRESULT CfixsEventEmittingProxyBeforeTestCaseStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TEST_CASE TestCase
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->FirstTestCaseBegun = TRUE;
	Context->CurrentTestCase = TestCase;

	Context->EventSink->BeforeTestCaseStart(
		Context->EventSink,
		ThreadId,
		TestCase->Fixture->Module->Name,
		TestCase->Fixture->Name,
		TestCase->Name );

	return Context->TargetExecContext->BeforeTestCaseStart(
		Context->TargetExecContext,
		ThreadId,
		TestCase );
}

//
// AFTER
//
static VOID CfixsEventEmittingProxyAfterFixtureFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_FIXTURE Fixture,
	__in BOOL RanToCompletion
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->CurrentFixture = NULL;

	Context->EventSink->AfterFixtureFinish(
		Context->EventSink,
		ThreadId,
		Fixture->Module->Name,
		Fixture->Name,
		RanToCompletion );

	Context->TargetExecContext->AfterFixtureFinish(
		Context->TargetExecContext,
		ThreadId,
		Fixture,
		RanToCompletion );
}


static VOID CfixsEventEmittingProxyAfterTestCaseFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PCFIX_TEST_CASE TestCase,
	__in BOOL RanToCompletion
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->CurrentTestCase = NULL;

	Context->EventSink->AfterTestCaseFinish(
		Context->EventSink,
		ThreadId,
		TestCase->Fixture->Module->Name,
		TestCase->Fixture->Name,
		TestCase->Name,
		RanToCompletion );

	Context->TargetExecContext->AfterTestCaseFinish(
		Context->TargetExecContext,
		ThreadId,
		TestCase,
		RanToCompletion );
}

VOID CfixsEventEmittingProxyBeforeChildThreadStart(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in_opt PVOID ThreadContext
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->TargetExecContext->BeforeChildThreadStart(
		Context->TargetExecContext,
		ThreadId,
		ThreadContext );
}

VOID CfixsEventEmittingProxyAfterChildThreadFinish(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in_opt PVOID ThreadContext
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->TargetExecContext->AfterChildThreadFinish(
		Context->TargetExecContext,
		ThreadId,
		ThreadContext );
}

static HRESULT CfixsEventEmittingProxyCreateChildThread(
	__in struct _CFIX_EXECUTION_CONTEXT *This,
	__in PCFIX_THREAD_ID ThreadId, 
	__out PVOID *ContextForChild
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	return Context->TargetExecContext->CreateChildThread(
		Context->TargetExecContext,
		ThreadId,
		ContextForChild );
}

static VOID CfixsEventEmittingProxyOnUnhandledException(
	__in PCFIX_EXECUTION_CONTEXT This,
	__in PCFIX_THREAD_ID ThreadId, 
	__in PEXCEPTION_POINTERS ExcpPointers
	)
{
	PCFIXP_EVENT_EMITTING_PROXY Context = ( PCFIXP_EVENT_EMITTING_PROXY ) This;
	ASSERT( Context );

	Context->TargetExecContext->OnUnhandledException(
		Context->TargetExecContext,
		ThreadId,
		ExcpPointers );
}

/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */

CFIXAPI HRESULT CFIXCALLTYPE CfixCreateEventEmittingExecutionContextProxy(
	__in PCFIX_EXECUTION_CONTEXT TargetExecContext,
	__in PCFIX_EVENT_SINK EventSink,
	__out PCFIX_EXECUTION_CONTEXT *Proxy
	)
{
	PCFIXP_EVENT_EMITTING_PROXY NewContext;

	if ( ! TargetExecContext || ! EventSink || ! Proxy )
	{
		return E_INVALIDARG;
	}

	NewContext = malloc( sizeof( CFIXP_EVENT_EMITTING_PROXY ) );
	if ( ! NewContext )
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory( NewContext, sizeof( CFIXP_EVENT_EMITTING_PROXY ) );

	NewContext->ReferenceCount				= 1;

	EventSink->Reference( EventSink );
	NewContext->EventSink					= EventSink;

	TargetExecContext->Reference( TargetExecContext );
	NewContext->TargetExecContext			= TargetExecContext;

	NewContext->Base.Version				= CFIX_TEST_CONTEXT_VERSION;
	NewContext->Base.ReportEvent			= CfixsEventEmittingProxyReportEvent;
	NewContext->Base.QueryDefaultDisposition= CfixsEventEmittingProxyQueryDefaultDisposition;
	NewContext->Base.BeforeFixtureStart		= CfixsEventEmittingProxyBeforeFixtureStart;
	NewContext->Base.AfterFixtureFinish		= CfixsEventEmittingProxyAfterFixtureFinish;
	NewContext->Base.BeforeTestCaseStart	= CfixsEventEmittingProxyBeforeTestCaseStart;
	NewContext->Base.AfterTestCaseFinish	= CfixsEventEmittingProxyAfterTestCaseFinish;
	NewContext->Base.CreateChildThread		= CfixsEventEmittingProxyCreateChildThread;
	NewContext->Base.BeforeChildThreadStart	= CfixsEventEmittingProxyBeforeChildThreadStart;
	NewContext->Base.AfterChildThreadFinish	= CfixsEventEmittingProxyAfterChildThreadFinish;
	NewContext->Base.OnUnhandledException	= CfixsEventEmittingProxyOnUnhandledException;
	NewContext->Base.Reference				= CfixsEventEmittingProxyReference;
	NewContext->Base.Dereference			= CfixsEventEmittingProxyDereference;

	*Proxy = &NewContext->Base;

	return S_OK;
}