/*----------------------------------------------------------------------
 * Purpose:
 *		Event DLL loading.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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

#include <cfixevnt.h>
#include <crtdbg.h>

#define ASSERT _ASSERTE

#define CFIXP_CREATE_EVENT_SINK_EXPORT_NAME "CreateEventSink"

HRESULT CfixutilLoadEventSinkFromDll(
	__in PCWSTR DllPath,
	__in ULONG Flags,
	__in_opt PCWSTR Options,
	__out PCFIX_EVENT_SINK *Sink
	)
{
	HMODULE Module;
	CFIX_CREATE_EVENT_SINK_ROUTINE Routine;

	if ( ! DllPath || ! Sink )
	{
		return E_INVALIDARG;
	}

	Module = LoadLibrary( DllPath );
	if ( Module == NULL )
	{
		return CFIX_E_LOADING_EVENTDLL_FAILED;
	}

	Routine = ( CFIX_CREATE_EVENT_SINK_ROUTINE ) GetProcAddress( 
		Module, 
		CFIXP_CREATE_EVENT_SINK_EXPORT_NAME );
	if ( Routine == NULL )
	{
		return CFIX_E_MISSING_EVENT_SINK_EXPORT;
	}

	return ( Routine ) (
		CFIX_EVENT_SINK_VERSION,
		Flags,
		Options,
		0,
		Sink );
}
