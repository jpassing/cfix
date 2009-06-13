/*----------------------------------------------------------------------
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

#include "stdafx.h"
#include "eventpkt.h"

VOID InitializeEventPacket(
	__in CDIAG_EVENT_TYPE Type,
	__in USHORT Flags,
	__in UCHAR Sev,
	__in UCHAR Mode,
	__in PCWSTR Machine,
	__in DWORD ProcessId,
	__in DWORD ThreadId,
	__in PFILETIME Timestamp,
	__in DWORD Code,
	__in PCWSTR Message,
	__in BOOL ProvideDebugInfo,
	__in PCWSTR Module,
	__in PCWSTR Function,
	__in PCWSTR SourceFile,
	__in ULONG SourceLine,
	__out PEVENT_PACKET_WITH_DATA Pkt
	)
{
	FILETIME ZeroTime = { 0, 0 };

	Pkt->EventPacket.Size = sizeof( CDIAG_EVENT_PACKET );
	Pkt->EventPacket.TotalSize = sizeof( EVENT_PACKET_WITH_DATA );

	Pkt->EventPacket.Type = Type;
	Pkt->EventPacket.Flags = Flags;
	Pkt->EventPacket.Severity = ( UCHAR ) Sev;
	Pkt->EventPacket.ProcessorMode = ( UCHAR ) Mode;
	if ( Machine )
	{
		StringCchCopy( 
			Pkt->Machine,
			_countof( Pkt->Machine ), 
			Machine );
		Pkt->EventPacket.MachineOffset = 
			FIELD_OFFSET( EVENT_PACKET_WITH_DATA, Machine );
	}
	else
	{
		Pkt->EventPacket.MachineOffset = 0;
	}
	Pkt->EventPacket.ProcessId = ProcessId;
	Pkt->EventPacket.ThreadId = ThreadId;
	Pkt->EventPacket.Timestamp = 
		( Timestamp ? *Timestamp : ZeroTime );
	Pkt->EventPacket.Code = Code;
	if ( Message )
	{
		StringCchCopy( 
			Pkt->Message, 
			_countof( Pkt->Message ), 
			Message );
		Pkt->EventPacket.MessageOffset = 
			FIELD_OFFSET( EVENT_PACKET_WITH_DATA, Message );
	}
	else
	{
		Pkt->EventPacket.MessageOffset = 0;
	}

	if ( ProvideDebugInfo )
	{
		if ( Module )
		{
			StringCchCopy( 
				Pkt->DebugInfo.Module, 
				_countof( Pkt->DebugInfo.Module ), 
				Module );
			Pkt->DebugInfo.DebugInfo.ModuleOffset = 
				FIELD_OFFSET( DEBUG_INFO_WITH_DATA, Module );
		}
		else
		{
			Pkt->DebugInfo.DebugInfo.ModuleOffset = 0;
		}

		if ( Function )
		{
			StringCchCopy( 
				Pkt->DebugInfo.Function, 
				_countof( Pkt->DebugInfo.Function ), 
				Function );
			Pkt->DebugInfo.DebugInfo.FunctionNameOffset = 
				FIELD_OFFSET( DEBUG_INFO_WITH_DATA, Function );
		}
		else
		{
			Pkt->DebugInfo.DebugInfo.FunctionNameOffset = 0;
		}

		if ( SourceFile )
		{
			StringCchCopy( 
				Pkt->DebugInfo.SourceFile, 
				_countof( Pkt->DebugInfo.SourceFile ), 
				SourceFile );
			Pkt->DebugInfo.DebugInfo.SourceFileOffset = 
				FIELD_OFFSET( DEBUG_INFO_WITH_DATA, SourceFile );
		}
		else
		{
			Pkt->DebugInfo.DebugInfo.SourceFileOffset = 0;
		}
		Pkt->DebugInfo.DebugInfo.SourceLine = SourceLine;
		
		Pkt->EventPacket.DebugInfoOffset = 
			FIELD_OFFSET( EVENT_PACKET_WITH_DATA, DebugInfo );
	}
	else
	{
		Pkt->EventPacket.DebugInfoOffset = 0;
	}

	Pkt->EventPacket.CustomData.Offset = 0;
	Pkt->EventPacket.CustomData.Length = 0;
	Pkt->EventPacket.MessageInsertionStrings.Count = 0;
}