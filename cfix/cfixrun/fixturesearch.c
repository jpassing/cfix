/*----------------------------------------------------------------------
 * Purpose:
 *		Fixture Search.
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

typedef struct _CFIXRUNP_SEARCH_CONTEXT
{
	PCFIX_ACTION SequenceAction;
	CDIAG_SESSION_HANDLE LogSession;
	CFIXRUNP_FILTER_FIXTURE_ROUTINE FilterCallback;
	CFIXRUNP_CREATE_ACTION_ROUTINE CreateActionCallback;
	PVOID CallbackContext;
} CFIXRUNP_SEARCH_CONTEXT, *PCFIXRUNP_SEARCH_CONTEXT;

static HRESULT CfixrunsAddFixturesOfDllToSequenceAction(
	__in PCWSTR Path,
	__in CFIXRUN_MODULE_TYPE Type,
	__in PVOID Context,
	__in BOOL SearchPerformed
	)
{
	HRESULT Hr;
	PCFIXRUNP_SEARCH_CONTEXT SearchCtx = 
		( PCFIXRUNP_SEARCH_CONTEXT ) Context;
	PCFIX_TEST_MODULE TestModule;

	ASSERT( CfixIsValidAction( SearchCtx->SequenceAction ) );
	
	if ( Type == CfixrunSys )
	{
		Hr = CfixklCreateTestModuleFromDriver( Path, &TestModule, NULL, NULL );
	}
	else
	{
		Hr = CfixCreateTestModuleFromPeImage( Path, &TestModule );
	}

	if ( FAILED( Hr ) )
	{
		if ( SearchPerformed )
		{
			//
			// Nevermind, this is probably just one of many DLLs.
			//
			CfixrunpOutputLogMessage(
				SearchCtx->LogSession,
				CdiagInfoSeverity,
				L"Failed to load module %s: 0x%08X\n",
				Path,
				Hr );
			return S_OK;
		}
		else
		{
			//
			// DLL was specified explicitly -> fail.
			//
			return Hr;
		}
	}

	CfixrunpOutputLogMessage(
		SearchCtx->LogSession,
		CdiagInfoSeverity,
		L"Loaded module %s\n",
		Path );

	ASSERT( TestModule->Version == CFIX_TEST_MODULE_VERSION );

	//
	// Add fixtures to sequence.
	//
	if ( TestModule->FixtureCount > 0 )
	{
		UINT Fixture;

		for ( Fixture = 0; Fixture < TestModule->FixtureCount; Fixture++ )
		{
			PCFIX_ACTION FixtureAction;
			ULONG TestCase;

			//
			// Apply filter.
			//
			if ( ! SearchCtx->FilterCallback( 
				TestModule->Fixtures[ Fixture ],
				SearchCtx->CallbackContext,
				&TestCase ) )
			{
				//
				// Ignore this one.
				//
				continue;
			}

			//
			// Create an action for this fixture.
			//
			Hr = SearchCtx->CreateActionCallback(
				TestModule->Fixtures[ Fixture ],
				SearchCtx->CallbackContext,
				TestCase,
				&FixtureAction );

			ASSERT( SUCCEEDED( Hr ) == ( FixtureAction != NULL ) );

			if ( FAILED( Hr ) || FixtureAction == NULL )
			{
				CfixrunpOutputLogMessage(
					SearchCtx->LogSession,
					CdiagErrorSeverity,
					L"Failed to create fixture execution action: 0x%08X\n",
					Hr );
				break;
			}
			else
			{
				ASSERT( CfixIsValidAction( FixtureAction ) );

				//
				// Add to sequence
				//
				Hr = CfixAddEntrySequenceAction(
					SearchCtx->SequenceAction,
					FixtureAction );
	
				FixtureAction->Dereference( FixtureAction );

				if ( FAILED( Hr ) )
				{
					CfixrunpOutputLogMessage(
						SearchCtx->LogSession,
						CdiagErrorSeverity,
						L"Failed to enqueue fixture execution action: 0x%08X\n",
						Hr );
					break;
				}
			}
		}
	}

	TestModule->Routines.Dereference( TestModule );
	return S_OK;
}

HRESULT CfixrunpSearchFixturesAndCreateSequenceAction( 
	__in PCWSTR DllOrDirectory,
	__in BOOL RecursiveSearch,
	__in BOOL IncludeKernelModules,
	__in CDIAG_SESSION_HANDLE LogSession,
	__in CFIXRUNP_FILTER_FIXTURE_ROUTINE FilterCallback,
	__in CFIXRUNP_CREATE_ACTION_ROUTINE CreateActionCallback,
	__in PVOID CallbackContext,
	__out PCFIX_ACTION *SequenceAction
	)
{
	CFIXRUNP_SEARCH_CONTEXT SearchCtx;

	HRESULT Hr;
	
	if ( ! DllOrDirectory || 
		 ! LogSession || 
		 ! FilterCallback || 
		 ! CreateActionCallback || 
		 ! SequenceAction )
	{
		return E_INVALIDARG;
	}

	*SequenceAction = NULL;
	
	Hr = CfixCreateSequenceAction( SequenceAction );
	if ( FAILED( Hr ) )
	{
		CfixrunpOutputLogMessage(
			LogSession,
			CdiagErrorSeverity,
			L"Failed to create sequence action: 0x%08X\n",
			Hr );
		return Hr;
	}

	//
	// Search DLLs and and populate sequence.
	//
	SearchCtx.SequenceAction		= *SequenceAction;
	SearchCtx.LogSession			= LogSession;
	SearchCtx.FilterCallback		= FilterCallback;
	SearchCtx.CreateActionCallback	= CreateActionCallback;
	SearchCtx.CallbackContext		= CallbackContext;

	Hr = CfixrunSearchModules(
		DllOrDirectory,
		RecursiveSearch,
		IncludeKernelModules,
		CfixrunsAddFixturesOfDllToSequenceAction,
		&SearchCtx );

	if ( SUCCEEDED( Hr ) )
	{
		return S_OK;
	}
	else
	{
		( *SequenceAction )->Dereference( *SequenceAction );
		*SequenceAction = NULL;
		return Hr;
	}
}