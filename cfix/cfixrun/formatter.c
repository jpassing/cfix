/*----------------------------------------------------------------------
 * Purpose:
 *		Custom formatter.
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
#include "internal.h"
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define FORMATTER_LOG_TEMPLATE L"[Log] %Severity (%ProcessId:%ThreadId): %Message"

typedef struct _FORMATTER
{
	JPDIAG_FORMATTER Base;

	//
	// Let other formatters do the real work.
	//
	PJPDIAG_FORMATTER LogFormatter;

	volatile LONG ReferenceCount;
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

	free( Formatter );
}

static VOID CfixrunsReferenceFormatter(
	__in PJPDIAG_FORMATTER This
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	InterlockedIncrement( &Formatter->ReferenceCount );
}

static VOID CfixrunsDereferenceFormatter(
	__in PJPDIAG_FORMATTER This
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	if ( 0 == InterlockedDecrement( &Formatter->ReferenceCount ) )
	{
		CfixrunsDeleteFormatter( Formatter );
	}
}

static HRESULT CfixrunsFormat(
	__in PJPDIAG_FORMATTER This,
	__in CONST PJPDIAG_EVENT_PACKET EventPkt,
	__in SIZE_T BufferSizeInChars,
	__out PWSTR Buffer
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;

	if ( EventPkt->Type == JpdiagCustomEvent &&
		 EventPkt->SubType == CFIXRUN_TEST_EVENT_PACKET_SUBTYPE )
	{
		PCFIXRUN_TEST_EVENT_PACKET Packet = CONTAINING_RECORD(
			EventPkt,
			CFIXRUN_TEST_EVENT_PACKET,
			Base );

		ASSERT( EventPkt->TotalSize == sizeof( CFIXRUN_TEST_EVENT_PACKET ) );

		switch ( Packet->EventType )
		{
		case CfixrunTestSuccess:
			return StringCchPrintf(
				Buffer,
				BufferSizeInChars,
				L"[Success]      %s.%s.%s\n",
				Packet->DebugInfo.ModuleName,
				Packet->FixtureName,
				Packet->TestCaseName );
		
		case CfixrunTestFailure:
			if ( Packet->DebugInfo.Base.SourceLine )
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Failure]      %s.%s.%s \n"
					L"                 %s(%d): %s\n\n"
					L"                 Expression: %s\n"
					L"                 Last Error: %d\n\n",
					Packet->DebugInfo.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->DebugInfo.SourceFile,
					Packet->DebugInfo.Base.SourceLine,
					Packet->DebugInfo.FunctionName,
					Packet->Details,
					Packet->LastError );
			}
			else
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Failure]      %s.%s.%s \n"
					L"                 Expression: %s\n"
					L"                 Last Error: %d\n\n",
					Packet->DebugInfo.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->Details,
					Packet->LastError );
			}

		case CfixrunTestInconclusive:
			if ( Packet->DebugInfo.Base.SourceLine )
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Inconclusive] %s.%s.%s \n"
					L"                 %s(%d): %s\n"
					L"                 %s\n\n",
					Packet->DebugInfo.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->DebugInfo.SourceFile,
					Packet->DebugInfo.Base.SourceLine,
					Packet->DebugInfo.FunctionName,
					Packet->Details );
			}
			else
			{
				return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
					L"[Inconclusive] %s.%s.%s \n"
					L"                 %s\n\n",
					Packet->DebugInfo.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->Details );
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
					Packet->DebugInfo.ModuleName,
					Packet->FixtureName,
					Packet->TestCaseName,
					Packet->DebugInfo.SourceFile,
					Packet->DebugInfo.Base.SourceLine,
					Packet->DebugInfo.FunctionName,
					Packet->Details );
			}
			else
			{
					return StringCchPrintf(
					Buffer,
					BufferSizeInChars,
						L"[Log]          %s.%s.%s \n"
						L"                 %s\n\n",
					Packet->DebugInfo.ModuleName,
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
	__in PJPDIAG_MESSAGE_RESOLVER Resolver,
	__out PJPDIAG_FORMATTER *Formatter
	)
{
	PFORMATTER NewFormatter;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! Formatter || ! Resolver )
	{
		return E_INVALIDARG;
	}

	NewFormatter = ( PFORMATTER ) malloc( sizeof( FORMATTER ) );
	if ( ! NewFormatter )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	NewFormatter->Base.Size			= sizeof( JPDIAG_FORMATTER );
	NewFormatter->Base.Format		= CfixrunsFormat;
	NewFormatter->Base.Reference	= CfixrunsReferenceFormatter;
	NewFormatter->Base.Dereference	= CfixrunsDereferenceFormatter;

	Hr = JpdiagCreateFormatter(
		FORMATTER_LOG_TEMPLATE,
		Resolver,
		0,
		&NewFormatter->LogFormatter );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

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