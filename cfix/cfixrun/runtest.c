/*----------------------------------------------------------------------
 * Purpose:
 *		Fixtures Display.
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
#include <stdio.h>
#include <shlwapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

typedef struct _CFIXRUNP_ASSEMBLE_ACTION_CONTEXT
{
	PCFIXRUN_STATE RunState;
	ULONG FixtureCount;
} CFIXRUNP_ASSEMBLE_ACTION_CONTEXT, *PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT;

static HRESULT CfixrunsCreateDisplayAction(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID PvContext,
	__in ULONG TestCase,
	__out PCFIX_ACTION *Action
	)
{
	PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT Context;
	HRESULT Hr;

	Context = ( PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT ) PvContext;
	ASSERT( Context != NULL );

	//
	// N.B. TestCase is ignored - it is of little value when displaying.
	//
	UNREFERENCED_PARAMETER( TestCase );

	Hr = CfixrunpCreateDisplayAction(
		Fixture,
		Context->RunState->Options,
		Action );

	if ( SUCCEEDED( Hr ) )
	{
		Context->FixtureCount++;
	}

	return Hr;
}

static HRESULT CfixrunsCreateTsExecAction(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID PvContext,
	__in ULONG TestCase,
	__out PCFIX_ACTION *Action
	)
{
	PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT Context;
	ULONG ExecutionFlags = 0;
	HRESULT Hr;

	Context = ( PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT ) PvContext;
	ASSERT( Context != NULL );

	if ( ! Context->RunState->Options->DisableStackTraces )
	{
		ExecutionFlags = CFIX_FIXTURE_EXECUTION_CAPTURE_STACK_TRACES;
	}

	if ( Context->RunState->Options->ShortCircuitFixtureOnFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_FIXTURE_ON_FAILURE;
	}

	if ( Context->RunState->Options->ShortCircuitRunOnFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_RUN_ON_FAILURE;
	}

	if ( Context->RunState->Options->ShortCircuitRunOnSetupFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_RUN_ON_SETUP_FAILURE;
	}

	Hr = CfixCreateFixtureExecutionAction(
		Fixture,
		ExecutionFlags,
		TestCase,
		Action );

	if ( SUCCEEDED( Hr ) )
	{
		Context->FixtureCount++;
	}

	return Hr;
}

static BOOL CfixrunsFilterFixtureByName(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID PvContext,
	__out PULONG TestCase
	)
{
	PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT Context;
	PCWSTR FixtureName = Fixture->Name;

	Context = ( PCFIXRUNP_ASSEMBLE_ACTION_CONTEXT ) PvContext;
	ASSERT( Context != NULL );

	ASSERT( TestCase );
	*TestCase = ( ULONG ) -1;	// All.

	if ( Context->RunState->Options->Fixture )
	{
		WCHAR Filter[ CFIX_MAX_FIXTURE_NAME_CCH * 2 ];
		PWSTR TestCaseName;
		
		//
		// Create writable copy.
		//
		if ( FAILED( StringCchCopy( 
			Filter, 
			_countof( Filter ), 
			Context->RunState->Options->Fixture ) ) )
		{
			return FALSE;
		}

		TestCaseName = wcschr( Filter, L'.' );
		if ( TestCaseName != NULL )
		{
			//
			// Fixture and test case name specifie, split.
			//
			*TestCaseName = L'\0';
			TestCaseName++;
		}

		if ( 0 != _wcsicmp( Filter, FixtureName ) )
		{
			//
			// Fixture name mismatch.
			//
			return FALSE;
		}

		if ( TestCaseName == NULL )
		{
			return TRUE;
		}
		else
		{
			//
			// Find matching test case.
			//
			ULONG Index;
			for ( Index = 0; Index < Fixture->TestCaseCount; Index++ )
			{
				if ( 0 == _wcsicmp( TestCaseName, Fixture->TestCases[ Index ].Name ) )
				{
					*TestCase = Index;
					return TRUE;
				}
			}

			//
			// This test case does not exist.
			//
			return FALSE;
		}
	}
	else if ( Context->RunState->Options->FixturePrefix )
	{
		if ( FixtureName != StrStrI( FixtureName, Context->RunState->Options->FixturePrefix ) )
		{
			//
			// Skip.
			//
			return FALSE;
		}
	}

	return TRUE;
}

static HRESULT CfixrunsCreateSequenceActionForCurrentExecutable( 
	__in CFIXRUNP_FILTER_FIXTURE_ROUTINE FilterCallback,
	__in CFIXRUNP_CREATE_ACTION_ROUTINE CreateActionCallback,
	__in PVOID CallbackContext,
	__out PCFIX_ACTION *SequenceAction
	)
{
	HRESULT Hr;
	HMODULE Module = GetModuleHandle( NULL );
	PCFIX_TEST_MODULE TestModule;

	ASSERT( Module != NULL );
	__assume( Module != NULL );

	Hr = CfixCreateTestModule(
		Module,
		&TestModule );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	Hr = CfixrunpCreateSequenceAction(
		TestModule,
		FilterCallback,
		CreateActionCallback,
		CallbackContext,
		SequenceAction );

	TestModule->Routines.Dereference( TestModule );
	return Hr;
}

HRESULT CfixrunpAssembleExecutionAction( 
	__in PCFIXRUN_STATE State,
	__out PCFIX_ACTION *Action,
	__out PULONG FixtureCount
	)
{
	CFIXRUNP_ASSEMBLE_ACTION_CONTEXT Context;
	HRESULT Hr;

	ASSERT( Action );
	ASSERT( FixtureCount );

	Context.RunState		= State;
	Context.FixtureCount	= 0;

	if ( State->Options->InputFileType == CfixrunInputDynamicallyLoadable )
	{
		Hr = CfixrunpSearchFixturesAndCreateSequenceAction(
			State->Options->InputFile,
			State->Options->RecursiveSearch,
			State->Options->EnableKernelFeatures,
			CfixrunsFilterFixtureByName,
			CfixrunsCreateTsExecAction,
			&Context,
			Action );
	}
	else
	{
		Hr = CfixrunsCreateSequenceActionForCurrentExecutable(
			CfixrunsFilterFixtureByName,
			CfixrunsCreateTsExecAction,
			&Context,
			Action );
	}

	*FixtureCount = Context.FixtureCount;
	return Hr;
}

HRESULT CfixrunpAssembleDisplayAction( 
	__in PCFIXRUN_STATE State,
	__out PCFIX_ACTION *Action,
	__out PULONG FixtureCount
	)
{
	CFIXRUNP_ASSEMBLE_ACTION_CONTEXT Context;
	HRESULT Hr;

	ASSERT( Action );
	ASSERT( FixtureCount );

	Context.RunState		= State;
	Context.FixtureCount	= 0;
	
	if ( State->Options->InputFileType == CfixrunInputDynamicallyLoadable )
	{
		Hr = CfixrunpSearchFixturesAndCreateSequenceAction(
			State->Options->InputFile,
			State->Options->RecursiveSearch,
			State->Options->EnableKernelFeatures,
			CfixrunsFilterFixtureByName,
			CfixrunsCreateDisplayAction,
			&Context,
			Action );
	}
	else
	{
		Hr = CfixrunsCreateSequenceActionForCurrentExecutable(
			CfixrunsFilterFixtureByName,
			CfixrunsCreateDisplayAction,
			&Context,
			Action );
	}

	*FixtureCount = Context.FixtureCount;
	return Hr;
}