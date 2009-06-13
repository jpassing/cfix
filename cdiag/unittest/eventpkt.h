#pragma once

/*----------------------------------------------------------------------
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

typedef struct _DEBUG_INFO_WITH_DATA
{
	CDIAG_DEBUG_INFO DebugInfo;
	WCHAR Module[ 32 ];
	WCHAR Function[ 32 ];
	WCHAR SourceFile[ 32 ];
} DEBUG_INFO_WITH_DATA, *PDEBUG_INFO_WITH_DATA;

typedef struct _EVENT_PACKET_WITH_DATA
{
	CDIAG_EVENT_PACKET EventPacket;
	DEBUG_INFO_WITH_DATA DebugInfo;
	WCHAR Machine[ 32 ];
	WCHAR Message[ 64 ];	
} EVENT_PACKET_WITH_DATA, *PEVENT_PACKET_WITH_DATA;


/*++
	Routine Description:
		Initialize a self-relative event packet.
--*/
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
	);