/*----------------------------------------------------------------------
 * Purpose:
 *		Search for Test-DLLs.
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
#include "Cfixrun.h"
#include <stdio.h>

typedef enum
{
	StateExpectFlag,
	StateExpectValue,
	StateExpectAny
} PARSE_STATE;

static BOOL CfixrunsSetOutputTarget(
	__in PCWSTR Name,
	__in BOOL ProvideDefault,
	__out CFIXRUN_OUTPUT_TARGET *Target
	)
{
	if ( Name )
	{
		if ( 0 == wcscmp( Name, L"console" ) )
		{
			*Target = CfixrunTargetConsole;
		}
		else if ( 0 == wcscmp( Name, L"debug" ) )
		{
			*Target = CfixrunTargetDebug;
		}
		else
		{
			//
			// Must be a file.
			//
			*Target = CfixrunTargetFile;
		}
	}
	else if ( ProvideDefault )
	{
		if ( IsDebuggerPresent() )
		{
			*Target = CfixrunTargetDebug;
		}
		else
		{
			*Target = CfixrunTargetConsole;
		}
	}
	else 
	{
		*Target = CfixrunTargetNone;
	}

	return TRUE;
}

BOOL CfixrunParseCommandLine(
	__in UINT Argc,
	__in PCWSTR *Argv,
	__out PCFIXRUN_OPTIONS Options
	)
{
	PARSE_STATE State = StateExpectAny;
	PCWSTR *Value = NULL;
	UINT ArgIndex;

	ASSERT( Options->PrintConsole );

	for ( ArgIndex = 1; ArgIndex < Argc; ArgIndex++ )
	{
		size_t Len = wcslen( Argv[ ArgIndex ] );
		if ( Len == 0 )
		{
			continue;
		}

		if ( ArgIndex == Argc - 1 )
		{
			//
			// Last arg - always a value.
			//
			if ( State == StateExpectValue )
			{
				//
				// We are missing one value.
				//
				Options->PrintConsole( L"Expected value after '%s'\n", Argv[ ArgIndex - 1 ] );
				return FALSE;
			}

			Options->InputFileType = CfixrunInputDllOrDirectory;
			Options->InputFile = Argv[ ArgIndex ];
		}
		else if ( Argv[ ArgIndex ][ 0 ] == L'-' ||
			 Argv[ ArgIndex ][ 0 ] == L'/' )
		{
			//
			// A flag.
			//
			PCWSTR FlagName = &Argv[ ArgIndex ][ 1 ];
			if ( State == StateExpectValue )
			{
				Options->PrintConsole( L"Expected value after '%s'\n", Argv[ ArgIndex - 1 ] );
				return FALSE;
			}

			//
			// Test Fixture Selection Options.
			//
			if ( 0 == wcscmp( FlagName, L"r" ) )
			{
				Options->RecursiveSearch = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"n" ) )
			{
				Value = &Options->Fixture;
				State = StateExpectValue;
			}
			else if ( 0 == wcscmp( FlagName, L"p" ) )
			{
				Value = &Options->FixturePrefix;
				State = StateExpectValue;
			}

			//
			// Execution Options.
			//
			else if ( 0 == wcscmp( FlagName, L"f" ) )
			{
				Options->AbortOnFirstFailure = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"u" ) )
			{
				Options->DoNotCatchUnhandledExceptions = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"b" ) )
			{
				Options->AlwaysBreakOnFailure = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"d" ) )
			{
				Options->DisplayOnly = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"z" ) )
			{
				Options->Summary = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"y" ) )
			{
				Options->PauseAtEnd = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"Y" ) )
			{
				Options->PauseAtBeginning = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"kern" ) )
			{
				Options->EnableKernelFeatures = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"fsf" ) )
			{
				Options->ShortcutFixtureOnFailure = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"fsr" ) )
			{
				Options->ShortcutRunOnFailure = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"fss" ) )
			{
				Options->ShortcutRunOnSetupFailure = TRUE;
				State = StateExpectAny;
			}

			//
			// Output Options.
			//
			else if ( 0 == wcscmp( FlagName, L"out" ) )
			{
				Value = &Options->ProgressOutputTargetName;
				State = StateExpectValue;
			}
			else if ( 0 == wcscmp( FlagName, L"log" ) )
			{
				Value = &Options->LogOutputTargetName;
				State = StateExpectValue;
			}
			else if ( 0 == wcscmp( FlagName, L"nologo" ) )
			{
				Options->NoLogo = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"ts" ) )
			{
				Options->OmitSourceInfoInStackTrace = TRUE;
				State = StateExpectAny;
			}
			else
			{
				Options->PrintConsole( L"Unknown flag '%s'\n", Argv[ ArgIndex ] );
				return FALSE;
			}
		}
		else
		{
			//
			// A value.
			//
			if ( State != StateExpectValue )
			{
				Options->PrintConsole( L"Unexpected value '%s'\n", Argv[ ArgIndex ] );
				return FALSE;
			}

			ASSERT( Value != NULL );
			*Value = Argv[ ArgIndex ];
			State = StateExpectAny;
		}
	}

	if ( ! CfixrunsSetOutputTarget( 
		Options->LogOutputTargetName,
		FALSE,
		&Options->LogOutputTarget ) )
	{
		return FALSE;
	}
	
	if ( ! CfixrunsSetOutputTarget( 
		Options->ProgressOutputTargetName,
		TRUE,
		&Options->ProgressOutputTarget ) )
	{
		return FALSE;
	}

	return TRUE;	
}