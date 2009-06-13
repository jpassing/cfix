/*----------------------------------------------------------------------
 * Purpose:
 *		Custom formatter.
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
#include "cfixrunp.h"
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define FORMATTER_LOG_TEMPLATE L"[Log] %Severity (%ProcessId:%ThreadId): %Message"

typedef struct _FORMATTER
{
	CDIAG_FORMATTER Base;

	//
	// Let other formatters do the real work.
	//
	PCDIAG_FORMATTER LogFormatter;
	PCDIAG_MESSAGE_RESOLVER Resolver;

	volatile LONG ReferenceCount;

	DWORD Flags;
} FORMATTER, *PFORMATTER;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static VOID CfixrunsDeleteFormatter(
	__in PFORMATTER Formatter
	)
{
	Formatter->LogFormatter->Dereference( Formatter->LogFormatter );
	Formatter->Resolver->Dereference( Formatter->Resolver );

	free( Formatter );
}

static VOID CfixrunsReferenceFormatter(
	__in PCDIAG_FORMATTER This
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	InterlockedIncrement( &Formatter->ReferenceCount );
}

static VOID CfixrunsDereferenceFormatter(
	__in PCDIAG_FORMATTER This
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	if ( 0 == InterlockedDecrement( &Formatter->ReferenceCount ) )
	{
		CfixrunsDeleteFormatter( Formatter );
	}
}

static VOID CfixrunsResolveErrorMessage(
	__in PFORMATTER Formatter,
	__in DWORD MessageCode,
	__out PWSTR Buffer,
	__in SIZE_T BufferSizeInChars
	)
{
	HRESULT Hr = Formatter->Resolver->ResolveMessage(
		Formatter->Resolver,
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

static VOID CfixrunsFormatStackTrace(
	__in PCFIXRUN_TEST_EVENT_PACKET Packet,
	__in BOOL ShowSourceInformation,
	__out PWSTR Buffer,
	__in SIZE_T BufferSizeInChars
	)
{
	WCHAR FrameBuffer[ 200 ];
	UINT FrameIndex;
	for ( FrameIndex = 0; FrameIndex < Packet->StackTrace.FrameCount; FrameIndex++ )
	{
		HRESULT Hr;

		if ( Packet->StackTrace.Frames[ FrameIndex ].Source.SourceLine != 0 &&
			 ShowSourceInformation )
		{
			Hr = StringCchPrintf(
				FrameBuffer,
				_countof( FrameBuffer ),
				L"                 %s!%s +0x%x (%s:%d)\n",
				Packet->StackTrace.Frames[ FrameIndex ].Source.ModuleName,
				Packet->StackTrace.Frames[ FrameIndex ].Source.FunctionName,
				Packet->StackTrace.Frames[ FrameIndex ].Displacement,
				Packet->StackTrace.Frames[ FrameIndex ].Source.SourceFile,
				Packet->StackTrace.Frames[ FrameIndex ].Source.SourceLine );
		}
		else
		{
			Hr = StringCchPrintf(
				FrameBuffer,
				_countof( FrameBuffer ),
				L"                 %s!%s +0x%x\n",
				Packet->StackTrace.Frames[ FrameIndex ].Source.ModuleName,
				Packet->StackTrace.Frames[ FrameIndex ].Source.FunctionName,
				Packet->StackTrace.Frames[ FrameIndex ].Displacement );
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

static HRESULT CfixrunsFormat(
	__in PCDIAG_FORMATTER This,
	__in CONST PCDIAG_EVENT_PACKET EventPkt,
	__in SIZE_T BufferSizeInChars,
	__out PWSTR Buffer
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	WCHAR LastErrorMessage[ 200 ] = { 0 };
	WCHAR StackTraceBuffer[ 1536 ] = { 0 };

	if ( EventPkt->Type == CdiagCustomEvent &&
		 EventPkt->SubType == CFIXRUN_TEST_EVENT_PACKET_SUBTYPE )
	{
		PCFIXRUN_TEST_EVENT_PACKET Packet = CONTAINING_RECORD(
			EventPkt,
			CFIXRUN_TEST_EVENT_PACKET,
			Base );

		ASSERT( EventPkt->TotalSize >= sizeof( CFIXRUN_TEST_EVENT_PACKET ) );

		if ( Packet->StackTrace.FrameCount > 0 )
		{
			CfixrunsFormatStackTrace( 
				Packet,
				Formatter->Flags & CFIXRUNP_FORMATTER_SHOW_STACKTRACE_SOURCE_INFORMATION,
				StackTraceBuffer,
				_countof( StackTraceBuffer ) );
		}


		switch ( Packet->EventType )
		{
		case CfixrunTestSuccess:
			return StringCchPrintf(
				Buffer,
				BufferSizeInChars,
				L"[Success]      %s.%s.%s\n",
				Packet->DebugInfo.Source.ModuleName,
				Packet->FixtureName,
				Packet->TestCaseName );
		
		case CfixrunTestFailure:
			//
			// Get message for last error.
			//
			CfixrunsResolveErrorMessage( 
				Formatter,
				Packet->LastError,
				LastErrorMessage,
				_countof( LastErrorMessage ) );

			if ( Packet->DebugInfo.Base.SourceLine )
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Failure]      %s.%s.%s \n"
					L"                 %s(%d): %s\n\n"
					L"                 Expression: %s\n"
					L"                 Last Error: %d (%s)\n\n"
					L"%s\n\n",
					Packet->DebugInfo.Source.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->DebugInfo.Source.SourceFile,
					Packet->DebugInfo.Source.SourceLine,
					Packet->DebugInfo.Source.FunctionName,
					Packet->Details,
					Packet->LastError,
					LastErrorMessage,
					StackTraceBuffer );
			}
			else
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Failure]      %s.%s.%s \n"
					L"                 Expression: %s\n"
					L"                 Last Error: %d (%s)\n\n"
					L"%s\n\n",
					Packet->DebugInfo.Source.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->Details,
					Packet->LastError,
					LastErrorMessage,
					StackTraceBuffer );
			}

		case CfixrunTestInconclusive:
			if ( Packet->DebugInfo.Base.SourceLine )
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Inconclusive] %s.%s.%s \n"
					L"                 %s(%d): %s\n"
					L"                 %s\n\n"
					L"%s\n\n",
					Packet->DebugInfo.Source.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->DebugInfo.Source.SourceFile,
					Packet->DebugInfo.Source.SourceLine,
					Packet->DebugInfo.Source.FunctionName,
					Packet->Details,
					StackTraceBuffer );
			}
			else
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Inconclusive] %s.%s.%s \n"
					L"                 %s\n\n"
					L"%s\n\n",
					Packet->DebugInfo.Source.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->Details,
					StackTraceBuffer );
			}

		case CfixrunLog:
			if ( Packet->DebugInfo.Base.SourceLine )
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Log]          %s.%s.%s \n"
					L"                 %s(%d): %s\n"
					L"                 %s\n\n",
					Packet->DebugInfo.Source.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->DebugInfo.Source.SourceFile,
					Packet->DebugInfo.Source.SourceLine,
					Packet->DebugInfo.Source.FunctionName,
					Packet->Details );
			}
			else
			{
					return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
						L"[Log]          %s.%s.%s \n"
						L"                 %s\n\n",
					Packet->DebugInfo.Source.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->Details );
			}

		default:
			ASSERT( !"Unknown result!" );
			return E_NOTIMPL;
		}
	}
	else
	{
		return Formatter->LogFormatter->Format(
			Formatter->LogFormatter,
			EventPkt,
			BufferSizeInChars,
			Buffer );
	}
}

/*----------------------------------------------------------------------
 *
 * 
 *
 */
HRESULT CfixrunpCreateFormatter(
	__in PCDIAG_MESSAGE_RESOLVER Resolver,
	__in DWORD Flags,
	__out PCDIAG_FORMATTER *Formatter
	)
{
	PFORMATTER NewFormatter;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! Formatter || 
		 ! Resolver )
	{
		return E_INVALIDARG;
	}

	NewFormatter = ( PFORMATTER ) malloc( sizeof( FORMATTER ) );
	if ( ! NewFormatter )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	NewFormatter->Base.Size			= sizeof( CDIAG_FORMATTER );
	NewFormatter->Base.Format		= CfixrunsFormat;
	NewFormatter->Base.Reference	= CfixrunsReferenceFormatter;
	NewFormatter->Base.Dereference	= CfixrunsDereferenceFormatter;
	NewFormatter->Flags				= Flags;
	NewFormatter->ReferenceCount	= 1;

	Hr = CdiagCreateFormatter(
		FORMATTER_LOG_TEMPLATE,
		Resolver,
		0,
		&NewFormatter->LogFormatter );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	NewFormatter->Resolver			= Resolver;
	Resolver->Reference( Resolver );

	*Formatter = &NewFormatter->Base;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( NewFormatter )
		{
			if ( NewFormatter->LogFormatter )
			{
				NewFormatter->LogFormatter->Dereference( NewFormatter->LogFormatter );
			}
		}

		free( NewFormatter );
	}

	return Hr;
}