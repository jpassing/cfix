/*----------------------------------------------------------------------
 * Purpose:
 *		Test Fixture Exection.
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

#include "cfixp.h"
#include <stdlib.h>

#define TSEXEC_ACTION_SIGNATURE 'xesT'

typedef struct _TSEXEC_ACTION
{
	//
	// Always set to TSEXEC_ACTION_SIGNATURE.
	//
	DWORD Signature;

	CFIX_ACTION Base;

	volatile LONG ReferenceCount;

	//
	// Fixture, implicitly locked via Module.
	//
	PCFIX_FIXTURE Fixture;

	//
	// Module - addref'ed.
	//
	PCFIX_TEST_MODULE Module;

	//
	// Flags determining the execution behaviour.
	//
	ULONG Flags;

	//
	// Index of test case to run or -1 to run all.
	//
	ULONG TestCaseIndex;
} TSEXEC_ACTION, *PTSEXEC_ACTION;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static VOID CfixsDeleteFixtureExecutionAction( 
	__in PTSEXEC_ACTION Action 
	)
{
	Action->Module->Routines.Dereference( Action->Module );
	free( Action );
}

static VOID CfixsReferenceFixtureExecutionAction(
	__in PCFIX_ACTION This
	)
{
	PTSEXEC_ACTION Action = CONTAINING_RECORD(
		This,
		TSEXEC_ACTION,
		Base );
	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == TSEXEC_ACTION_SIGNATURE );

	InterlockedIncrement( &Action->ReferenceCount );
}

static VOID CfixsDereferenceFixtureExecutionAction(
	__in PCFIX_ACTION This
	)
{
	PTSEXEC_ACTION Action = CONTAINING_RECORD(
		This,
		TSEXEC_ACTION,
		Base );
	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == TSEXEC_ACTION_SIGNATURE );

	if ( 0 == InterlockedDecrement( &Action->ReferenceCount ) )
	{
		CfixsDeleteFixtureExecutionAction( Action );
	}
}

static HRESULT CfixsRunTestCaseFixtureExecutionAction(
	__in PTSEXEC_ACTION Action,
	__in PCFIX_TEST_CASE TestCase,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	HRESULT Hr;
	HRESULT HrTestCase;

	//
	// Run before-routine.
	//
	Hr = Action->Module->Routines.Before(
		Action->Fixture,
		Context );

	ASSERT( S_OK == Hr ||
			CFIX_E_TESTRUN_ABORTED == Hr ||
			CFIX_E_BEFORE_ROUTINE_FAILED == Hr );

	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	//
	// Before-routine succeeded, proceed with actual test routine.
	//
	HrTestCase = Action->Module->Routines.RunTestCase(
		TestCase,
		Context );

	ASSERT( S_OK == Hr ||
			CFIX_E_TEST_ROUTINE_FAILED == Hr ||
			CFIX_E_TESTRUN_ABORTED == Hr );

	//
	// Run after-routine, regardless of whether test routine 
	// succeeded or not.
	//
	Hr = Action->Module->Routines.After(
		Action->Fixture,
		Context );

	ASSERT( S_OK == Hr ||
			CFIX_E_TESTRUN_ABORTED == Hr ||
			CFIX_E_AFTER_ROUTINE_FAILED == Hr );

	if ( FAILED( Hr ) )
	{
		if ( FAILED( HrTestCase ) )
		{
			//
			// Both test routine and after-routine failed. Ignore
			// exact failure of latter.
			//
			return HrTestCase;
		}
		else
		{
			return Hr;
		}
	}

	return HrTestCase;
}

static HRESULT CfixsRunFixtureExecutionAction(
	__in PCFIX_ACTION This,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	PTSEXEC_ACTION Action = CONTAINING_RECORD(
		This,
		TSEXEC_ACTION,
		Base );
	HRESULT Hr;
	UINT Index;
	BOOL FixtureRanToCompletion = TRUE;
	ULONG MainThreadId;

	//
	// The calling thread is always the MainThread.
	//
	MainThreadId = GetCurrentThreadId();

	ASSERT( CfixIsValidAction( This ) );
	ASSERT( CfixIsValidContext( Context ) );

	if ( ! CfixIsValidAction( This ) ||
		 ! CfixIsValidContext( Context ) ||
		 Action->Signature != TSEXEC_ACTION_SIGNATURE )
	{
		return E_INVALIDARG;
	}

	Hr = Context->BeforeFixtureStart(
		Context,
		MainThreadId,
		Action->Fixture );
	if ( FAILED( Hr ) )
	{
		//
		// Immediate abortion.
		//
		return Hr;
	}

	Hr = Action->Module->Routines.Setup( 
		Action->Fixture,
		Context );

	ASSERT( S_OK == Hr ||
			CFIX_E_SETUP_ROUTINE_FAILED == Hr ||
			CFIX_E_TESTRUN_ABORTED == Hr );

	//
	// N.B. These HRESULTs expected. But there may also be true failure 
	// HRESULTs that must be treated as fatal.
	//

	if ( CFIX_E_SETUP_ROUTINE_FAILED == Hr ||
		 CFIX_E_TESTRUN_ABORTED == Hr )
	{
		//
		// Setup routine failed.
		// N.B. Event has already been delivered to execution context.
		//
		FixtureRanToCompletion = FALSE;
		if ( Action->Flags & CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_RUN_ON_SETUP_FAILURE )
		{
			//
			// Short-circuit run by returning failure HR.
			//
		}
		else
		{
			//
			// Short-circuit fixture by setting HR back to success.
			//
			Hr = S_OK;
		}
	}
	else if ( FAILED( Hr ) )
	{
		//
		// Fatal error.
		//
		return Hr;
	}
	else
	{
		BOOL FixtureShortCircuit = FALSE;
		HRESULT TeardownHr;
		
		//
		// Run all testcases.
		//
		for ( Index = 0; 
			  ! FixtureShortCircuit && Index < Action->Fixture->TestCaseCount; 
			  Index++ )
		{
			BOOL TestCaseRanToCompletion;

			if ( Action->TestCaseIndex != -1 &&
				 Action->TestCaseIndex != Index )
			{
				//
				// Skip this test case.
				//
				continue;
			}

			Hr = Context->BeforeTestCaseStart(
				Context,
				MainThreadId,
				&Action->Fixture->TestCases[ Index ] );
			if ( FAILED( Hr ) )
			{
				FixtureShortCircuit = TRUE;
				break;
			}

			Hr = CfixsRunTestCaseFixtureExecutionAction(
				Action,
				&Action->Fixture->TestCases[ Index ],
				Context );

			ASSERT( S_OK == Hr ||
					CFIX_E_BEFORE_ROUTINE_FAILED == Hr ||
					CFIX_E_AFTER_ROUTINE_FAILED == Hr ||
					CFIX_E_TEST_ROUTINE_FAILED == Hr ||
					CFIX_E_TESTRUN_ABORTED == Hr );

			TestCaseRanToCompletion = SUCCEEDED( Hr );

			if ( CFIX_E_BEFORE_ROUTINE_FAILED == Hr ||
				 CFIX_E_AFTER_ROUTINE_FAILED == Hr ||
				 CFIX_E_TEST_ROUTINE_FAILED == Hr )
			{
				if ( Action->Flags & CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_FIXTURE_ON_FAILURE )
				{
					//
					// Short-circuit fixture.
					//
					FixtureShortCircuit = TRUE;

					if ( Action->Flags & CFIX_FIXTURE_EXECUTION_ESCALATE_FIXTURE_FAILUES )
					{
						//
						// Maintain failure HR s.t. run is aborted
						//
					}
					else
					{
						//
						// Set HR to success s.t. run continues.
						//
						Hr = S_OK;
					}
				}
				else
				{
					//
					// Proceed as if nothing happened.
					//
					Hr = S_OK;
				}
			}
			else if ( CFIX_E_TESTRUN_ABORTED == Hr )
			{
				//
				// Maintain failure HR s.t. run is aborted
				//
				FixtureShortCircuit = TRUE;
			}
			else if ( FAILED( Hr ) )
			{
				//
				// Fatal error.
				//
				return Hr;
			}

			Context->AfterTestCaseFinish(
				Context,
				MainThreadId,
				&Action->Fixture->TestCases[ Index ],
				TestCaseRanToCompletion );
		}

		FixtureRanToCompletion = ! FixtureShortCircuit;

		//
		// Teardown. Teardown failures are not severe - action
		// is resumed, though the fixture must be considered
		// not to have run to completion.
		//
		TeardownHr = Action->Module->Routines.Teardown( 
			Action->Fixture,
			Context );
		if ( FAILED( TeardownHr ) )
		{
			FixtureRanToCompletion = FALSE;
			if ( Action->Flags & CFIX_FIXTURE_EXECUTION_ESCALATE_FIXTURE_FAILUES )
			{
				//
				// Pass failure HR s.t. run is aborted.
				//
				Hr = TeardownHr;
			}
			else
			{
				//
				// Eat TeardownHr s.t. run continues.
				//
			}
		}
	}

	Context->AfterFixtureFinish(
		Context,
		MainThreadId,
		Action->Fixture,
		FixtureRanToCompletion );

	return Hr;
}

/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */
CFIXAPI HRESULT CFIXCALLTYPE CfixCreateFixtureExecutionAction(
	__in PCFIX_FIXTURE Fixture,
	__in ULONG Flags,
	__in ULONG TestCase,
	__out PCFIX_ACTION *Action
	)
{
	PTSEXEC_ACTION NewAction = NULL;
	HRESULT Hr = E_UNEXPECTED;
	if ( ! Fixture || 
		 ! Action || 
		 ( TestCase != ( ULONG ) -1 && TestCase >= Fixture->TestCaseCount ) )
	{
		return E_INVALIDARG;
	}

	//
	// Allocate.
	//
	NewAction = malloc( sizeof( TSEXEC_ACTION ) );
	if ( ! NewAction )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	NewAction->Signature		= TSEXEC_ACTION_SIGNATURE;
	NewAction->ReferenceCount	= 1;
	NewAction->Fixture			= Fixture;
	NewAction->Module			= Fixture->Module;
	NewAction->Flags			= Flags;
	NewAction->TestCaseIndex	= TestCase;

	NewAction->Base.Version		= CFIX_ACTION_VERSION;
	NewAction->Base.Run			= CfixsRunFixtureExecutionAction;
	NewAction->Base.Reference	= CfixsReferenceFixtureExecutionAction;
	NewAction->Base.Dereference	= CfixsDereferenceFixtureExecutionAction;

	//
	// Addref module to lock it.
	//
	Fixture->Module->Routines.Reference( Fixture->Module );

	*Action = &NewAction->Base;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) && NewAction )
	{
		free( NewAction );
	}

	return Hr;
}