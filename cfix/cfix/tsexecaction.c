/*----------------------------------------------------------------------
 * Purpose:
 *		Test Fixture Exection.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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
} TSEXEC_ACTION, *PTSEXEC_ACTION;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static VOID CfixsDeleteTsexecActionMethod( 
	__in PTSEXEC_ACTION Action 
	)
{
	Action->Module->Routines.Dereference( Action->Module );
	free( Action );
}

static VOID CfixsReferenceTsexecActionMethod(
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

static VOID CfixsDereferenceTsexecActionMethod(
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
		CfixsDeleteTsexecActionMethod( Action );
	}
}

static HRESULT CfixsRunTsexecActionMethod(
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

	if ( ! CfixIsValidAction( This ) ||
		 ! CfixIsValidContext( Context ) ||
		 Action->Signature != TSEXEC_ACTION_SIGNATURE )
	{
		return E_INVALIDARG;
	}

	Context->BeforeFixtureStart(
		Context,
		Action->Fixture );

	//
	// Setup. Setup failures are considered severe - a failure 
	// HRESULT is returned s.t. action execution is forestalled.
	//
	Hr = Action->Module->Routines.Setup( 
		Action->Fixture,
		Context );
	if ( SUCCEEDED( Hr ) )
	{
		//
		// Run all testcases.
		//
		for ( Index = 0; Index < Action->Fixture->TestCaseCount; Index++ )
		{
			Context->BeforeTestCaseStart(
				Context,
				&Action->Fixture->TestCases[ Index ] );

			Hr = Action->Module->Routines.RunTestCase(
				Action->Fixture,
				&Action->Fixture->TestCases[ Index ],
				Context );

			Context->AfterTestCaseFinish(
				Context,
				&Action->Fixture->TestCases[ Index ],
				SUCCEEDED( Hr ) );

			if ( FAILED( Hr ) )
			{
				FixtureRanToCompletion = FALSE;
				break;
			}
		}
	}
	else
	{
		FixtureRanToCompletion = FALSE;
	}

	if ( FixtureRanToCompletion )
	{
		//
		// Teardown. Teardown failures are not severe - action
		// is resumed, though the fixture must be considered
		// not to have run to completion.
		//
		if ( FAILED(  Action->Module->Routines.Teardown( 
			Action->Fixture,
			Context ) ) )
		{
			FixtureRanToCompletion = FALSE;
		}
	}

	Context->AfterFixtureFinish(
		Context,
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
	__out PCFIX_ACTION *Action
	)
{
	PTSEXEC_ACTION NewAction = NULL;
	HRESULT Hr = E_UNEXPECTED;
	if ( ! Fixture || ! Action )
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
	NewAction->Base.Version		= CFIX_ACTION_VERSION;
	NewAction->Base.Run			= CfixsRunTsexecActionMethod;
	NewAction->Base.Reference	= CfixsReferenceTsexecActionMethod;
	NewAction->Base.Dereference	= CfixsDereferenceTsexecActionMethod;

	//
	// Addref module to lock ot.
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