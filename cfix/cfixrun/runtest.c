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

static HRESULT CfixrunsCreateDisplayAction(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__out PCFIX_ACTION *Action
	)
{
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;

	return CfixrunpCreateDisplayAction(
		Fixture,
		State->Options,
		Action );
}

static HRESULT CfixrunsCreateTsExecAction(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__out PCFIX_ACTION *Action
	)
{
	ULONG ExecutionFlags;
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;

	ASSERT( State != NULL );

	ExecutionFlags = 0;
	if ( State->Options->ShortcutFixtureOnFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCUT_FIXTURE_ON_FAILURE;
	}
	if ( State->Options->ShortcutRunOnFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCUT_RUN_ON_FAILURE;
	}
	if ( State->Options->ShortcutRunOnSetupFailure )
	{
		ExecutionFlags |= CFIX_FIXTURE_EXECUTION_SHORTCUT_RUN_ON_SETUP_FAILURE;
	}

	return CfixCreateFixtureExecutionAction(
		Fixture,
		ExecutionFlags,
		Action );
}

static BOOL CfixrunsFilterFixtureByName(
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context
	)
{
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;
	
	PCWSTR FixtureName = Fixture->Name;

	if ( State->Options->Fixture )
	{
		if ( 0 != _wcsicmp( State->Options->Fixture, FixtureName ) )
		{
			//
			// Skip.
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