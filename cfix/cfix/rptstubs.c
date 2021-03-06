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

DWORD CfixpExceptionFilter(
	__in PEXCEPTION_POINTERS ExcpPointers,
	__in PCFIXP_FILAMENT Filament,
	__out PBOOL AbortRun
	)
{
	DWORD ExcpCode = ExcpPointers->ExceptionRecord->ExceptionCode;
	CFIX_THREAD_ID ThreadId;

	CfixpInitializeThreadId( 
		&ThreadId,
		Filament->MainThreadId,
		GetCurrentThreadId() );

	if ( EXCEPTION_TESTCASE_INCONCLUSIVE == ExcpCode ||
		 EXCEPTION_TESTCASE_FAILED == ExcpCode )
	{
		//
		// Testcase failed/turned out to be inconclusive.
		//
		*AbortRun = FALSE;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else if ( EXCEPTION_TESTCASE_FAILED_ABORT == ExcpCode )
	{
		//
		// Testcase failed and is to be aborted.
		//
		*AbortRun = TRUE;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else if ( EXCEPTION_BREAKPOINT == ExcpCode )
	{
		//
		// May happen when CfixBreakAlways was used. Do not handle
		// the exception, s.t. the debugger gets the opportunity to
		// attach.
		//
		return EXCEPTION_CONTINUE_SEARCH; 
	}
	else if ( EXCEPTION_STACK_OVERFLOW == ExcpCode )
	{
		//
		// I will not handle that one.
		//
		return EXCEPTION_CONTINUE_SEARCH; 
	}
	else
	{
		CFIX_REPORT_DISPOSITION Disp;
		CFIXP_EVENT_WITH_STACKTRACE Event;

		//
		// Notify.
		//
		Filament->ExecutionContext->OnUnhandledException(
			Filament->ExecutionContext,
			&ThreadId,
			ExcpPointers );

		//
		// Capture stacktrace.
		//
		if ( ! CfixpFlagOn( Filament->Flags, CFIXP_FILAMENT_FLAG_CAPTURE_STACK_TRACES ) || 
			FAILED( CfixpCaptureStackTrace(
				ExcpPointers->ContextRecord,
				&Event.Base.StackTrace,
				CFIXP_MAX_STACKFRAMES ) ) )
		{
			Event.Base.StackTrace.FrameCount = 0;
		}

		//
		// Report unhandled exception.
		//
		Event.Base.Type = CfixEventUncaughtException;
		memcpy( 
			&Event.Base.Info.UncaughtException,
			ExcpPointers->ExceptionRecord,
			sizeof( EXCEPTION_RECORD ) );

		Disp = Filament->ExecutionContext->ReportEvent(
			Filament->ExecutionContext,
			&ThreadId,
			&Event.Base );

		*AbortRun = ( Disp == CfixAbort );
		if ( Disp == CfixBreak )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
		else
		{
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}
}

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */

CFIXAPI VOID CFIXCALLTYPE CfixPeFail()
{
	PCFIXP_FILAMENT Filament;
	BOOL FilamentDerivedFromDefaultFilament;
	
	HRESULT Hr = CfixpGetCurrentFilament( 
		&Filament, 
		&FilamentDerivedFromDefaultFilament );
	
	if ( SUCCEEDED( Hr ) && ! FilamentDerivedFromDefaultFilament )
	{
		RaiseException(
			EXCEPTION_TESTCASE_FAILED,
			0,
			0,
			NULL );
	}
	else
	{
		//
		// No CfixpExceptionFilter installed as this is 
		// an anonymous thread that has been auto-registered.
		//
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	ASSERT( !"Will not make it here" );
}

CFIXAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeReportFailedAssertion(
	__in_opt PCWSTR File,
	__in_opt PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression
	)
{
	CFIX_REPORT_DISPOSITION Disp;
	CFIXP_EVENT_WITH_STACKTRACE Event;
	PCFIXP_FILAMENT Filament;
	BOOL FilamentDerivedFromDefaultFilament;
	HRESULT Hr;
	CFIX_THREAD_ID ThreadId;

	DWORD LastError = GetLastError();

	//
	// Filament should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixpGetCurrentFilament( 
		&Filament, 
		&FilamentDerivedFromDefaultFilament );
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

	CfixpInitializeThreadId( 
		&ThreadId,
		Filament->MainThreadId,
		GetCurrentThreadId() );
	
	Event.Base.Type								= CfixEventFailedAssertion;
	Event.Base.Info.FailedAssertion.File		= File;
	Event.Base.Info.FailedAssertion.Routine		= Routine;
	Event.Base.Info.FailedAssertion.Line		= Line;
	Event.Base.Info.FailedAssertion.Expression	= Expression;
	Event.Base.Info.FailedAssertion.LastError	= LastError;

	//
	// Capture stacktrace.
	//
	if ( ! CfixpFlagOn( Filament->Flags, CFIXP_FILAMENT_FLAG_CAPTURE_STACK_TRACES ) || 
		 FAILED( CfixpCaptureStackTrace(
			NULL,
			&Event.Base.StackTrace,
			CFIXP_MAX_STACKFRAMES ) ) )
	{
		Event.Base.StackTrace.FrameCount = 0;
	}

	Disp = Filament->ExecutionContext->ReportEvent(
		Filament->ExecutionContext,
		&ThreadId,
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
		if ( ! FilamentDerivedFromDefaultFilament )
		{
			//
			// Throw exception to abort testcase. Will be catched by 
			// CfixpExceptionFilter installed a few frames deeper.
			//
			RaiseException(
				EXCEPTION_TESTCASE_FAILED_ABORT,
				0,
				0,
				NULL );
		}
		else
		{
			//
			// No CfixpExceptionFilter installed as this is 
			// an anonymous thread that has been auto-registered.
			//
			ExitThread( CFIX_EXIT_THREAD_ABORTED );
		}

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
	__in_opt __format_string PCWSTR Format,
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
	__in_opt __format_string PCSTR Format,
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
	CFIXP_EVENT_WITH_STACKTRACE Event;
	PCFIXP_FILAMENT Filament;
	BOOL FilamentDerivedFromDefaultFilament;
	HRESULT Hr;
	CFIX_THREAD_ID ThreadId;

	//
	// Filament should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixpGetCurrentFilament( 
		&Filament, 
		&FilamentDerivedFromDefaultFilament );
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

	CfixpInitializeThreadId( 
		&ThreadId,
		Filament->MainThreadId,
		GetCurrentThreadId() );

	//
	// Capture stacktrace.
	//
	if ( ! CfixpFlagOn( Filament->Flags, CFIXP_FILAMENT_FLAG_CAPTURE_STACK_TRACES ) || 
		 FAILED( CfixpCaptureStackTrace(
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

	( VOID ) Filament->ExecutionContext->ReportEvent(
		Filament->ExecutionContext,
		&ThreadId,
		&Event.Base );

	if ( ! FilamentDerivedFromDefaultFilament )
	{
		//
		// Throw exception to abort testcase. Will be catched by 
		// CfixpExceptionFilter installed a few frames deeper.
		//
		RaiseException(
			EXCEPTION_TESTCASE_INCONCLUSIVE,
			0,
			0,
			NULL );
	}
	else
	{
		//
		// No CfixpExceptionFilter installed as this is 
		// an anonymous thread that has been auto-registered.
		//
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

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
	CFIX_TESTCASE_EXECUTION_EVENT Event;
	PCFIXP_FILAMENT Filament;
	HRESULT Hr;
	CFIX_THREAD_ID ThreadId;

	//
	// Filament should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixpGetCurrentFilament( &Filament, NULL );
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

	CfixpInitializeThreadId( 
		&ThreadId,
		Filament->MainThreadId,
		GetCurrentThreadId() );

	//
	// Logs do not need a stacktrace.
	//
	Event.StackTrace.FrameCount = 0;

	//
	// Report log event.
	//
	Event.Type				= CfixEventLog;
	Event.Info.Log.Message	= Message;

	( VOID ) Filament->ExecutionContext->ReportEvent(
		Filament->ExecutionContext,
		&ThreadId,
		&Event );
}

CFIXAPI VOID __cdecl CfixPeReportLog(
	__in_opt __format_string PCWSTR Format,
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
	__in_opt __format_string PCSTR Format,
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