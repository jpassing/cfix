/*----------------------------------------------------------------------
 * Purpose:
 *		Internal header file.
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
#include "cfixrun.h"
#include <cdiag.h>

typedef struct _CFIXRUN_STATE
{
	PCFIXRUN_OPTIONS Options;

	//
	// Custom formatter for all output.
	//
	PCDIAG_FORMATTER Formatter;

	//
	// Message resolver - used both internally and by sessions.
	//
	PCDIAG_MESSAGE_RESOLVER Resolver;

	//
	// Session for logging output.
	//
	CDIAG_SESSION_HANDLE LogSession;

	//
	// Session for process output.
	//
	CDIAG_SESSION_HANDLE ProgressSession;
} CFIXRUN_STATE, *PCFIXRUN_STATE;

/*++
	Routine Description:
		Assemble action for displaying all testcases specified.
--*/
HRESULT CfixrunpAssembleDisplayAction( 
	__in PCFIXRUN_STATE State,
	__out PCFIX_ACTION *Action,
	__out PULONG FixtureCount
	);

/*++
	Routine Description:
		Assemble action for executing all testcases specified.
--*/
HRESULT CfixrunpAssembleExecutionAction( 
	__in PCFIXRUN_STATE State,
	__out PCFIX_ACTION *Action,
	__out PULONG FixtureCount
	);

/*++
	Routine Description:
		Output a debug message vis cdiag.
--*/
HRESULT __cdecl CfixrunpOutputLogMessage(
	__in CDIAG_SESSION_HANDLE Session,
	__in CDIAG_SEVERITY_LEVEL Severity,
	__in PCWSTR Format,
	...
	);

#define CFIXRUNP_FORMATTER_SHOW_STACKTRACE_SOURCE_INFORMATION 1

/*++
	Routine Description:
		Create formatter for proper output formatting.
--*/
HRESULT CfixrunpCreateFormatter(
	__in PCDIAG_MESSAGE_RESOLVER Resolver,
	__in DWORD Flags,
	__out PCDIAG_FORMATTER *Formatter
	);

//
// Subtype as required by cdiag event packet.
//
#define CFIXRUN_TEST_EVENT_PACKET_SUBTYPE 1

typedef enum
{
	CfixrunTestSuccess,
	CfixrunTestFailure,
	CfixrunTestInconclusive,
	CfixrunLog
} CFIXRUNS_EVENT_TYPE;

typedef struct _CFIXRUN_SOURCE_INFO
{
	WCHAR ModuleName[ 64 ];
	WCHAR FunctionName[ 100 ];
	WCHAR SourceFile[ 100 ];
	ULONG SourceLine;
} CFIXRUN_SOURCE_INFO, *PCFIXRUN_SOURCE_INFO;

typedef struct _CFIXRUN_TEST_EVENT_DEBUG_INFO
{
	CDIAG_DEBUG_INFO Base;
	CFIXRUN_SOURCE_INFO Source;
} CFIXRUN_TEST_EVENT_DEBUG_INFO;

typedef struct _CFIXRUN_TEST_EVENT_STACKFRAME
{
	CFIXRUN_SOURCE_INFO Source;
	UINT Displacement;
} CFIXRUN_TEST_EVENT_STACKFRAME, *PCFIXRUN_TEST_EVENT_STACKFRAME;

typedef struct _CFIXRUN_TEST_EVENT_PACKET
{
	CDIAG_EVENT_PACKET Base;
	CFIXRUN_TEST_EVENT_DEBUG_INFO DebugInfo;

	CFIXRUNS_EVENT_TYPE EventType;

	WCHAR FixtureName[ 64 ];
	WCHAR TestCaseName[ 64 ];

	WCHAR Details[ 1024 ];

	DWORD LastError;

	struct
	{
		UINT FrameCount;
		CFIXRUN_TEST_EVENT_STACKFRAME Frames[ ANYSIZE_ARRAY ];
	} StackTrace;
} CFIXRUN_TEST_EVENT_PACKET, *PCFIXRUN_TEST_EVENT_PACKET;

/*++
	Routine Description:
		Create and Output a test event vis cdiag.
--*/
HRESULT CfixrunpOutputTestEvent(
	__in CDIAG_SESSION_HANDLE Session,
	__in CFIXRUNS_EVENT_TYPE EventType,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in PCWSTR Details,
	__in PCWSTR ModuleName,
	__in PCWSTR FunctionName,
	__in PCWSTR SourceFile,
	__in UINT SourceLine,
	__in DWORD LastError,
	__in PCFIX_THREAD_ID ThreadId,
	__in_opt PCFIX_STACKTRACE StackTrace,
	__in_opt CFIX_GET_INFORMATION_STACKFRAME_ROUTINE GetInfoStackFrameRoutine
	);

/*++
	Routine Description:
		Create an execution context object that creates and handles
		cdiag event log packets.
--*/
HRESULT CfixrunpCreateExecutionContext(
	__in PCFIXRUN_STATE State,
	__out PCFIX_EXECUTION_CONTEXT *Context
	);

typedef struct _CFIXRUN_STATISTICS
{
	LONG Fixtures;
	LONG TestCases;
	LONG SucceededTestCases;
	LONG FailedTestCases;
	LONG InconclusiveTestCases;
} CFIXRUN_STATISTICS, *PCFIXRUN_STATISTICS;


/*++
	Routine Description:
		Fetch statistics about fixtures run.
--*/
VOID CfixrunpGetStatisticsExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context,
	__out PCFIXRUN_STATISTICS Statistics
	);


/*++
	Routine Description:
		Create Action displaying the contents of a fixture.

	Parameters:
		Fixture			Fixture to display.
		RunOptions		Required for output.
		Action			Result.
--*/
HRESULT CFIXCALLTYPE CfixrunpCreateDisplayAction(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIXRUN_OPTIONS RunOptions,
	__out PCFIX_ACTION *Action
	);

/*++
	Routine Description:
		Create an action for the given fixture.

	Parameters:
		TestCase	- ordinal of test case to include or -1 to include all.
--*/
typedef HRESULT ( CFIXCALLTYPE * CFIXRUNP_CREATE_ACTION_ROUTINE ) (
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__in ULONG TestCase,
	__out PCFIX_ACTION *Action
	);

/*++
	Routine Description:
		Decides whether to include a fixture in an action.

	Parameters:
		TestCase	- If the return value is true, this parameters is 
					  set to the ordinal of the test case to include
					  or -1 if the entire fixture should be included.
	Return Value:
		Indicater whether to include the fixture.
--*/
typedef BOOL ( CFIXCALLTYPE * CFIXRUNP_FILTER_FIXTURE_ROUTINE ) (
	__in PCFIX_FIXTURE Fixture,
	__in PVOID Context,
	__out PULONG TestCase
	);

/*++
	Routine Description:
		Search fixtures and assemble them into a sequence action.

	Parameters:
		DllOrDirectory	Path to search.
		RecursiveSearch	Recurse into subdirs?
		IncludeKernelM	Include kernel tests.
		LogSession		Session for diagnostic output.
		Callback		Callback for creating an action for each 
						fixture encountered.
		CallbackContext Context passed to callback.
		SequenceAction	Result. Contains one action per fixture.
--*/
HRESULT CfixrunpSearchFixturesAndCreateSequenceAction( 
	__in PCWSTR DllOrDirectory,
	__in BOOL RecursiveSearch,
	__in BOOL IncludeKernelModules,
	__in CDIAG_SESSION_HANDLE LogSession,
	__in CFIXRUNP_FILTER_FIXTURE_ROUTINE FilterCallback,
	__in CFIXRUNP_CREATE_ACTION_ROUTINE CreateActionCallback,
	__in PVOID CallbackContext,
	__out PCFIX_ACTION *SequenceAction
	);

/*++
	Routine Description:
		Create sequence action for a given module.

	Parameters:
		TestModule		Module to use.
		LogSession		Session for diagnostic output.
		Callback		Callback for creating an action for each 
						fixture encountered.
		CallbackContext Context passed to callback.
		SequenceAction	Result. Contains one action per fixture.
--*/
HRESULT CfixrunpCreateSequenceAction( 
	__in PCFIX_TEST_MODULE TestModule,
	__in CDIAG_SESSION_HANDLE LogSession,
	__in CFIXRUNP_FILTER_FIXTURE_ROUTINE FilterCallback,
	__in CFIXRUNP_CREATE_ACTION_ROUTINE CreateActionCallback,
	__in PVOID CallbackContext,
	__out PCFIX_ACTION *SequenceAction
	);

/*++
	Routine Description:
		Test whether a given path addresses a DLL file.
--*/
BOOL CfixrunpIsDll(
	__in PCWSTR Path
	);

/*++
	Routine Description:
		Test whether a given path addresses a SYS file.
--*/
BOOL CfixrunpIsSys(
	__in PCWSTR Path
	);

/*++
	Routine Description:
		Test whether a given path addresses a EXE file.
--*/
BOOL CfixrunpIsExe(
	__in PCWSTR Path
	);