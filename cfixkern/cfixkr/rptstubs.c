/*----------------------------------------------------------------------
 *	Purpose:
 *		Report stubs as exposed to the test driver. Events
 *		are routed to a report channel.
 *
 *		N.B. Context parameters all refers to a PCFIXKRP_DRIVER_CONNECTION.
 * 
 *	Copyright:
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
#include <ntddk.h>
#include "cfixkrp.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <ntstrsafe.h>
#pragma warning( pop )

#ifndef MAX_USHORT
#define MAX_USHORT 0xffff
#endif

static VOID CfixkrsQueueEvent(
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__in_opt PCONTEXT CallerContext,
	__in CFIX_EVENT_TYPE Type,
	__in_opt PEXCEPTION_RECORD ExceptionRecord,
	__in_opt PCWSTR File,
	__in_opt PCWSTR Routine,
	__in_opt ULONG Line,
	__in_opt PCWSTR Expression,
	__in_opt PCWSTR Message
	)
{
	PCFIXKR_EXECUTION_EVENT Event;
	KIRQL OldIrql;
	ULONG StructureBaseSize;
	ULONG StructureTotalSize;

	size_t ExpressionLengthCb;
	size_t FileLengthCb;
	size_t MessageLengthCb;
	size_t RoutineLengthCb;

	USHORT Offset;
	PUCHAR StringsPointer;

	ASSERT( ( Type == CfixEventFailedAssertion && Expression ) ||
		    ( Type == CfixEventUncaughtException && ExceptionRecord ) ||
			( Type == CfixEventInconclusiveness && Message ) ||
			( Type == CfixEventLog && Message ) );

	//
	// Only one thread may write to the buffer at a time.
	//
	KeAcquireSpinLock( &Channel->EventBuffer.Lock, &OldIrql );

	//
	// First do the stuff common to all event types.
	//

	//
	// Check if the buffer is large enough to hold an event
	// with stack trace.
	//
	if ( Channel->EventBuffer.BufferSize - Channel->EventBuffer.BufferLength
		< RTL_SIZEOF_THROUGH_FIELD( 
			CFIXKR_EXECUTION_EVENT,
			StackTrace.Frames[ CFIXKRP_MAX_STACKFRAMES ] ) )
	{
		//
		// Too small.
		//
		Channel->EventBuffer.BufferTruncated = TRUE;
		goto Cleanup;
	}

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Channel->EventBuffer.Buffer + Channel->EventBuffer.BufferLength );

	//
	// Capture stacktrace and write it directly into the buffer.
	// Then, see how much space of the buffer has indeed been 
	// occupied.
	//
	if ( NT_SUCCESS( CfixkrpCaptureStackTrace(
		CallerContext,
		&Event->StackTrace,
		CFIXKRP_MAX_STACKFRAMES ) ) &&
		Event->StackTrace.FrameCount > 0 )
	{
		StructureBaseSize = FIELD_OFFSET(
			CFIXKR_EXECUTION_EVENT,
			StackTrace.Frames[ Event->StackTrace.FrameCount ] );
	}
	else
	{
		Event->StackTrace.FrameCount = 0;

		StructureBaseSize = sizeof( CFIXKR_EXECUTION_EVENT );
	}

	//
	// Calculate total size. 
	//
	// N.B. RtlStringCbLength is specified
	// (like all safestr-routines) as requiring IRQL == PASSIVE_LEVEL. 
	// Judging from source code, this seems to be overly restricting and 
	// calling it should be safe at elevated IRQL.
	//
	// N.B. BufferLength has not been modified,
	// so the event is still consistent when we return in case of 
	// a failure on the next lines.
	//
	if ( File == NULL )
	{
		FileLengthCb = 0;
	}
	else if ( ! NT_SUCCESS( RtlStringCbLengthW( File, MAX_USHORT, &FileLengthCb ) ) )
	{
		goto Cleanup;
	}

	if ( Routine == NULL )
	{
		RoutineLengthCb = 0;
	}
	else if ( ! NT_SUCCESS( RtlStringCbLengthW( Routine, MAX_USHORT, &RoutineLengthCb ) ) )
	{
		goto Cleanup;
	}

	if ( Message == NULL )
	{
		MessageLengthCb = 0;
	}
	else if ( ! NT_SUCCESS( RtlStringCbLengthW( Message, MAX_USHORT, &MessageLengthCb ) ) )
	{
		goto Cleanup;
	}

	if ( Expression == NULL )
	{
		ExpressionLengthCb = 0;
	}
	else if ( ! NT_SUCCESS( RtlStringCbLengthW( Expression, MAX_USHORT, &ExpressionLengthCb ) ) )
	{
		goto Cleanup;
	}

	//
	// N.B. No risk of overfow here, as all values are bounded. We just add
	// all values - those not applying to this event type will be 0.
	//
	ASSERT( StructureBaseSize < MAX_USHORT );

	StructureTotalSize = StructureBaseSize + 
		( ULONG ) ExpressionLengthCb + 
		( ULONG ) FileLengthCb + 
		( ULONG ) MessageLengthCb + 
		( ULONG ) RoutineLengthCb;

	ASSERT( StructureTotalSize < MAX_USHORT );

	//
	// Round up to multiple of 8.
	//
	StructureTotalSize = ( StructureTotalSize + 7 ) & ~7;

	ASSERT( StructureTotalSize < MAX_USHORT );

	//
	// Now that the total size is known, check buffer size again.
	//
	if ( Channel->EventBuffer.BufferSize - Channel->EventBuffer.BufferLength
		< StructureTotalSize )
	{
		//
		// Buffer too small.
		//
		// N.B. BufferLength has not been modified,
		// so the event is still consistent. Yet we signalize the
		// buffer shortage.
		//
		Channel->EventBuffer.BufferTruncated = TRUE;
		goto Cleanup;
	}
	else if ( StructureTotalSize > MAX_USHORT )
	{
		goto Cleanup;
	}

	Event->Type = Type;
	Event->Size = ( USHORT ) StructureTotalSize;

	Event->ThreadId.MainThreadId =
		Event->ThreadId.ThreadId = CfixkrGetCurrentThreadId();

	//
	// Write Info.
	//
	Offset = ( USHORT ) StructureBaseSize;
	StringsPointer = ( PUCHAR ) Event + Offset;

	switch ( Type )
	{
	case CfixEventFailedAssertion:
		Event->Info.FailedAssertion.Line				= Line;

		Event->Info.FailedAssertion.FileLength			= ( USHORT ) FileLengthCb;
		Event->Info.FailedAssertion.RoutineLength		= ( USHORT ) RoutineLengthCb;
		Event->Info.FailedAssertion.ExpressionLength	= ( USHORT ) ExpressionLengthCb;

		if ( FileLengthCb == 0 )
		{
			Event->Info.FailedAssertion.FileOffset		= 0;
		}
		else
		{
			Event->Info.FailedAssertion.FileOffset		= Offset;
			RtlCopyMemory( 
				StringsPointer,
				File,
				FileLengthCb );
			Offset			= Offset + ( USHORT ) FileLengthCb;
			StringsPointer	+= FileLengthCb;
		}

		if ( RoutineLengthCb == 0 )
		{
			Event->Info.FailedAssertion.RoutineOffset	= 0;
		}
		else
		{
			Event->Info.FailedAssertion.RoutineOffset	= Offset;
			RtlCopyMemory( 
				StringsPointer,
				Routine,
				RoutineLengthCb);
			Offset			= Offset + ( USHORT ) RoutineLengthCb;
			StringsPointer	+= RoutineLengthCb;
		}

		if ( ExpressionLengthCb == 0 )
		{
			Event->Info.FailedAssertion.ExpressionOffset = 0;
		}
		else
		{
			Event->Info.FailedAssertion.ExpressionOffset = Offset;
			RtlCopyMemory( 
				StringsPointer,
				Expression,
				ExpressionLengthCb );
			Offset			= Offset + ( USHORT ) ExpressionLengthCb;
			StringsPointer	+= ExpressionLengthCb;
		}

		break;
	case CfixEventUncaughtException:
		if ( ExceptionRecord )
		{
			Event->Info.UncaughtException.Type = CfixkrExceptionRecordDefault;
			Event->Info.UncaughtException.u.ExceptionRecord = *ExceptionRecord;
		}
		else
		{
			Event->Info.UncaughtException.Type = CfixkrExceptionRecordNone;
		}
		break;

	case CfixEventInconclusiveness:
	case CfixEventLog:
		//
		// N.B. Inconclusiveness and Log are overlaid.
		//
		Event->Info.Log.MessageLength					= ( USHORT ) MessageLengthCb;

		if ( MessageLengthCb == 0 )
		{
			Event->Info.Log.MessageOffset				= 0;
		}
		else
		{
			Event->Info.Log.MessageOffset				= Offset;
			RtlCopyMemory( 
				StringsPointer,
				Message,
				MessageLengthCb );
			Offset			= Offset + ( USHORT ) MessageLengthCb;
			StringsPointer	+= MessageLengthCb;
		}

		break;

	default:
		ASSERT( !"Unknown event type" );
	}

	ASSERT( Offset <= StructureTotalSize && Offset > StructureTotalSize - 8 );

	//
	// Commit the event by adjusting BufferLength.
	//
	Channel->EventBuffer.BufferLength += StructureTotalSize;
	Channel->EventBuffer.EventCount++;

Cleanup:
	KeReleaseSpinLock( &Channel->EventBuffer.Lock, OldIrql );

	return;
}

static VOID CfixkrsAbortCurrentTestCase(
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__in NTSTATUS Status
	)
{
	//
	// Abort the current test case by raising a status that will
	// be catched by CfixkrsExceptionFilter installed a few frames deeper.
	//
	if ( KeGetCurrentIrql() > PASSIVE_LEVEL )
	{
		//
		// ExRaiseStatus works at PASSIVE_LEVEL only. All test cases initially
		// start at PASSIVE_LEVEL, but the test case may have raised the IRQL.
		//
		// We have three options now:
		//  - return CfixContinue. Bad idea as the test case is 
		//    inconclusive/failed and will probably fail arbitrarily.
		//  - Bugcheck
		//  - Force IRQL back to PASSIVE_LEVEL (which it was at the beginning)
		//    Depending on what the test case did, that may or may not corrupt
		//    some state. Still, it seems to be the best choice for the time
		//    being.
		//
		CfixkrsQueueEvent(
				Channel,
				NULL,
				CfixEventLog,
				NULL,
				NULL,
				NULL,
				0,
				NULL,
				L"Testcase aborted at elevated IRQL. IRQL  "
				L"will be forced back to PASSIVE_LEVEL. Machine state might "
				L"become corrupted" );
		KeLowerIrql( PASSIVE_LEVEL );
	}

	ExRaiseStatus( Status );
}

static VOID CFIXCALLTYPE CfixkrsFailStub(
	__in PVOID Context
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) Context;
	PCFIXKRP_FILAMENT Filament;

	ASSERT( Conn );

	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Conn ) );
	if ( Filament != NULL )
	{
		CfixkrsAbortCurrentTestCase( 
			Filament->Channel, 
			EXCEPTION_TESTCASE_FAILED );
	}
	else
	{
		//
		// No Channel -> This routine must have been called from an 
		// unknown thread. 
		//
		DbgPrint( ( "CFIXKR: Fail on unknown thread.\n" ) );
	}
}

static CFIX_REPORT_DISPOSITION CfixkrsReportFailedAssertionStub(
	__in PVOID Context,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) Context;
	PCFIXKRP_FILAMENT Filament;

	ASSERT( Conn );

	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Conn ) );
	if ( Filament != NULL )
	{
		CFIX_REPORT_DISPOSITION Disp;
	
		CfixkrsQueueEvent(
			Filament->Channel,
			NULL,
			CfixEventFailedAssertion,
			NULL,
			File,
			Routine,
			Line,
			Expression,
			NULL );

		Disp = Filament->Channel->Dispositions.FailedAssertion;

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
			CfixkrsAbortCurrentTestCase( 
				Filament->Channel, 
				EXCEPTION_TESTCASE_FAILED_ABORT );
			
		}
		else if ( Disp == CfixContinue )
		{
			return Disp;
		}
		else // CfixBreak
		{
			if ( KD_DEBUGGER_ENABLED && ! KD_DEBUGGER_NOT_PRESENT )
			{
				return Disp;
			}
			else
			{
				CfixkrsAbortCurrentTestCase( 
					Filament->Channel, 
					EXCEPTION_TESTCASE_FAILED );
			}
		}
	}
	else
	{
		//
		// No Channel -> This routine must have been called from an 
		// unknown thread. Use default disposition.
		//
		DbgPrint( ( "CFIXKR: Failed assertion reported on unknown thread.\n" ) );
		return CfixContinue;
	}

#if DBG
	return CfixBreak;
#endif
}

static CFIX_REPORT_DISPOSITION CfixkrsReportFailedAssertionFormatStub(
	__in PVOID Context,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Format,
	__in va_list Args
	)
{
	WCHAR Buffer[ 200 ] = { 0 };
	PCWSTR Message;

	if ( Format == NULL )
	{
		Message = NULL;
	}
	else if ( KeGetCurrentIrql() == PASSIVE_LEVEL )
	{
		//
		// IRQL is low enough to format a message.
		//
		( VOID ) RtlStringCchVPrintfW(
			Buffer, 
			sizeof( Buffer ) / sizeof( WCHAR ) ,
			Format,
			Args );

		Message = Buffer;
	}
	else
	{
		//
		// Cannot format .
		//
		Message = Format;
	}

	return CfixkrsReportFailedAssertionStub(
		Context,
		File,
		Routine,
		Line,
		Message );
}

static CFIX_REPORT_DISPOSITION CfixkrsAssertEqualsUlongStub(
	__in PVOID Context,
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
		if ( KeGetCurrentIrql() == PASSIVE_LEVEL )
		{
			//
			// IRQL is low enough to format a message.
			//
			( VOID ) RtlStringCchPrintfW( 
				Buffer, 
				sizeof( Buffer ) / sizeof( WCHAR ),
				L"Comparison failed. Expected: 0x%08X, Actual: 0x%08X (Expression: %s)",
				Expected,
				Actual,
				Expression );

			Expression = Buffer;
		}
		else
		{
			//
			// Must leave Expression as is.
			//
		}

		return CfixkrsReportFailedAssertionStub(
			Context,
			File,
			Routine,
			Line,
			Expression );
	}
}

static VOID CfixkrsReportInconclusivenessStub(
	__in PVOID Context,
	__in PCWSTR Message
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) Context;
	PCFIXKRP_FILAMENT Filament;

	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Conn ) );
	if ( Filament != NULL )
	{
		CfixkrsQueueEvent(
			Filament->Channel,
			NULL,
			CfixEventInconclusiveness,
			NULL,
			NULL,
			NULL,
			0,
			NULL,
			Message != NULL
				? Message
				: L"" );

		CfixkrsAbortCurrentTestCase( 
			Filament->Channel, 
			EXCEPTION_TESTCASE_INCONCLUSIVE );

	}
	else
	{
		//
		// No Channel -> This routine must have been called from an 
		// unknown thread. Use default disposition.
		//
		DbgPrint( ( "CFIXKR: Inconclusiveness reported on unknown thread.\n" ) );
	}
}

static VOID CfixkrsReportLogStub(
	__in PVOID Context,
	__in PCWSTR Format,
	__in va_list Args
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) Context;
	PCFIXKRP_FILAMENT Filament;

	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Conn ) );
	if ( Filament != NULL )
	{
		WCHAR Buffer[ 200 ] = { 0 };
		PCWSTR Message;
		
		if ( Format == NULL )
		{
			Message = Buffer;
		}
		else if ( KeGetCurrentIrql() == PASSIVE_LEVEL )
		{
			//
			// IRQL is low enough to format a message.
			//
			( VOID ) RtlStringCchVPrintfW(
				Buffer, 
				sizeof( Buffer ) / sizeof( WCHAR ) ,
				Format,
				Args );

			Message = Buffer;
		}
		else
		{
			//
			// Cannot format .
			//
			Message = Format;
		}

		CfixkrsQueueEvent(
			Filament->Channel,
			NULL,
			CfixEventLog,
			NULL,
			NULL,
			NULL,
			0,
			NULL,
			Message );
	}
	else
	{
		//
		// No Channel -> This routine must have been called from an 
		// unknown thread. Use default disposition.
		//
		DbgPrint( ( "CFIXKR: Log on unknown thread.\n" ) );
	}
}

CFIX_REPORT_DISPOSITION CfixkrpReportUnhandledException(
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__in PEXCEPTION_POINTERS ExceptionPointers
	)
{
	if ( Channel != NULL )
	{
		CfixkrsQueueEvent(
			Channel,
			ExceptionPointers->ContextRecord,
			CfixEventUncaughtException,
			ExceptionPointers->ExceptionRecord,
			NULL,
			NULL,
			0,
			NULL,
			NULL );
	
		return Channel->Dispositions.UnhandledException;
	}
	else
	{
		//
		// No Channel -> This routine must have been called from an 
		// unknown thread. Use default disposition.
		//
		DbgPrint( ( "CFIXKR: Unhandled exception reported on unknown thread.\n" ) );
		return CfixContinue;
	}
}

EXCEPTION_DISPOSITION CfixkrpExceptionFilter(
	__in PEXCEPTION_POINTERS ExcpPointers,
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__out BOOLEAN *AbortRun
	)
{
	ULONG ExcpCode = ExcpPointers->ExceptionRecord->ExceptionCode;

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
	else
	{
		CFIX_REPORT_DISPOSITION Disp;

		Disp = CfixkrpReportUnhandledException( 
			Channel, 
			ExcpPointers );
		
		*AbortRun = ( BOOLEAN ) ( Disp == CfixAbort );
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

static PVOID CFIXCALLTYPE CfixkrsGetValueStub(
	__in PVOID Context,
	__in ULONG Tag
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) Context;
	PCFIXKRP_FILAMENT Filament;

	ASSERT( Conn );
	
	if ( Tag != 0 )
	{
		return NULL;
	}

	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Conn ) );
	if ( Filament != NULL && Tag == 0 )
	{
		return Filament->Channel->TlsValue;
	}
	else
	{
		//
		// No Channel/invalid tag.
		//
		return NULL;
	}
}

static VOID CFIXCALLTYPE CfixkrsSetValueStub(
	__in PVOID Context,
	__in ULONG Tag,
	__in PVOID Value
	)
{
	PCFIXKRP_DRIVER_CONNECTION Conn = ( PCFIXKRP_DRIVER_CONNECTION ) Context;
	PCFIXKRP_FILAMENT Filament;

	ASSERT( Conn );

	if ( Tag != 0 )
	{
		return;
	}

	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Conn ) );
	if ( Filament != NULL && Tag == 0  )
	{
		Filament->Channel->TlsValue = Value;
	}
	else
	{
		//
		// No Channel/invalid tag.
		//
	}
}

static NTSTATUS CFIXCALLTYPE CfixkrsCreateSystemThread(
    __in PVOID Context,
	__out PHANDLE ThreadHandle,
    __in ULONG DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt HANDLE ProcessHandle,
    __out_opt PCLIENT_ID ClientId,
    __in PKSTART_ROUTINE StartRoutine,
    __in PVOID StartContext,
	__in ULONG Flags
    )
{
	return CfixkrpCreateSystemThread(
		ThreadHandle,
		DesiredAccess,
		ObjectAttributes,
		ProcessHandle,
		ClientId,
		StartRoutine,
		StartContext,
		Flags,
		( PCFIXKRP_DRIVER_CONNECTION ) Context );
}

VOID CfixkrpGetReportSinkStubs(
	__out PCFIXKR_REPORT_SINK_METHODS Stubs
	)
{
	ASSERT( Stubs );

	Stubs->ReportFailedAssertion		= CfixkrsReportFailedAssertionStub;
	Stubs->AssertEqualsUlong			= CfixkrsAssertEqualsUlongStub;	
	Stubs->ReportInconclusiveness		= CfixkrsReportInconclusivenessStub;
	Stubs->ReportLog					= CfixkrsReportLogStub;	
}

VOID CfixkrpGetReportSinkStubs2(
	__out PCFIXKR_REPORT_SINK_METHODS_2 Stubs
	)
{
	ASSERT( Stubs );

	Stubs->ReportFailedAssertion		= CfixkrsReportFailedAssertionStub;
	Stubs->AssertEqualsUlong			= CfixkrsAssertEqualsUlongStub;	
	Stubs->ReportInconclusiveness		= CfixkrsReportInconclusivenessStub;
	Stubs->ReportLog					= CfixkrsReportLogStub;	
	Stubs->ReportFailedAssertionFormat	= CfixkrsReportFailedAssertionFormatStub;
	Stubs->Fail							= CfixkrsFailStub;
	Stubs->GetValue						= CfixkrsGetValueStub;
	Stubs->SetValue						= CfixkrsSetValueStub;
}

VOID CfixkrpGetReportSinkStubs3(
	__out PCFIXKR_REPORT_SINK_METHODS_3 Stubs
	)
{
	ASSERT( Stubs );

	Stubs->ReportFailedAssertion		= CfixkrsReportFailedAssertionStub;
	Stubs->AssertEqualsUlong			= CfixkrsAssertEqualsUlongStub;	
	Stubs->ReportInconclusiveness		= CfixkrsReportInconclusivenessStub;
	Stubs->ReportLog					= CfixkrsReportLogStub;	
	Stubs->ReportFailedAssertionFormat	= CfixkrsReportFailedAssertionFormatStub;
	Stubs->Fail							= CfixkrsFailStub;
	Stubs->GetValue						= CfixkrsGetValueStub;
	Stubs->SetValue						= CfixkrsSetValueStub;
	Stubs->CreateSystemThread			= CfixkrsCreateSystemThread;
}