/*----------------------------------------------------------------------
 * Purpose:
 *		Command line parsing.
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

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

typedef enum
{
	StateExpectFlag,
	StateExpectValue,
	StateExpectAny
} PARSE_STATE;

static int CfixcmdsPrintConsoleAndDebug(
	__in_z __format_string PCWSTR Format, 
	... 
	)
{
	WCHAR Buffer[ 256 ];

	va_list lst;
	va_start( lst, Format );
	( VOID ) StringCchVPrintfW(
		Buffer, 
		_countof( Buffer ),
		Format,
		lst );
	va_end( lst );
	
	OutputDebugString( Buffer );

	return wprintf( L"%s", Buffer );;
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
	WCHAR Trash[ 100 ];
	PCWSTR TrashValue = Trash;

	ASSERT( Options->PrintConsole == NULL );

	if ( IsDebuggerPresent() )
	{
		Options->PrintConsole = CfixcmdsPrintConsoleAndDebug;
	}
	else
	{
		Options->PrintConsole = wprintf;
	}

	//
	// Default, may be overridden during parsing.
	//
	Options->InputFileType = CfixrunInputDynamicallyLoadable;

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
			else if ( 0 == wcscmp( FlagName, L"exe" ) )
			{
				Options->InputFileType = CfixrunInputRequiresSpawn;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"fsf" ) )
			{
				Options->ShortCircuitFixtureOnFailure = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"fsr" ) )
			{
				Options->ShortCircuitRunOnFailure = TRUE;
				State = StateExpectAny;
			}
			else if ( 0 == wcscmp( FlagName, L"fss" ) )
			{
				Options->ShortCircuitRunOnSetupFailure = TRUE;
				State = StateExpectAny;
			}

			//
			// Output Options.
			//
			else if ( 0 == wcscmp( FlagName, L"out" ) )
			{
				Options->PrintConsole( L"Warning: Option -out has been removed and will be ignored.\n" );
				Value = &TrashValue;
				State = StateExpectValue;
			}
			else if ( 0 == wcscmp( FlagName, L"log" ) )
			{
				Options->PrintConsole( L"Warning: Option -log has been removed and will be ignored.\n" );
				Value = &TrashValue;
				State = StateExpectValue;
			}
			else if ( 0 == wcscmp( FlagName, L"eventdll" ) )
			{
				Value = &Options->EventDll;
				State = StateExpectValue;
			}
			else if ( 0 == wcscmp( FlagName, L"eventdlloptions" ) )
			{
				Value = &Options->EventDllOptions;
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
			else if ( 0 == wcscmp( FlagName, L"td" ) )
			{
				Options->DisableStackTraces = TRUE;
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

	if ( Options->InputFileType == CfixrunInputRequiresSpawn )
	{
		if ( Options->EnableKernelFeatures )
		{
			Options->PrintConsole( L"Cannot use -kern and -exe at the same time\n" );
			return FALSE;
		}
		else if ( Options->RecursiveSearch )
		{
			Options->PrintConsole( L"Cannot use -r and -exe at the same time\n" );
			return FALSE;
		}
		else if ( Options->PauseAtBeginning || Options->PauseAtEnd )
		{
			Options->PrintConsole( L"-y and -Y are currently not supported in "
				L"conjunction with -exe\n" );
			return FALSE;
		}
		else if ( ! CfixrunpIsExe( Options->InputFile ) )
		{
			Options->PrintConsole( L"-exe is only applicable for .exe files\n" );
			return FALSE;
		}
	}

	return TRUE;	
}