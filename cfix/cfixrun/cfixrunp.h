/*----------------------------------------------------------------------
 * Purpose:
 *		Internal header file.
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
		Display testcases, do not run.
--*/
HRESULT CfixrunpDisplayFixtures( 
	__in PCFIXRUN_STATE State
	);

/*++
	Routine Description:
		Run testcases.
--*/
HRESULT CfixrunpRunFixtures( 
	__in PCFIXRUN_STATE State,
	__out PDWORD ExitCode
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

/*++
	Routine Description:
		Delete an execution context object.
--*/
VOID CfixrunpDeleteExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context
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