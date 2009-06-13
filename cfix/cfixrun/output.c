/*----------------------------------------------------------------------
 * Purpose:
 *		Output Wrapper Routines.
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

typedef struct _DEBUG_MESSAGE_PACKET
{
	JPDIAG_EVENT_PACKET Base;
	WCHAR Message[ 200 ];
} DEBUG_MESSAGE_PACKET, *PDEBUG_MESSAGE_PACKET;

HRESULT __cdecl CfixrunpOutputLogMessage(
	__in JPDIAG_SESSION_HANDLE Session,
	__in JPDIAG_SEVERITY_LEVEL Severity,
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
	Packet.Base.Size			= sizeof( JPDIAG_EVENT_PACKET );
	Packet.Base.TotalSize		= sizeof( DEBUG_MESSAGE_PACKET );

	Packet.Base.Type			= JpdiagLogEvent;
	Packet.Base.Severity		= Severity;
	Packet.Base.ProcessorMode	= JpdiagUserMode;
	Packet.Base.ProcessId		= GetCurrentProcessId();
	Packet.Base.ThreadId		= GetCurrentThreadId();
	Packet.Base.MessageOffset	= FIELD_OFFSET( DEBUG_MESSAGE_PACKET, Message );
	
	GetSystemTimeAsFileTime( &Packet.Base.Timestamp );

	return JpdiagHandleEvent( Session, &Packet.Base );
}


HRESULT CfixrunpOutputTestEvent(
	__in JPDIAG_SESSION_HANDLE Session,
	__in CFIXRUNS_EVENT_TYPE EventType,
	__in PCWSTR FixtureName,
	__in PCWSTR TestCaseName,
	__in PCWSTR Details,
	__in PCWSTR ModuleName,
	__in PCWSTR FunctionName,
	__in PCWSTR SourceFile,
	__in UINT SourceLine,
	__in DWORD LastError
	)
{
	CFIXRUN_TEST_EVENT_PACKET Packet;
	
	ZeroMemory( &Packet, sizeof( CFIXRUN_TEST_EVENT_PACKET ) );

	//
	// Initialize base part.
	//
	Packet.Base.Size				= sizeof( JPDIAG_EVENT_PACKET );
	Packet.Base.TotalSize			= sizeof( CFIXRUN_TEST_EVENT_PACKET );

	Packet.Base.Type				= JpdiagCustomEvent;
	Packet.Base.SubType				= CFIXRUN_TEST_EVENT_PACKET_SUBTYPE;
	Packet.Base.Severity			= JpdiagInfoSeverity;
	Packet.Base.ProcessorMode		= JpdiagUserMode;
	Packet.Base.ProcessId			= GetCurrentProcessId();
	Packet.Base.ThreadId			= GetCurrentThreadId();
	Packet.Base.DebugInfoOffset		= 
		FIELD_OFFSET( CFIXRUN_TEST_EVENT_PACKET, DebugInfo );
	
	GetSystemTimeAsFileTime( &Packet.Base.Timestamp );


	//
	// Initialize debug part.
	//
	Packet.DebugInfo.Base.Size		= sizeof( JPDIAG_DEBUG_INFO );
	Packet.DebugInfo.Base.TotalSize	= sizeof( CFIXRUN_TEST_EVENT_DEBUG_INFO );
	
	if ( ModuleName )
	{
		( VOID ) StringCchCopy( 
			Packet.DebugInfo.ModuleName, 
			_countof( Packet.DebugInfo.ModuleName ), 
			ModuleName );

		Packet.DebugInfo.Base.ModuleOffset = 
			FIELD_OFFSET( CFIXRUN_TEST_EVENT_DEBUG_INFO, ModuleName );
	}

	if ( FunctionName )
	{
		( VOID ) StringCchCopy( 
			Packet.DebugInfo.FunctionName, 
			_countof( Packet.DebugInfo.FunctionName ), 
			FunctionName );

		Packet.DebugInfo.Base.FunctionNameOffset = 
			FIELD_OFFSET( CFIXRUN_TEST_EVENT_DEBUG_INFO, FunctionName );
	}

	if ( SourceFile )
	{
		( VOID ) StringCchCopy( 
			Packet.DebugInfo.SourceFile, 
			_countof( Packet.DebugInfo.SourceFile ), 
			SourceFile );

		Packet.DebugInfo.Base.SourceFileOffset = 
			FIELD_OFFSET( CFIXRUN_TEST_EVENT_DEBUG_INFO, SourceFile );
	}

	Packet.DebugInfo.Base.SourceLine = SourceLine;

	//
	// Initialize custom part.
	//
	Packet.EventType = EventType;
	Packet.LastError = LastError;

	( VOID ) StringCchCopy( 
		Packet.FixtureName,
		_countof( Packet.FixtureName ),
		FixtureName ? FixtureName : L"unknown" );
	( VOID ) StringCchCopy( 
		Packet.TestCaseName,
		_countof( Packet.TestCaseName ),
		TestCaseName ? TestCaseName : L"unknown"  );
	( VOID ) StringCchCopy( 
		Packet.Details,
		_countof( Packet.Details ),
		Details ? Details : L""  );
	
	return JpdiagHandleEvent( Session, &Packet.Base );
}