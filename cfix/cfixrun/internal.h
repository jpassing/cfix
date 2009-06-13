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
#include <jpdiag.h>

typedef struct _CFIXRUN_STATE
{
	PCFIXRUN_OPTIONS Options;

	//
	// Custom formatter for all output.
	//
	PJPDIAG_FORMATTER Formatter;

	//
	// Message resolver - used both internally and by sessions.
	//
	PJPDIAG_MESSAGE_RESOLVER Resolver;

	//
	// Session for logging output.
	//
	JPDIAG_SESSION_HANDLE LogSession;

	//
	// Session for process output.
	//
	JPDIAG_SESSION_HANDLE ProgressSession;
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
		Output a debug message vis jpdiag.
--*/
HRESULT __cdecl CfixrunpOutputLogMessage(
	__in JPDIAG_SESSION_HANDLE Session,
	__in JPDIAG_SEVERITY_LEVEL Severity,
	__in PCWSTR Format,
	...
	);

/*++
	Routine Description:
		Create formatter for proper output formatting.
--*/
HRESULT CfixrunpCreateFormatter(
	__in PJPDIAG_MESSAGE_RESOLVER Resolver,
	__out PJPDIAG_FORMATTER *Formatter
	);

//
// Subtype as required by jpdiag event packet.
//
#define CFIXRUN_TEST_EVENT_PACKET_SUBTYPE 1

typedef enum
{
	CfixrunTestSuccess,
	CfixrunTestFailure,
	CfixrunTestInconclusive,
	CfixrunLog
} CFIXRUNS_EVENT_TYPE;

typedef struct _CFIXRUN_TEST_EVENT_DEBUG_INFO
{
	JPDIAG_DEBUG_INFO Base;
	WCHAR ModuleName[ 64 ];
	WCHAR FunctionName[ 100 ];
	WCHAR SourceFile[ 100 ];
} CFIXRUN_TEST_EVENT_DEBUG_INFO;

typedef struct _CFIXRUN_TEST_EVENT_PACKET
{
	JPDIAG_EVENT_PACKET Base;
	CFIXRUN_TEST_EVENT_DEBUG_INFO DebugInfo;

	CFIXRUNS_EVENT_TYPE EventType;

	WCHAR FixtureName[ 64 ];
	WCHAR TestCaseName[ 64 ];

	WCHAR Details[ 1024 ];

	DWORD LastError;
} CFIXRUN_TEST_EVENT_PACKET, *PCFIXRUN_TEST_EVENT_PACKET;

/*++
	Routine Description:
		Create and Output a test event vis jpdiag.
--*/
HRESULT CfixrunpOutputTestEvent(
	__in JPDIAG_SESSION_HANDLE Session,
	__in CFIXRUNS_EVENT_TYPE EventType,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in PCWSTR Details,
	__in PCWSTR ModuleName,
	__in PCWSTR FunctionName,
	__in PCWSTR SourceFile,
	__in UINT SourceLine,
	__in DWORD LastError
	);

/*++
	Routine Description:
		Create an execution context object that creates and handles
		jpdiag event log packets.
--*/
HRESULT JurunpCreateExecutionContext(
	__in PCFIXRUN_STATE State,
	__out PCFIX_EXECUTION_CONTEXT *Context
	);

/*++
	Routine Description:
		Delete an execution context object.
--*/
VOID JurunpDeleteExecutionContext(
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
VOID JurunpGetStatisticsExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context,
	__out PCFIXRUN_STATISTICS Statistics
	);