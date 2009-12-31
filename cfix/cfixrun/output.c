/*----------------------------------------------------------------------
 * Purpose:
 *		Output Wrapper Routines.
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

typedef struct _DEBUG_MESSAGE_PACKET
{
	CDIAG_EVENT_PACKET Base;
	WCHAR Message[ 200 ];
} DEBUG_MESSAGE_PACKET, *PDEBUG_MESSAGE_PACKET;

HRESULT __cdecl CfixrunpOutputLogMessage(
	__in CDIAG_SESSION_HANDLE Session,
	__in CDIAG_SEVERITY_LEVEL Severity,
	__in PCWSTR Format,
	...
	)
{
	HRESULT Hr;
	DEBUG_MESSAGE_PACKET Packet;
	va_list lst;

	ZeroMemory( &Packet, sizeof( DEBUG_MESSAGE_PACKET ) );

	//
	// Format message.
	//
	va_start( lst, Format );
	Hr = StringCchVPrintf(
		Packet.Message, 
		_countof( Packet.Message ),
		Format,
		lst );
	va_end( lst );

	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	//
	// Initialize packet.
	//
	Packet.Base.Size			= sizeof( CDIAG_EVENT_PACKET );
	Packet.Base.TotalSize		= sizeof( DEBUG_MESSAGE_PACKET );

	Packet.Base.Type			= CdiagLogEvent;
	Packet.Base.Severity		= Severity;
	Packet.Base.ProcessorMode	= CdiagUserMode;
	Packet.Base.ProcessId		= GetCurrentProcessId();
	Packet.Base.ThreadId		= GetCurrentThreadId();
	Packet.Base.MessageOffset	= FIELD_OFFSET( DEBUG_MESSAGE_PACKET, Message );
	
	GetSystemTimeAsFileTime( &Packet.Base.Timestamp );

	return CdiagHandleEvent( Session, &Packet.Base );
}


HRESULT CfixrunpOutputTestEvent(
	__in CDIAG_SESSION_HANDLE Session,
	__in CFIXRUNS_EVENT_TYPE EventType,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in_opt PCWSTR Details,
	__in_opt PCWSTR ModuleName,
	__in_opt PCWSTR FunctionName,
	__in_opt PCWSTR SourceFile,
	__in UINT SourceLine,
	__in DWORD LastError,
	__in PCFIX_THREAD_ID ThreadId,
	__in_opt PCFIX_STACKTRACE StackTrace,
	__in_opt CFIX_GET_INFORMATION_STACKFRAME_ROUTINE GetInfoStackFrameRoutine
	)
{
	HRESULT Hr;
	PCFIXRUN_TEST_EVENT_PACKET Packet;
	SIZE_T PacketSize;

	if ( StackTrace != NULL )
	{
		PacketSize = RTL_SIZEOF_THROUGH_FIELD(
			CFIXRUN_TEST_EVENT_PACKET,
			StackTrace.Frames[ StackTrace->FrameCount - 1 ] );
	}
	else
	{
		PacketSize = sizeof( CFIXRUN_TEST_EVENT_PACKET );
	}

	//
	// Packet size depends on number of stackframes.
	//
	Packet = ( PCFIXRUN_TEST_EVENT_PACKET ) malloc( PacketSize );
	if ( ! Packet )
	{
		return E_OUTOFMEMORY;
	}
	
	ZeroMemory( Packet, PacketSize );

	//
	// Initialize base part.
	//
	Packet->Base.Size				= sizeof( CDIAG_EVENT_PACKET );
	Packet->Base.TotalSize			= sizeof( CFIXRUN_TEST_EVENT_PACKET );

	Packet->Base.Type				= CdiagCustomEvent;
	Packet->Base.SubType				= CFIXRUN_TEST_EVENT_PACKET_SUBTYPE;
	Packet->Base.Severity			= CdiagInfoSeverity;
	Packet->Base.ProcessorMode		= CdiagUserMode;
	Packet->Base.ProcessId			= GetCurrentProcessId();
	Packet->Base.ThreadId			= ThreadId->ThreadId;
	Packet->Base.DebugInfoOffset		= 
		FIELD_OFFSET( CFIXRUN_TEST_EVENT_PACKET, DebugInfo );
	
	GetSystemTimeAsFileTime( &Packet->Base.Timestamp );

	//
	// Initialize debug part.
	//
	Packet->DebugInfo.Base.Size		= sizeof( CDIAG_DEBUG_INFO );
	Packet->DebugInfo.Base.TotalSize	= sizeof( CFIXRUN_TEST_EVENT_DEBUG_INFO );
	
	if ( ModuleName )
	{
		( VOID ) StringCchCopy( 
			Packet->DebugInfo.Source.ModuleName, 
			_countof( Packet->DebugInfo.Source.ModuleName ), 
			ModuleName );

		Packet->DebugInfo.Base.ModuleOffset = 
			FIELD_OFFSET( CFIXRUN_TEST_EVENT_DEBUG_INFO, Source.ModuleName );
	}

	if ( FunctionName )
	{
		( VOID ) StringCchCopy( 
			Packet->DebugInfo.Source.FunctionName, 
			_countof( Packet->DebugInfo.Source.FunctionName ), 
			FunctionName );

		Packet->DebugInfo.Base.FunctionNameOffset = 
			FIELD_OFFSET( CFIXRUN_TEST_EVENT_DEBUG_INFO, Source.FunctionName );
	}

	if ( SourceFile )
	{
		( VOID ) StringCchCopy( 
			Packet->DebugInfo.Source.SourceFile, 
			_countof( Packet->DebugInfo.Source.SourceFile ), 
			SourceFile );

		Packet->DebugInfo.Base.SourceFileOffset = 
			FIELD_OFFSET( CFIXRUN_TEST_EVENT_DEBUG_INFO, Source.SourceFile );
	}

	Packet->DebugInfo.Base.SourceLine = SourceLine;
	Packet->DebugInfo.Source.SourceLine = SourceLine;

	//
	// Initialize custom part.
	//
	Packet->EventType = EventType;
	Packet->LastError = LastError;

	( VOID ) StringCchCopy( 
		Packet->FixtureName,
		_countof( Packet->FixtureName ),
		FixtureName ? FixtureName : L"unknown" );
	( VOID ) StringCchCopy( 
		Packet->TestCaseName,
		_countof( Packet->TestCaseName ),
		TestCaseName ? TestCaseName : L"unknown"  );
	( VOID ) StringCchCopy( 
		Packet->Details,
		_countof( Packet->Details ),
		Details ? Details : L""  );
	
	//
	// Stack trace.
	//
	if ( StackTrace && GetInfoStackFrameRoutine )
	{
		UINT FrameIndex;

		for ( FrameIndex = 0; FrameIndex < StackTrace->FrameCount; FrameIndex++ )
		{
			PCFIXRUN_TEST_EVENT_STACKFRAME Frame;

			Frame = &Packet->StackTrace.Frames[ FrameIndex ];

			if ( SUCCEEDED( ( GetInfoStackFrameRoutine ) (
				StackTrace->Frames[ FrameIndex ],
				_countof( Frame->Source.ModuleName ),
				Frame->Source.ModuleName,
				_countof( Frame->Source.FunctionName ),
				Frame->Source.FunctionName,
				( PDWORD ) &Frame->Displacement,
				_countof( Frame->Source.SourceFile ),
				Frame->Source.SourceFile,
				( PDWORD ) &Frame->Source.SourceLine ) ) )
			{
				Packet->StackTrace.FrameCount++;
			}
			else 
			{
				break;
			}
		}

		ASSERT( Packet->StackTrace.FrameCount == StackTrace->FrameCount );
	}

	Hr = CdiagHandleEvent( Session, &Packet->Base );

	free( Packet );

	return Hr;
}