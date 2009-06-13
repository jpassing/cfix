#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Header file for inclusion by test code.
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

#ifdef CFIX_KERNELMODE
	#ifndef CFIX_NO_LIB
	#pragma comment( lib, "cfixkdrv" )
	#endif

	#include <wdm.h>

	//
	// Routines are statically linked.
	//
	#define CFIXREPORTAPI
#else
	#ifndef CFIX_NO_LIB
	#pragma comment( lib, "cfix" )
	#endif

	#define CFIXREPORTAPI CFIXAPI

	#include <windows.h>
#endif

/*----------------------------------------------------------------------
 *
 * Downlevel compiler compatibility.
 *
 * cl < 14.00 does not support SAL annotations.
 *
 */

#if _MSC_VER < 1400 
#define __in
#define __in_opt
#define __out
#define __out_ecount( x )
#define __out_opt
#define __inout
#define __inout_opt
#define __format_string
#define __reserved

#define _countof( Array ) ( sizeof( Array ) / sizeof( *Array ) )
#endif

#include <cfixpe.h>

#define CFIX_TAG_RESERVED_FOR_CC	0xFFFFFFFF

/*++
	Routine Description:
		Set a thread-local/test-local value. 

	Parameters:
		Reserved	- must be 0.
--*/
CFIXREPORTAPI VOID CFIXCALLTYPE CfixPeSetValue(
	__in ULONG Reserved,
	__in PVOID Value
	);

/*++
	Routine Description:
		Retrieve a thread-local/test-local value. If none has been set 
		yet, NULL is returned.

	Parameters:
		Reserved	- must be 0.
--*/
CFIXREPORTAPI PVOID CFIXCALLTYPE CfixPeGetValue(
	__in ULONG Reserved
	);

/*++
	Routine Description:
		Fail the current test case.
--*/
CFIXREPORTAPI VOID CFIXCALLTYPE CfixPeFail();

/*++
	Routine Description:
		Report an event to the current execution context.

		Only for use from within a testcase.
--*/
CFIXREPORTAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeReportFailedAssertion(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression
	);

/*++
	Routine Description:
		Report an event to the current execution context. Allows printf-
		style formatting.

		Only for use from within a testcase.
--*/
CFIXREPORTAPI CFIX_REPORT_DISPOSITION __cdecl CfixPeReportFailedAssertionFormatW(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in __format_string PCWSTR Format,
	...
	);

CFIXREPORTAPI CFIX_REPORT_DISPOSITION __cdecl CfixPeReportFailedAssertionFormatA(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in __format_string PCSTR Format,
	...
	);

/*++
	Routine Description:
		Test Expected and Actual for equality. If the parameters do not
		equal, a failed assretion is reported.

		Only for use from within a testcase.
--*/
CFIXREPORTAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeAssertEqualsUlong(
	__in ULONG Expected,
	__in ULONG Actual,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression,
	__reserved ULONG Reserved
	);

//
// Compat 1.0
//
#define CfixPeAssertEqualsDword CfixPeAssertEqualsUlong

/*++
	Routine Description:
		Report that testcase is inconclusive.

		Only for use from within a testcase.
--*/
CFIXREPORTAPI VOID CFIXCALLTYPE CfixPeReportInconclusiveness(
	__in __format_string PCWSTR Message
	);

CFIXREPORTAPI VOID CFIXCALLTYPE CfixPeReportInconclusivenessA(
	__in __format_string PCSTR Message
	);

/*++
	Routine Description:
		Report log event.

		Only for use from within a testcase.
--*/
CFIXREPORTAPI VOID __cdecl CfixPeReportLog(
	__in __format_string PCWSTR Format,
	...
	);

CFIXREPORTAPI VOID __cdecl CfixPeReportLogA(
	__in PCSTR Format,
	...
	);

/*----------------------------------------------------------------------
 *
 * Normal assertions.
 *
 */
#define CFIX_ASSERT_EXPR( expr, msg )						\
	( void ) ( ( !! ( expr ) ) ||							\
		( CfixBreak != CfixPeReportFailedAssertion(			\
			__CFIX_WIDE( __FILE__ ),						\
			__CFIX_WIDE( __FUNCTION__ ),					\
			__LINE__,										\
			msg ) ||										\
		( __debugbreak(), CfixPeFail(), 0 ) ) )

#define CFIX_ASSERT( expr )	CFIX_ASSERT_EXPR( expr, __CFIX_WIDE( #expr ) )

#define CFIX_ASSERT_EQUALS_DWORD( Expected, Actual )		\
	( void ) ( ( CfixBreak != CfixPeAssertEqualsDword(		\
			Expected,										\
			Actual,											\
			__CFIX_WIDE( __FILE__ ),						\
			__CFIX_WIDE( __FUNCTION__ ),					\
			__LINE__,										\
			__CFIX_WIDE( #Actual ),							\
			0 ) ||											\
		( __debugbreak(), CfixPeFail(), 0 ) ) )
#define CFIX_ASSERT_EQUALS_ULONG CFIX_ASSERT_EQUALS_DWORD

#if ! defined( UNICODE ) && ! defined( CFIX_KERNELMODE )
#define CFIX_INCONCLUSIVE( msg )							\
	CfixPeReportInconclusivenessA( msg )
#else
#define CFIX_INCONCLUSIVE( msg )							\
	CfixPeReportInconclusiveness( msg )
#endif

#if ! defined( UNICODE ) && ! defined( CFIX_KERNELMODE )
#define CFIX_LOG CfixPeReportLogA
#else
#define CFIX_LOG CfixPeReportLog
#endif

/*----------------------------------------------------------------------
 *
 * Formatted assertions (cl 14+ only).
 *
 */

#if _MSC_VER >= 1400 
#define CFIX_ASSERT_EXPR_FORMAT( ver, expr, format, ... )	\
	( void ) ( ( !! ( expr ) ) ||							\
	( CfixBreak != CfixPeReportFailedAssertionFormat##ver(	\
			__CFIX_WIDE( __FILE__ ),						\
			__CFIX_WIDE( __FUNCTION__ ),					\
			__LINE__,										\
			format,											\
			__VA_ARGS__ ) ||								\
		( __debugbreak(), CfixPeFail(), 0 ) ) )


#if ! defined( UNICODE ) && ! defined( CFIX_KERNELMODE )
#define CFIX_ASSERT_MESSAGE( expr, format, ... )			\
	CFIX_ASSERT_EXPR_FORMAT( A, expr, format, __VA_ARGS__ )
#else
#define CFIX_ASSERT_MESSAGE( expr, format, ... )			\
	CFIX_ASSERT_EXPR_FORMAT( W, expr, format, __VA_ARGS__ )
#endif

#endif // _MSC_VER

#ifndef CFIX_KERNELMODE
/*++
	Routine Description:
		Creates a thread like CreateThread does, but registers the
		thread appropriately s.t. assrertions, unhandled exceptions
		etc. can be properly handled by the framwwork.

		If the thread is aborted due to an unhandled exception, assertion
		etc, the thread's exit code is CFIX_E_THREAD_ABORTED.

	Parameters:
		See MSDN.
--*/
CFIXAPI HANDLE CFIXCALLTYPE CfixCreateThread(
	__in PSECURITY_ATTRIBUTES ThreadAttributes,
	__in SIZE_T StackSize,
	__in PTHREAD_START_ROUTINE StartAddress,
	__in PVOID Parameter,
	__in DWORD CreationFlags,
	__in PDWORD ThreadId
	);

#endif // CFIX_KERNELMODE
