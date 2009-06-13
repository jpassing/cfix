/*----------------------------------------------------------------------
 * Purpose:
 *		Fixtures Display.
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
#include "internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>

static HRESULT CfixrunsDisplayFixturesOfDll(
	__in PCWSTR Path,
	__in PVOID Context,
	__in BOOL SearchPerformed
	)
{
	PCFIXRUN_STATE State = ( PCFIXRUN_STATE ) Context;
	PCFIX_TEST_MODULE TestModule;
	HRESULT Hr;

	Hr = CfixCreateTestModuleFromPeImage( Path, &TestModule );
	if ( FAILED( Hr ) )
	{
		if ( SearchPerformed )
		{
			//
			// Nevermind, this is probably just one of many DLLs.
			//
			CfixrunpOutputLogMessage(
				State->LogSession,
				JpdiagInfoSeverity,
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
		State->LogSession,
		JpdiagInfoSeverity,
		L"Loaded module %s\n",
		Path );

	if ( TestModule->FixtureCount > 0 )
	{
		UINT Fixture;
		BOOL ModuleLinePrinted = FALSE;

		for ( Fixture = 0; Fixture < TestModule->FixtureCount; Fixture++ )
		{
			UINT Case;
			PCWSTR FixtureName = TestModule->Fixtures[ Fixture ]->Name;

			if ( State->Options->Fixture )
			{
				if ( 0 != _wcsicmp( State->Options->Fixture, FixtureName ) )
				{
					//
					// Skip.
					//
					continue;
				}
			}
			else if ( State->Options->FixturePrefix )
			{
				if ( FixtureName != StrStrI( FixtureName, State->Options->FixturePrefix ) )
				{
					//
					// Skip.
					//
					continue;
				}
			}

			if ( ! ModuleLinePrinted )
			{
				wprintf( L"Module: %s (%s)\n", TestModule->Name, Path );
				ModuleLinePrinted = TRUE;
			}

			wprintf( L"  Fixture: %s\n",
				TestModule->Fixtures[ Fixture ]->Name );

			for ( Case = 0; Case < TestModule->Fixtures[ Fixture ]->TestCaseCount; Case++ )
			{
				#pragma warning( suppress : 6385 )
				wprintf( L"    %s\n", 
					TestModule->Fixtures[ Fixture ]->TestCases[ Case ].Name );
			}
			wprintf( L"\n" );
		}
		wprintf( L"\n" );
	}

	TestModule->Routines.Dereference( TestModule );
	return S_OK;
}

HRESULT CfixrunpDisplayFixtures( 
	__in PCFIXRUN_STATE State
	)
{
	//
	// Search DLLs and deisplay them from within callback.
	//
	return CfixrunSearchDlls(
		State->Options->DllOrDirectory,
		State->Options->RecursiveSearch,
		CfixrunsDisplayFixturesOfDll,
		State );
}