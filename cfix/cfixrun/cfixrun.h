/*----------------------------------------------------------------------
 * Purpose:
 *		Cfixrun header. Defines routines for use by runtest.
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
#include <windows.h>
#include <cfixapi.h>
#include <crtdbg.h>

#define ASSERT _ASSERTE

#ifdef DBG
#define VERIFY ASSERT
#else
#define VERIFY( x ) ( ( VOID ) ( x ) )
#endif


#define CFIXRUN_EXIT_ALL_SUCCEEDED		0
#define CFIXRUN_EXIT_NONE_EXECUTED		1
#define CFIXRUN_EXIT_SOME_FAILED		2
#define CFIXRUN_EXIT_USAGE_FAILURE		3
#define CFIXRUN_EXIT_FAILURE			4

typedef enum
{
	CfixrunTargetConsole,
	CfixrunTargetDebug,
	CfixrunTargetFile,
	CfixrunTargetNone
} CFIXRUN_OUTPUT_TARGET;

typedef struct _CFIXRUN_OPTIONS
{
	PCWSTR DllOrDirectory;

	//
	// Test Fixture Selection Options.
	//
	BOOL RecursiveSearch;
	PCWSTR Fixture;
	PCWSTR FixturePrefix;

	//
	// Execution Options.
	//
	BOOL AbortOnFirstFailure;
	BOOL DoNotCatchUnhandledExceptions;
	BOOL AlwaysBreakOnFailure;
	BOOL DisplayOnly;
	BOOL Summary;
	BOOL PauseAtEnd;
	BOOL PauseAtBeginning;
	BOOL EnableKernelFeatures;

	//
	// Output Options.
	//
	PCWSTR ProgressOutputTargetName;
	PCWSTR LogOutputTargetName;
	BOOL NoLogo;
	BOOL OmitSourceInfoInStackTrace;
	
	CFIXRUN_OUTPUT_TARGET ProgressOutputTarget;
	CFIXRUN_OUTPUT_TARGET LogOutputTarget;

	int ( __cdecl * PrintConsole )(
		__in_z __format_string const wchar_t * _Format, 
		... 
		);

} CFIXRUN_OPTIONS, *PCFIXRUN_OPTIONS;

/*++
	Routine Description:
		Parse command line.
--*/
BOOL CfixrunParseCommandLine(
	__in UINT Argc,
	__in PCWSTR *Argv,
	__out PCFIXRUN_OPTIONS Options
	);

/*++
	Routine Description:
		Main function.

	Parameters:
		Options		Command line options.

		Return Value:
		Process exit status.
--*/
DWORD CfixrunMain(
	__in PCFIXRUN_OPTIONS Options
	);

typedef enum 
{
	CfixrunDll,
	CfixrunSys
} CFIXRUN_MODULE_TYPE;

/*++
	Routine Description:
		Callback for CfixrunSearchDlls

	Parameters:
		Path			Path of module found.
		Type			Type of module.
		Context			Caller supplied context
		SearchPerformed	Specifies whether an actual search was
						performed (FALSE when an exact file path
						was passed to CfixrunSearchDlls)

	Return Value:
		S_OK on success
		failure HRESULT on failure. Search will be aborted and
			return value is propagated to caller.
--*/
typedef HRESULT ( * CFIXRUN_VISIT_DLL_ROUTINE ) (
	__in PCWSTR Path,
	__in CFIXRUN_MODULE_TYPE Type,
	__in_opt PVOID Context,
	__in BOOL SearchPerformed
	);

/*++
	Routine Description:
		Search for DLLs (and optionally .sys files) in a directory.

	Parameters:
		Path			Path to a file or directory.
		Recursive		Search recursively?
		IncludeDrivers	Include .sys files?
		Routine			Callback.
		Context			Callback context argument.
--*/
HRESULT CfixrunSearchModules(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in BOOL IncludeDrivers,
	__in CFIXRUN_VISIT_DLL_ROUTINE Routine,
	__in_opt PVOID Context
	);