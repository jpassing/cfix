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

static HRESULT CfixrunsCreateDisplayAction(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__in ULONG TestCase,
	__out PCFIX_ACTION *Action
	)
{
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;

	//
	// N.B. TestCase is ignored - it is of little value when displaying.
	//
	UNREFERENCED_PARAMETER( TestCase );

	return CfixrunpCreateDisplayAction(
		Fixture,
		State->Options,
		Action );
}

static HRESULT CfixrunsCreateTsExecAction(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__in ULONG TestCase,
	__out PCFIX_ACTION *Action
	)
{
	ULONG ExecutionFlags = 0;
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;

	ASSERT( State != NULL );

	if ( ! State->Options->DisableStackTraces )
	{
		ExecutionFlags = CFIX_FIXTURE_EXECUTION_CAPTURE_STACK_TRACES;
	}

	if ( State->Options->ShortCircuitFixtureOnFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_FIXTURE_ON_FAILURE;
	}

	if ( State->Options->ShortCircuitRunOnFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_RUN_ON_FAILURE;
	}

	if ( State->Options->ShortCircuitRunOnSetupFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCIRCUIT_RUN_ON_SETUP_FAILURE;
	}

	return CfixCreateFixtureExecutionAction(
		Fixture,
		ExecutionFlags,
		TestCase,
		Action );
}

static BOOL CfixrunsFilterFixtureByName(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__out PULONG TestCase
	)
{
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;
	
	PCWSTR FixtureName = Fixture->Name;

	ASSERT( TestCase );
	*TestCase = ( ULONG ) -1;	// All.

	if ( State->Options->Fixture )
	{
		WCHAR Filter[ CFIX_MAX_FIXTURE_NAME_CCH * 2 ];
		PWSTR TestCaseName;
		
		//
		// Create writable copy.
		//
		if ( FAILED( StringCchCopy( 
			Filter, 
			_countof( Filter ), 
			State->Options->Fixture ) ) )
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
	else if ( State->Options->FixturePrefix )
	{
		if ( FixtureName != StrStrI( FixtureName, State->Options->FixturePrefix ) )
		{
			//
			// Skip.
			//
			return FALSE;
		}
	}

	return TRUE;
}

HRESULT CfixrunpAssembleExecutionAction( 
	__in PCFIXRUN_STATE State,
	__out PCFIX_ACTION *Action
	)
{
	if ( State->Options->InputFileType != CfixrunInputDllOrDirectory )
	{
		return E_INVALIDARG;
	}
	
	return CfixrunpSearchFixturesAndCreateSequenceAction(
		State->Options->InputFile,
		State->Options->RecursiveSearch,
		State->Options->EnableKernelFeatures,
		State->LogSession,
		CfixrunsFilterFixtureByName,
		CfixrunsCreateTsExecAction,
		State,
		Action );
}

HRESULT CfixrunpAssembleDisplayAction( 
	__in PCFIXRUN_STATE State,
	__out PCFIX_ACTION *Action
	)
{
	return CfixrunpSearchFixturesAndCreateSequenceAction(
		State->Options->InputFile,
		State->Options->RecursiveSearch,
		State->Options->EnableKernelFeatures,
		State->LogSession,
		CfixrunsFilterFixtureByName,
		CfixrunsCreateDisplayAction,
		State,
		Action );
}