/*----------------------------------------------------------------------
 * Purpose:
 *		Event reporting routines.
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

#include "cfixp.h"
#include <stdlib.h>
#include <windows.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */

CFIXAPI VOID CFIXCALLTYPE CfixPeFail()
{
	RaiseException(
		EXCEPTION_TESTCASE_FAILED,
		0,
		0,
		NULL );
	ASSERT( !"Will not make it here" );
}

CFIXAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeReportFailedAssertion(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression
	)
{
	PCFIX_EXECUTION_CONTEXT Context;
	CFIX_REPORT_DISPOSITION Disp;
	CFIXP_EVENT_WITH_STACKTRACE Event;
	HRESULT Hr;

	DWORD LastError = GetLastError();
	
	//
	// Context should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixpGetCurrentExecutionContext( &Context );
	if ( FAILED( Hr ) )
	{
		WCHAR Buffer[ 200 ] = { 0 };

		//
		// Failed assertion on unknown thread - there is little
		// we can do.
		//
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Failed assertion '%s' on unknown thread - terminating.",
			Expression );
		OutputDebugString( Buffer );
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	Event.Base.Type								= CfixEventFailedAssertion;
	Event.Base.Info.FailedAssertion.File		= File;
	Event.Base.Info.FailedAssertion.Routine		= Routine;
	Event.Base.Info.FailedAssertion.Line		= Line;
	Event.Base.Info.FailedAssertion.Expression	= Expression;
	Event.Base.Info.FailedAssertion.LastError	= LastError;

	//
	// Capture stacktrace.
	//
	if ( IsDebuggerPresent() || FAILED( CfixpCaptureStackTrace(
		NULL,
		&Event.Base.StackTrace,
		CFIXP_MAX_STACKFRAMES ) ) )
	{
		Event.Base.StackTrace.FrameCount = 0;
	}

	Disp = Context->ReportEvent(
		Context,
		&Event.Base );

	if ( Disp == CfixBreakAlways )
	{
		//
		// Will break, even if not running in a debugger.
		//
		return CfixBreak;
	}
	else if ( Disp == CfixAbort )
	{
		//
		// Throw exception to abort testcase. Will be catched by 
		// CfixsExceptionFilter installed a few frames deeper.
		//
		RaiseException(
			EXCEPTION_TESTCASE_FAILED_ABORT,
			0,
			0,
			NULL );

		ASSERT( !"Will not make it here" );
		return CfixAbort;
	}
	else if ( Disp == CfixContinue )
	{
		return Disp;
	}
	else // CfixBreak
	{
		if ( IsDebuggerPresent() )
		{
			return Disp;
		}
		else
		{
			CfixPeFail();

			//
			// Will not make it here.
			//
			return CfixAbort;
		}
	}
}

CFIXAPI CFIX_REPORT_DISPOSITION __cdecl CfixPeReportFailedAssertionFormatW(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in __format_string PCWSTR Format,
	...
	)
{
	WCHAR Buffer[ 256 ] = { 0 };

	if ( Format != NULL )
	{
		va_list lst;
		va_start( lst, Format );
		( VOID ) StringCchVPrintfW(
			Buffer, 
			_countof( Buffer ),
			Format,
			lst );
		va_end( lst );
	}

	return CfixPeReportFailedAssertion(
		File,
		Routine,
		Line,
		Buffer );
}

CFIXAPI CFIX_REPORT_DISPOSITION __cdecl CfixPeReportFailedAssertionFormatA(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in __format_string PCSTR Format,
	...
	)
{
	//
	// Create ANSI string first, as some of the arguments
	// may be ANSI.
	//
	CHAR AnsiBuffer[ 256 ] = { 0 };
	WCHAR Buffer[ 256 ] = { 0 };

	if ( Format != NULL )
	{
		va_list lst;
		va_start( lst, Format );
		( VOID ) StringCchVPrintfA(
			AnsiBuffer, 
			_countof( AnsiBuffer ),
			Format,
			lst );
		va_end( lst );

		//
		// Convert.
		//
		if ( ! MultiByteToWideChar(
			CP_ACP,
			0,
			AnsiBuffer,
			-1,
			Buffer,
			_countof( Buffer ) ) )
		{
			( VOID ) StringCchCopy(
				Buffer,
				_countof( Buffer ),
				L"(Invalid string)" );
		}
	}

	return CfixPeReportFailedAssertion(
		File,
		Routine,
		Line,
		Buffer );
}

CFIXAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeAssertEqualsUlong(
	__in ULONG Expected,
	__in ULONG Actual,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression,
	__reserved ULONG Reserved
	)
{
	UNREFERENCED_PARAMETER( Reserved );

	if ( Expected == Actual )
	{
		return CfixContinue;
	}
	else
	{
		WCHAR Buffer[ 200 ] = { 0 };
		
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Comparison failed. Expected: 0x%08X, Actual: 0x%08X (Expression: %s)",
			Expected,
			Actual,
			Expression );

		return CfixPeReportFailedAssertion(
			File,
			Routine,
			Line,
			Buffer );
	}
}

CFIXAPI VOID CFIXCALLTYPE CfixPeReportInconclusiveness(
	__in PCWSTR Message
	)
{
	PCFIX_EXECUTION_CONTEXT Context;
	CFIXP_EVENT_WITH_STACKTRACE Event;
	HRESULT Hr;
	
	//
	// Context should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixpGetCurrentExecutionContext( &Context );
	if ( FAILED( Hr ) )
	{
		WCHAR Buffer[ 200 ] = { 0 };

		//
		// Failed assertion on unknown thread - there is little
		// we can do.
		//
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Inconclusiveness report '%s' on unknown thread - terminating.",
			Message );
		OutputDebugString( Buffer );
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	//
	// Capture stacktrace.
	//
	if ( IsDebuggerPresent() || FAILED( CfixpCaptureStackTrace(
		NULL,
		&Event.Base.StackTrace,
		CFIXP_MAX_STACKFRAMES ) ) )
	{
		Event.Base.StackTrace.FrameCount = 0;
	}

	//
	// Report inconclusiveness.
	//
	Event.Base.Type							= CfixEventInconclusiveness;
	Event.Base.Info.Inconclusiveness.Message = Message;

	( VOID ) Context->ReportEvent(
		Context,
		&Event.Base );

	//
	// Throw exception to abort testcase. Will be catched by 
	// CfixsExceptionFilter installed a few frames deeper.
	//
	RaiseException(
		EXCEPTION_TESTCASE_INCONCLUSIVE,
		0,
		0,
		NULL );

	ASSERT( !"Will not make it here" );
}

CFIXAPI VOID CFIXCALLTYPE CfixPeReportInconclusivenessA(
	__in PCSTR Message
	)
{
	WCHAR Buffer[ 256 ] = { 0 };

	if ( Message != NULL )
	{
		//
		// Convert.
		//
		if ( ! MultiByteToWideChar(
			CP_ACP,
			0,
			Message,
			-1,
			Buffer,
			_countof( Buffer ) ) )
		{
			( VOID ) StringCchCopy(
				Buffer,
				_countof( Buffer ),
				L"(Invalid string)" );
		}
	}

	CfixPeReportInconclusiveness( Buffer );
}

static CFIXAPI VOID CfixsPeReportLog(
	__in PCWSTR Message
	)
{
	PCFIX_EXECUTION_CONTEXT Context;
	HRESULT Hr;
	CFIX_TESTCASE_EXECUTION_EVENT Event;

	//
	// Context should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixpGetCurrentExecutionContext( &Context );
	if ( FAILED( Hr ) )
	{
		WCHAR Buffer[ 200 ] = { 0 };

		//
		// Log on unknown thread - there is little
		// we can do.
		//
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Log report '%s' on unknown thread - terminating.",
			Message );
		OutputDebugString( Buffer );
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	//
	// Logs do not need a stacktrace.
	//
	Event.StackTrace.FrameCount = 0;

	//
	// Report log event.
	//
	Event.Type				= CfixEventLog;
	Event.Info.Log.Message	= Message;

	( VOID ) Context->ReportEvent(
		Context,
		&Event );
}

CFIXAPI VOID __cdecl CfixPeReportLog(
	__in __format_string PCWSTR Format,
	...
	)
{
	WCHAR Message[ 512 ] = { 0 };
	va_list lst;

	if ( Format != NULL )
	{
		//
		// Format message.
		//
		va_start( lst, Format );
		( VOID ) StringCchVPrintf(
			Message, 
			_countof( Message ),
			Format,
			lst );
		va_end( lst );
	}

	CfixsPeReportLog( Message );
}

CFIXAPI VOID __cdecl CfixPeReportLogA(
	__in __format_string PCSTR Format,
	...
	)
{
	CHAR AnsiMessage[ 512 ] = { 0 };
	WCHAR Message[ 512 ] = { 0 };
	va_list lst;

	if ( Format != NULL )
	{
		//
		// Format message.
		//
		va_start( lst, Format );
		( VOID ) StringCchVPrintfA(
			AnsiMessage, 
			_countof( AnsiMessage ),
			Format,
			lst );
		va_end( lst );

		//
		// Convert.
		//
		if ( ! MultiByteToWideChar(
			CP_ACP,
			0,
			AnsiMessage,
			-1,
			Message,
			_countof( Message ) ) )
		{
			( VOID ) StringCchCopy(
				Message,
				_countof( Message ),
				L"(Invalid string)" );
		}
	}
	
	CfixsPeReportLog( Message );
}