#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Cfix Kernel API. Interface between test drivers and cfixkr.
 *
 *		Shared header file. Do not include directly!
 *		 
 *		N.B. Include wdm.h before including this header. Do not use for
 *		user mode modules.
 *
 *            cfixaux.h        cfixkrio.h
 *              ^ ^ ^--------+     ^
 *             /   \          \   /
 *            /     \          \ /
 *		cfixapi.h  cfixpe.h  cfixkr.h
 *			^	  ^	  ^         
 *			|	 /	  |         
 *			|	/	  |         
 *		  [cfix]	cfix.h      
 *                    ^         
 *                    |         
 *                    |         
 *          [Test DLLs/Drivers] 
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
#include <initguid.h>
#include <cfixaux.h>
#include <cfixkrio.h>

#ifdef _WIN64
#define CFIXCALLTYPE
#else
#define CFIXCALLTYPE __stdcall
#endif

#define CFIXKR_DEVICE_NT_NAME	L"\\Device\\Cfixkr"
#define CFIXKR_DEVICE_DOS_NAME	L"\\DosDevices\\Cfixkr"

/*----------------------------------------------------------------------
 *
 * Report Sink interface exposed by cfixkr driver for use by test drivers.
 *
 */

/*++
	Interface GUID.
		1.1: {118F0A99-C318-45da-B55C-6E19A4E1240C}
--*/
DEFINE_GUID(GUID_CFIXKR_REPORT_SINK, 
	0x118f0a99, 0xc318, 0x45da, 0xb5, 0x5c, 0x6e, 0x19, 0xa4, 0xe1, 0x24, 0xc);

//
// Initial version (1.1).
//
#define CFIXKR_REPORT_SINK_VERSION_1 0x1000

//
// Extended version (1.2).
//
#define CFIXKR_REPORT_SINK_VERSION_2 0x2000

//
// Extended version (1.5).
//
#define CFIXKR_REPORT_SINK_VERSION_3 0x3000

/*++
	Routine Description:
		Report an event to the current execution context.

		Only for use from within a testcase.

		Callable at IRQL <= DISPATCH_LEVEL.
--*/
typedef CFIX_REPORT_DISPOSITION ( CFIXCALLTYPE * CFIXKR_REPORT_FAILED_ASSERTION_ROUTINE )(
	__in PVOID Context,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression
	);

/*++
	Routine Description:
		Report an event to the current execution context.

		Only for use from within a testcase.

		Callable at IRQL <= DISPATCH_LEVEL.
--*/
typedef CFIX_REPORT_DISPOSITION ( CFIXCALLTYPE * CFIXKR_REPORT_FAILED_ASSERTION_FORMAT_ROUTINE )(
	__in PVOID Context,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Format,
	__in va_list Args
	);

/*++
	Routine Description:
		Test Expected and Actual for equality. If the parameters do not
		equal, a failed assretion is reported.

		Only for use from within a testcase.

		Callable at IRQL <= DISPATCH_LEVEL.
--*/
typedef CFIX_REPORT_DISPOSITION ( CFIXCALLTYPE * CFIXKR_ASSERT_EQUALS_ULONG_ROUTINE )(
	__in PVOID Context,
	__in ULONG Expected,
	__in ULONG Actual,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression,
	__reserved ULONG Reserved
	);

/*++
	Routine Description:
		Fail current testcase.

		Only for use from within a testcase.

		Callable at IRQL <= DISPATCH_LEVEL.
--*/
typedef VOID ( CFIXCALLTYPE * CFIXKR_FAIL_ROUTINE )(
	__in PVOID Context
	);

/*++
	Routine Description:
		Report that testcase is inconclusive.

		Only for use from within a testcase.

		Callable at IRQL <= DISPATCH_LEVEL.
--*/
typedef VOID ( CFIXCALLTYPE * CFIXKR_REPORT_INCONCLUSIVENESS_ROUTINE )(
	__in PVOID Context,
	__in PCWSTR Message
	);

/*++
	Routine Description:
		Report log event.

		Only for use from within a testcase.

		Callable at IRQL <= DISPATCH_LEVEL.
--*/
typedef VOID ( CFIXCALLTYPE * CFIXKR_REPORT_LOG_ROUTINE )(
	__in PVOID Context,
	__in PCWSTR Format,
	__in va_list Args
	);

/*++
	Routine Description:
		Get thread local/test local value.

		Only for use from within a testcase.

		Callable at any IRQL.
--*/
typedef PVOID ( CFIXCALLTYPE * CFIXKR_GET_VALUE_ROUTINE )(
	__in PVOID Context,
	__in ULONG Tag
	);

/*++
	Routine Description:
		Set thread local/test local value.

		Only for use from within a testcase.

		Callable at any IRQL.
--*/
typedef VOID ( CFIXCALLTYPE * CFIXKR_SET_VALUE_ROUTINE )(
	__in PVOID Context,
	__in ULONG Tag,
	__in PVOID Value
	);

/*++
	Routine Description:
		Create system thread.
--*/
typedef NTSTATUS ( CFIXCALLTYPE * CFIXKR_CREATESYSTEMTHREAD )(
    __in PVOID Context,
	__out PHANDLE ThreadHandle,
    __in ULONG DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt HANDLE ProcessHandle,
    __out_opt PCLIENT_ID ClientId,
    __in PKSTART_ROUTINE StartRoutine,
    __in PVOID StartContext,
	__in ULONG Flags
    );

/*++
	Structure Description:
		See CFIXKR_REPORT_SINK_INTERFACE.
--*/
typedef struct _CFIXKR_REPORT_SINK_METHODS
{
	CFIXKR_REPORT_FAILED_ASSERTION_ROUTINE ReportFailedAssertion;
	CFIXKR_ASSERT_EQUALS_ULONG_ROUTINE AssertEqualsUlong;
	CFIXKR_REPORT_INCONCLUSIVENESS_ROUTINE ReportInconclusiveness;
	CFIXKR_REPORT_LOG_ROUTINE ReportLog;
} CFIXKR_REPORT_SINK_METHODS, *PCFIXKR_REPORT_SINK_METHODS;

typedef struct _CFIXKR_REPORT_SINK_METHODS_2
{
	CFIXKR_REPORT_FAILED_ASSERTION_ROUTINE ReportFailedAssertion;
	CFIXKR_ASSERT_EQUALS_ULONG_ROUTINE AssertEqualsUlong;
	CFIXKR_REPORT_INCONCLUSIVENESS_ROUTINE ReportInconclusiveness;
	CFIXKR_REPORT_LOG_ROUTINE ReportLog;
	CFIXKR_REPORT_FAILED_ASSERTION_FORMAT_ROUTINE ReportFailedAssertionFormat;
	CFIXKR_FAIL_ROUTINE Fail;
	CFIXKR_GET_VALUE_ROUTINE GetValue;
	CFIXKR_SET_VALUE_ROUTINE SetValue;
} CFIXKR_REPORT_SINK_METHODS_2, *PCFIXKR_REPORT_SINK_METHODS_2;

typedef struct _CFIXKR_REPORT_SINK_METHODS_3
{
	CFIXKR_REPORT_FAILED_ASSERTION_ROUTINE ReportFailedAssertion;
	CFIXKR_ASSERT_EQUALS_ULONG_ROUTINE AssertEqualsUlong;
	CFIXKR_REPORT_INCONCLUSIVENESS_ROUTINE ReportInconclusiveness;
	CFIXKR_REPORT_LOG_ROUTINE ReportLog;
	CFIXKR_REPORT_FAILED_ASSERTION_FORMAT_ROUTINE ReportFailedAssertionFormat;
	CFIXKR_FAIL_ROUTINE Fail;
	CFIXKR_GET_VALUE_ROUTINE GetValue;
	CFIXKR_SET_VALUE_ROUTINE SetValue;
	CFIXKR_CREATESYSTEMTHREAD CreateSystemThread;
} CFIXKR_REPORT_SINK_METHODS_3, *PCFIXKR_REPORT_SINK_METHODS_3;

/*++
	Structure Description:
		Interface between test driver and cfixkr.
--*/
typedef struct _CFIXKR_REPORT_SINK_INTERFACE
{
	INTERFACE Base;
	CFIXKR_REPORT_SINK_METHODS Methods;
} CFIXKR_REPORT_SINK_INTERFACE, *PCFIXKR_REPORT_SINK_INTERFACE;

typedef struct _CFIXKR_REPORT_SINK_INTERFACE_2
{
	INTERFACE Base;
	CFIXKR_REPORT_SINK_METHODS_2 Methods;
} CFIXKR_REPORT_SINK_INTERFACE_2, *PCFIXKR_REPORT_SINK_INTERFACE_2;

typedef struct _CFIXKR_REPORT_SINK_INTERFACE_3
{
	INTERFACE Base;
	CFIXKR_REPORT_SINK_METHODS_3 Methods;
} CFIXKR_REPORT_SINK_INTERFACE_3, *PCFIXKR_REPORT_SINK_INTERFACE_3;
