/*----------------------------------------------------------------------
 * Purpose:
 *		Event Sink.
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

#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <cfixevnt.h>
#include <crtdbg.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define ASSERT _ASSERTE

typedef struct _LOGFILE_EVENT_SINK
{
	CFIX_EVENT_SINK Base;

	volatile LONG ReferenceCount;

	FILE* File;

	struct
	{
		ULONG FailureCount;
		ULONG InconclusiveCount;
	} CurrentTestCase;
} LOGFILE_EVENT_SINK, *PLOGFILE_EVENT_SINK;


/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */

static HRESULT LogfileOpen(
	__in PCWSTR Path,
	__out FILE** File
	)
{
	HANDLE FileHandle = CreateFile(
		Path,
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		0,
		NULL );
	if ( File == INVALID_HANDLE_VALUE )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	int Fdes = _open_osfhandle( ( intptr_t ) FileHandle, _O_APPEND );
	if ( Fdes == -1 )
	{
		CloseHandle( FileHandle );
		return E_UNEXPECTED;
	}

	*File = _wfdopen( Fdes, L"a" );
	if ( ! *File )
	{
		_close( Fdes );
		return E_UNEXPECTED;
	}

	return S_OK;
}

static VOID LogfileFormatStackTrace(
	__in PCFIX_STACKTRACE StackTrace,
	__in BOOL ShowSourceInformation,
	__out PWSTR Buffer,
	__in SIZE_T BufferSizeInChars
	)
{
	UINT FrameIndex;

	ASSERT( StackTrace->FrameCount > 0 );
	ASSERT( StackTrace->GetInformationStackFrame );

	for ( FrameIndex = 0; FrameIndex < StackTrace->FrameCount; FrameIndex++ )
	{
		HRESULT Hr;

		WCHAR ModuleName[ 64 ];
		WCHAR FunctionName[ 100 ];
		WCHAR SourceFile[ 100 ];
		ULONG SourceLine;
		ULONG Displacement;

		WCHAR FrameBuffer[ 200 ];

		//
		// Resolve symbolic information.
		//
		if ( FAILED( ( StackTrace->GetInformationStackFrame ) (
			StackTrace->Frames[ FrameIndex ],
			_countof( ModuleName ),
			ModuleName,
			_countof( FunctionName ),
			FunctionName,
			( PDWORD ) &Displacement,
			_countof( SourceFile ),
			SourceFile,
			( PDWORD ) &SourceLine ) ) )
		{
			continue;
		}

		//
		// Add to buffer.
		//
		
		if ( SourceLine != 0 && ShowSourceInformation )
		{
			Hr = StringCchPrintf(
				FrameBuffer,
				_countof( FrameBuffer ),
				L"                 %s!%s +0x%x (%s:%d)\r\n",
				ModuleName,
				FunctionName,
				Displacement,
				SourceFile,
				SourceLine );
		}
		else
		{
			Hr = StringCchPrintf(
				FrameBuffer,
				_countof( FrameBuffer ),
				L"                 %s!%s +0x%x\r\n",
				ModuleName,
				FunctionName,
				Displacement );
		}

		if ( FAILED( Hr ) )
		{
			break;
		}

		if ( FAILED( StringCchCat(
			Buffer,
			BufferSizeInChars,
			FrameBuffer ) ) )
		{
			break;
		}
	}
}

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */

static VOID CFIXCALLTYPE LogfileReference(
	__in PCFIX_EVENT_SINK This
	)
{
	PLOGFILE_EVENT_SINK Sink = ( PLOGFILE_EVENT_SINK ) This;
	ASSERT( Sink );

	InterlockedIncrement( &Sink->ReferenceCount );
}

static VOID CFIXCALLTYPE LogfileDereference(
	__in PCFIX_EVENT_SINK This
	)
{
	PLOGFILE_EVENT_SINK Sink = ( PLOGFILE_EVENT_SINK ) This;
	ASSERT( Sink );

	if ( 0 == InterlockedDecrement( &Sink->ReferenceCount ) )
	{
		fclose( Sink->File );
		free( Sink );
	}
}

static VOID CFIXCALLTYPE LogfileReportEvent(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PLOGFILE_EVENT_SINK Sink = ( PLOGFILE_EVENT_SINK ) This;
	WCHAR StackTraceBuffer[ 2048 ] = { 0 };
	WCHAR LastErrorMessage[ 260 ] = { 0 };
	WCHAR ExceptionMessage[ 260 ] = { 0 };

	UNREFERENCED_PARAMETER( Thread );

	if ( Event->StackTrace.FrameCount > 0 )
	{
		LogfileFormatStackTrace( 
			&Event->StackTrace,
			TRUE,
			StackTraceBuffer,
			_countof( StackTraceBuffer ) );
	}


	switch ( Event->Type )
	{
	case CfixEventFailedAssertion:
		if ( Event->Info.FailedAssertion.Line )
		{
			fwprintf(
				Sink->File,
				L"[Failure]      %s.%s.%s \r\n"
				L"                 %s(%d): %s\r\n\r\n"
				L"                 Expression: %s\r\n"
				L"                 Last Error: %d\r\n\r\n"
				L"%s\r\n\r\n",
				ModuleBaseName,
				FixtureName,
				TestCaseName,
				Event->Info.FailedAssertion.File,
				Event->Info.FailedAssertion.Line,
				Event->Info.FailedAssertion.Routine,
				Event->Info.FailedAssertion.Expression,
				Event->Info.FailedAssertion.LastError,
				StackTraceBuffer );
		}
		else
		{
			fwprintf(
				Sink->File,
				L"[Failure]      %s.%s.%s \r\n"
				L"                 Expression: %s\r\n"
				L"                 Last Error: %d (%s)\r\n\r\n"
				L"%s\r\n\r\n",
				ModuleBaseName,
				FixtureName,
				TestCaseName,
				Event->Info.FailedAssertion.Expression,
				Event->Info.FailedAssertion.LastError,
				LastErrorMessage,
				StackTraceBuffer );
		}

		break;

	case CfixEventUncaughtException:
		( VOID ) StringCchPrintf(
			ExceptionMessage,
			_countof( ExceptionMessage ),
			L"Unhandled Exception 0x%08X at %p",
			Event->Info.UncaughtException.ExceptionRecord.ExceptionCode,
			Event->Info.UncaughtException.ExceptionRecord.ExceptionAddress );

		fwprintf(
			Sink->File,
			L"[Failure]      %s.%s.%s \r\n"
			L"                 %s\r\n\r\n"
			L"%s\r\n\r\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName,
			ExceptionMessage,
			StackTraceBuffer );

		break;

	case CfixEventInconclusiveness:	
		fwprintf(
			Sink->File,
			L"[Inconclusive] %s.%s.%s \r\n"
			L"                 %s\r\n\r\n"
			L"%s\r\n\r\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName,
			Event->Info.Inconclusiveness.Message,
			StackTraceBuffer );

		break;

	case CfixEventLog:
		fwprintf(
			Sink->File,
			L"[Log]          %s.%s.%s \r\n"
			L"                 %s\r\n\r\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName,
			Event->Info.Log.Message );
	}
}

static VOID CFIXCALLTYPE LogfileBeforeFixtureStart(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Thread );
	UNREFERENCED_PARAMETER( ModuleBaseName );
	UNREFERENCED_PARAMETER( FixtureName );

	//
	// Nop.
	//
}

static VOID CFIXCALLTYPE LogfileAfterFixtureFinish(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in BOOL RanToCompletion
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Thread );
	UNREFERENCED_PARAMETER( ModuleBaseName );
	UNREFERENCED_PARAMETER( FixtureName );
	UNREFERENCED_PARAMETER( RanToCompletion );

	//
	// Nop.
	//
}

static VOID CFIXCALLTYPE LogfileBeforeTestCaseStart(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName
	)
{
	PLOGFILE_EVENT_SINK Sink = ( PLOGFILE_EVENT_SINK ) This;

	UNREFERENCED_PARAMETER( Thread );
	UNREFERENCED_PARAMETER( ModuleBaseName );
	UNREFERENCED_PARAMETER( FixtureName );
	UNREFERENCED_PARAMETER( TestCaseName );
	
	//
	// Reset counters.
	//
	Sink->CurrentTestCase.FailureCount		= 0;
	Sink->CurrentTestCase.InconclusiveCount	= 0;
}

static VOID CFIXCALLTYPE LogfileAfterTestCaseFinish(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in BOOL RanToCompletion
	)
{
	PLOGFILE_EVENT_SINK Sink = ( PLOGFILE_EVENT_SINK ) This;

	UNREFERENCED_PARAMETER( Thread );
	UNREFERENCED_PARAMETER( ModuleBaseName );
	UNREFERENCED_PARAMETER( FixtureName );
	UNREFERENCED_PARAMETER( TestCaseName );
	UNREFERENCED_PARAMETER( RanToCompletion );

	//
	// Was is a success or failure?
	//
	if ( Sink->CurrentTestCase.FailureCount == 0 && 
		 Sink->CurrentTestCase.InconclusiveCount == 0 )
	{
		//
		// Success -> report.
		//
		
		fwprintf(
			Sink->File,
			L"[Success]      %s.%s.%s\r\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName );
	}
	else
	{
		//
		// Failure/Inconclusive, errors have already been reported.
		//
	}
}


/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */

CFIXAPI HRESULT CFIXCALLTYPE CreateEventSink(
	__in ULONG Version,
	__in ULONG Flags,
	__in_opt PCWSTR FilePath,
	__reserved ULONG Reserved,
	__out PCFIX_EVENT_SINK *Sink 
	)
{
	HRESULT Hr;
	PLOGFILE_EVENT_SINK NewSink;

	UNREFERENCED_PARAMETER( Reserved );
	UNREFERENCED_PARAMETER( Flags );

	if ( Version != CFIX_EVENT_SINK_VERSION )
	{
		return CFIX_E_UNSUPPORTED_EVENT_SINK_VERSION;
	}

	if ( ! Sink || ! FilePath )
	{
		return CFIX_E_EVENTDLL_INVALID_OPTIONS;
	}

	NewSink = ( PLOGFILE_EVENT_SINK ) malloc( sizeof( LOGFILE_EVENT_SINK ) );
	if ( ! NewSink )
	{
		return E_OUTOFMEMORY;
	}

	
	ZeroMemory( NewSink, sizeof( LOGFILE_EVENT_SINK ) );

	NewSink->ReferenceCount						= 1;
	NewSink->CurrentTestCase.FailureCount		= 0;
	NewSink->CurrentTestCase.InconclusiveCount	= 0;

	Hr = LogfileOpen( FilePath, &NewSink->File );
	if ( FAILED( Hr ) )
	{
		free( NewSink );
		return Hr;
	}

	NewSink->Base.Version				= CFIX_EVENT_SINK_VERSION;
	NewSink->Base.ReportEvent			= LogfileReportEvent;
	NewSink->Base.BeforeFixtureStart	= LogfileBeforeFixtureStart;
	NewSink->Base.AfterFixtureFinish	= LogfileAfterFixtureFinish;
	NewSink->Base.BeforeTestCaseStart	= LogfileBeforeTestCaseStart;
	NewSink->Base.AfterTestCaseFinish	= LogfileAfterTestCaseFinish;
	NewSink->Base.Reference				= LogfileReference;
	NewSink->Base.Dereference			= LogfileDereference;

	*Sink = &NewSink->Base;

	return S_OK;
}


