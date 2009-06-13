/*----------------------------------------------------------------------
 * Purpose:
 *		Action that merely displays, rather than run, tests.
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

#include "cfixrunp.h"
#include <stdlib.h>

#define DISPLAY_ACTION_SIGNATURE 'psiD'

typedef struct _DISPLAY_ACTION
{
	//
	// Always set to DISPLAY_ACTION_SIGNATURE.
	//
	DWORD Signature;

	CFIX_ACTION Base;

	volatile LONG ReferenceCount;

	PCFIX_FIXTURE Fixture;
	PCFIXRUN_OPTIONS RunOptions;
} DISPLAY_ACTION, *PDISPLAY_ACTION;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static VOID CfixrunsDeleteDisplayAction( 
	__in PDISPLAY_ACTION Action 
	)
{
	Action->Fixture->Module->Routines.Dereference( Action->Fixture->Module );
	free( Action );
}

static VOID CfixrunsReferenceDisplayAction(
	__in PCFIX_ACTION This
	)
{
	PDISPLAY_ACTION Action = CONTAINING_RECORD(
		This,
		DISPLAY_ACTION,
		Base );
	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == DISPLAY_ACTION_SIGNATURE );

	InterlockedIncrement( &Action->ReferenceCount );
}

static VOID CfixrunsDereferenceDisplayAction(
	__in PCFIX_ACTION This
	)
{
	PDISPLAY_ACTION Action = CONTAINING_RECORD(
		This,
		DISPLAY_ACTION,
		Base );
	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == DISPLAY_ACTION_SIGNATURE );

	if ( 0 == InterlockedDecrement( &Action->ReferenceCount ) )
	{
		CfixrunsDeleteDisplayAction( Action );
	}
}

static HRESULT CfixrunsRunDisplayAction(
	__in PCFIX_ACTION This,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	ULONG Case;

	PDISPLAY_ACTION Action = CONTAINING_RECORD(
		This,
		DISPLAY_ACTION,
		Base );

	ASSERT( CfixIsValidAction( This ) &&
		    Action->Signature == DISPLAY_ACTION_SIGNATURE );
	UNREFERENCED_PARAMETER( Context );

	Action->RunOptions->PrintConsole( L"  Fixture: %s.%s\n",
		Action->Fixture->Module->Name,
		Action->Fixture->Name );

	for ( Case = 0; Case < Action->Fixture->TestCaseCount; Case++ )
	{
		#pragma warning( suppress : 6385 )
		Action->RunOptions->PrintConsole( L"    %s\n", 
			Action->Fixture->TestCases[ Case ].Name );
	}

	Action->RunOptions->PrintConsole( L"\n" );

	return S_OK;
}

/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */
HRESULT CFIXCALLTYPE CfixrunpCreateDisplayAction(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIXRUN_OPTIONS RunOptions,
	__out PCFIX_ACTION *Action
	)
{
	PDISPLAY_ACTION NewAction = NULL;
	HRESULT Hr = E_UNEXPECTED;
	if ( ! Action )
	{
		return E_INVALIDARG;
	}

	//
	// Allocate.
	//
	NewAction = malloc( sizeof( DISPLAY_ACTION ) );
	if ( ! NewAction )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	NewAction->Signature		= DISPLAY_ACTION_SIGNATURE;
	NewAction->ReferenceCount	= 1;

	NewAction->Base.Version		= CFIX_ACTION_VERSION;
	NewAction->Base.Run			= CfixrunsRunDisplayAction;
	NewAction->Base.Reference	= CfixrunsReferenceDisplayAction;
	NewAction->Base.Dereference	= CfixrunsDereferenceDisplayAction;

	//
	// Addref module to lock it.
	//
	Fixture->Module->Routines.Reference( Fixture->Module );
	NewAction->Fixture			= Fixture;
	NewAction->RunOptions		= RunOptions;

	*Action = &NewAction->Base;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) && NewAction )
	{
		free( NewAction );
	}

	return Hr;
}
