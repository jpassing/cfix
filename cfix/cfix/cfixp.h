#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Cfix main header file.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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

#include <cfixapi.h>
#include <cfixpe.h>
#include <crtdbg.h>

#define ASSERT _ASSERTE

#ifdef DBG
#define VERIFY ASSERT
#else
#define VERIFY( x ) ( ( VOID ) ( x ) )
#endif

#define NOP ( ( VOID ) 0 )

/*++
	Routines to be called from DllMain.
--*/
BOOL CfixpSetupContextTls();
BOOL CfixpSetupTestTls();
BOOL CfixpSetupStackTraceCapturing();

BOOL CfixpTeardownContextTls();
BOOL CfixpTeardownTestTls();
VOID CfixpTeardownStackTraceCapturing();

/*----------------------------------------------------------------------
 *
 * Stack trace handling.
 *
 */

/*++
	Routine Description:
		Capture the stack trace based on the given CONTEXT.

	Parameters:
		Context		Thread context. If NULL, the current context is
					used.
		Event		Event structure to populate.
		MaxFrames	Max number of frames the structure can hold.
--*/
HRESULT CfixpCaptureStackTrace(
	__in_opt CONST PCONTEXT Context,
	__in PCFIX_STACKTRACE StackTrace,
	__in UINT MaxFrames 
	);

//
// 64 (63+1) frames ought to be enough... otherwise the trace will
// be cropped.
//
#define CFIXP_MAX_STACKFRAMES 64

typedef struct _CFIXP_EVENT_WITH_STACKTRACE
{
	CFIX_TESTCASE_EXECUTION_EVENT Base;
	ULONGLONG __AdditionalFrames[ CFIXP_MAX_STACKFRAMES - 1 ];
} CFIXP_EVENT_WITH_STACKTRACE, *PCFIXP_EVENT_WITH_STACKTRACE;

HRESULT CFIXCALLTYPE CfixpGetInformationStackframe(
	__in ULONGLONG Frame,
	__in SIZE_T ModuleNameCch,
	__out_ecount(ModuleNameCch) PWSTR ModuleName,
	__in SIZE_T FunctionNameCch,
	__out_ecount(FunctionNameCch) PWSTR FunctionName,
	__out PDWORD Displacement,
	__in SIZE_T SourceFileCch,
	__out_ecount(SourceFileCch) PWSTR SourceFile,
	__out PDWORD SourceLine 
	);

/*----------------------------------------------------------------------
 *
 * Execution context.
 *
 */

/*++
	Routine Description:
		Obtain the execution context for the current thread.

		Can be used to report events, although usage of the 
		wrapper functions is preferred.

	Parameters:
		Context		- context to set.
		PrevContext - context that has been set before. Can be used
					  to stack/unstack contexts.
--*/
HRESULT CfixpSetCurrentExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context,
	__out_opt PCFIX_EXECUTION_CONTEXT *PrevContext
	);

/*++
	Routine Description:
		Obtain the execution context for the current thread.

		Can be used to report events, although usage of the 
		wrapper functions is preferred.

	Parameters:
		Context		- current context.
--*/
HRESULT CfixpGetCurrentExecutionContext(
	__out PCFIX_EXECUTION_CONTEXT *Context 
	);