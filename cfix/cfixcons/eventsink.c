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
#include <cdiag.h>
#include <cfixevnt.h>
#include <crtdbg.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define ASSERT _ASSERTE

typedef struct _CFIXCONS_EVENT_SINK
{
	CFIX_EVENT_SINK Base;

	volatile LONG ReferenceCount;

	//
	// Combination of CFIX_EVENT_SINK_FLAG_* flags.
	//
	ULONG Flags;

	PCDIAG_MESSAGE_RESOLVER Resolver;

	struct
	{
		ULONG FailureCount;
		ULONG InconclusiveCount;
	} CurrentTestCase;
} CFIXCONS_EVENT_SINK, *PCFIXCONS_EVENT_SINK;


/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */

static VOID CfixconssFormatStackTrace(
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
				L"                 %s!%s +0x%x (%s:%d)\n",
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
				L"                 %s!%s +0x%x\n",
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

static VOID CfixconssReference(
	__in PCFIX_EVENT_SINK This
	)
{
	PCFIXCONS_EVENT_SINK Sink = ( PCFIXCONS_EVENT_SINK ) This;
	ASSERT( Sink );

	InterlockedIncrement( &Sink->ReferenceCount );
}

static VOID CfixconssDereference(
	__in PCFIX_EVENT_SINK This
	)
{
	PCFIXCONS_EVENT_SINK Sink = ( PCFIXCONS_EVENT_SINK ) This;
	ASSERT( Sink );

	if ( 0 == InterlockedDecrement( &Sink->ReferenceCount ) )
	{
		Sink->Resolver->Dereference( Sink->Resolver );
		free( Sink );
	}
}

static VOID CfixconssResolveErrorMessage(
	__in PCDIAG_MESSAGE_RESOLVER Resolver,
	__in DWORD MessageCode,
	__out PWSTR Buffer,
	__in SIZE_T BufferSizeInChars
	)
{
	HRESULT Hr = Resolver->ResolveMessage(
		Resolver,
		MessageCode,
		CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS
			| CDIAG_MSGRES_STRIP_NEWLINES,
		NULL,
		BufferSizeInChars,
		Buffer );
	if ( CDIAG_E_UNKNOWN_MESSAGE == Hr )
	{
		( void ) StringCchCopy(
			Buffer,
			BufferSizeInChars,
			L"Unknown error" );
	}
	else if ( FAILED( Hr ) )
	{
		Buffer[ 0 ] = L'\0';
	}
	else
	{
		//
		// Succeeded.
		//
	}
}

static VOID CfixconssReportEvent(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in PCFIX_TESTCASE_EXECUTION_EVENT Event
	)
{
	PCFIXCONS_EVENT_SINK Sink = ( PCFIXCONS_EVENT_SINK ) This;
	WCHAR StackTraceBuffer[ 2048 ] = { 0 };
	WCHAR LastErrorMessage[ 260 ] = { 0 };
	WCHAR ExceptionMessage[ 260 ] = { 0 };

	UNREFERENCED_PARAMETER( Thread );

	if ( Event->StackTrace.FrameCount > 0 )
	{
		CfixconssFormatStackTrace( 
			&Event->StackTrace,
			Sink->Flags & CFIX_EVENT_SINK_FLAG_SHOW_STACKTRACE_SOURCE_INFORMATION,
			StackTraceBuffer,
			_countof( StackTraceBuffer ) );
	}


	switch ( Event->Type )
	{
	case CfixEventFailedAssertion:
		//
		// Get message for last error.
		//
		CfixconssResolveErrorMessage( 
			Sink->Resolver,
			Event->Info.FailedAssertion.LastError,
			LastErrorMessage,
			_countof( LastErrorMessage ) );

		if ( Event->Info.FailedAssertion.Line )
		{
			wprintf(
				L"[Failure]      %s.%s.%s \n"
				L"                 %s(%d): %s\n\n"
				L"                 Expression: %s\n"
				L"                 Last Error: %d (%s)\n\n"
				L"%s\n\n",
				ModuleBaseName,
				FixtureName,
				TestCaseName,
				Event->Info.FailedAssertion.File,
				Event->Info.FailedAssertion.Line,
				Event->Info.FailedAssertion.Routine,
				Event->Info.FailedAssertion.Expression,
				Event->Info.FailedAssertion.LastError,
				LastErrorMessage,
				StackTraceBuffer );
		}
		else
		{
			wprintf(
				L"[Failure]      %s.%s.%s \n"
				L"                 Expression: %s\n"
				L"                 Last Error: %d (%s)\n\n"
				L"%s\n\n",
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

		wprintf(
			L"[Failure]      %s.%s.%s \n"
			L"                 %s\n\n"
			L"%s\n\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName,
			ExceptionMessage,
			StackTraceBuffer );

		break;

	case CfixEventInconclusiveness:	
		wprintf(
			L"[Inconclusive] %s.%s.%s \n"
			L"                 %s\n\n"
			L"%s\n\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName,
			Event->Info.Inconclusiveness.Message,
			StackTraceBuffer );

		break;

	case CfixEventLog:
		wprintf(
			L"[Log]          %s.%s.%s \n"
			L"                 %s\n\n",
			ModuleBaseName,
			FixtureName,
			TestCaseName,
			Event->Info.Log.Message );
	}
}

static VOID CfixconssBeforeFixtureStart(
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

static VOID CfixconssAfterFixtureFinish(
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

static VOID CfixconssBeforeTestCaseStart(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName
	)
{
	PCFIXCONS_EVENT_SINK Sink = ( PCFIXCONS_EVENT_SINK ) This;

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

static VOID CfixconssAfterTestCaseFinish(
	__in PCFIX_EVENT_SINK This,
	__in PCFIX_THREAD_ID Thread,
	__in PCWSTR ModuleBaseName,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in BOOL RanToCompletion
	)
{
	PCFIXCONS_EVENT_SINK Sink = ( PCFIXCONS_EVENT_SINK ) This;

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
		
		wprintf(
			L"[Success]      %s.%s.%s\n",
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
	__in_opt PCWSTR Options,
	__reserved ULONG Reserved,
	__out PCFIX_EVENT_SINK *Sink 
	)
{
	PCFIXCONS_EVENT_SINK NewSink;
	HRESULT Hr;

	UNREFERENCED_PARAMETER( Reserved );

	if ( Version != CFIX_EVENT_SINK_VERSION )
	{
		return CFIX_E_UNSUPPORTED_EVENT_SINK_VERSION;
	}

	if ( ! Sink )
	{
		return E_INVALIDARG;
	}

	if ( Options != NULL )
	{
		return E_INVALIDARG;
	}

	NewSink = malloc( sizeof( CFIXCONS_EVENT_SINK ) );
	if ( ! NewSink )
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory( NewSink, sizeof( CFIXCONS_EVENT_SINK ) );

	NewSink->ReferenceCount						= 1;
	NewSink->Flags								= Flags;
	NewSink->CurrentTestCase.FailureCount		= 0;
	NewSink->CurrentTestCase.InconclusiveCount	= 0;

	Hr = CdiagCreateMessageResolver( &NewSink->Resolver );
	if ( FAILED( Hr ) )
	{
		free( NewSink );
		return Hr;
	}

	NewSink->Base.Version				= CFIX_EVENT_SINK_VERSION;
	NewSink->Base.ReportEvent			= CfixconssReportEvent;
	NewSink->Base.BeforeFixtureStart	= CfixconssBeforeFixtureStart;
	NewSink->Base.AfterFixtureFinish	= CfixconssAfterFixtureFinish;
	NewSink->Base.BeforeTestCaseStart	= CfixconssBeforeTestCaseStart;
	NewSink->Base.AfterTestCaseFinish	= CfixconssAfterTestCaseFinish;
	NewSink->Base.Reference				= CfixconssReference;
	NewSink->Base.Dereference			= CfixconssDereference;

	*Sink = &NewSink->Base;

	return S_OK;
}