/*----------------------------------------------------------------------
 * Purpose:
 *		Main routine for exe embedding.
 *
 *		Note that all implementation code is located in jpurun.lib.
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

#include <stdlib.h>
#include <cfixapi.h>
#include <cfixrun.h>
#include <shellapi.h>

/*++
	Routine Description:
		This routine is used es embedding-init routine to run 
		exe-embedded tests using the regular cfix command line tools.

		The original command line is exptected to be passed in an
		environment variable whose name is defined by 
		CFIXRUN_EMB_CMDLINE_ENVVAR_NAME.

		N.B. This function never returns -- it exits the current
		process.
--*/
HRESULT CfixEmbMain()
{
	int Argc;
	PWSTR *Argv = NULL;
	WCHAR CommandLine[ 512 ];
	DWORD ExitCode;
	CFIXRUN_OPTIONS Options;

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );

	if ( 0 == GetEnvironmentVariable(
		CFIXRUN_EMB_CMDLINE_ENVVAR_NAME,
		CommandLine,
		_countof( CommandLine ) ) )
	{
		ExitCode = ( DWORD ) E_INVALIDARG;
		goto Exit;
	}

	Argv = CommandLineToArgvW( CommandLine, &Argc );
	if ( NULL == Argv )
	{
		ExitCode = GetLastError();
		goto Exit;
	}

	//
	// N.B. The command line should be valid as it has been pre-
	// validated by the command line tool already. Therefore, no
	// printing of usage information etc. is required here.
	//

	if ( Argc == 0 )
	{
		ExitCode = CFIXRUN_EXIT_USAGE_FAILURE;
	}
	else if ( ! CfixrunParseCommandLine( Argc, Argv, &Options ) )
	{
		ExitCode = CFIXRUN_EXIT_USAGE_FAILURE;
	}
	else
	{
		//
		// No 'drive not ready'-dialogs, please.
		//
		SetErrorMode( SetErrorMode( 0 ) | SEM_FAILCRITICALERRORS );

		ExitCode = CfixrunMain( &Options );
	}

Exit:
	if ( Argv )
	{
		LocalFree( Argv );
	}

	ExitProcess( ExitCode );
}