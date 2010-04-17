#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Cfix utility header file. 
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

#include <windows.h>
#include <cfixevnt.h>
#include <cfixmsg.h>


/*----------------------------------------------------------------------
 *
 * Utility API.
 *
 */

typedef enum CFIXUTIL_VISIT_TYPE
{
	CfixutilEnterDirectory,
	CfixutilLeaveDirectory,
	CfixutilFile
} CFIXUTIL_VISIT_TYPE;

/*++
	Routine Description:
		Callback for CfixutilSearch

	Parameters:
		Path			Path of module found.
		Type			Type of module.
		Context			Caller supplied context
		SearchPerformed	Specifies whether an actual search was
						performed (FALSE when an exact file path
						was passed to CfixutilSearch)

	Return Value:
		S_OK on success
		failure HRESULT on failure. Search will be aborted and
			return value is propagated to caller.
--*/
typedef HRESULT ( CFIXCALLTYPE * CFIXUTIL_VISIT_ROUTINE ) (
	__in PCWSTR Path,
	__in CFIXUTIL_VISIT_TYPE Type,
	__in_opt PVOID Context,
	__in BOOL SearchPerformed
	);

/*++
	Routine Description:
		Search for files in a directory.

	Parameters:
		Path			Path to a file or directory.
		Recursive		Search recursively?
		Routine			Callback.
		Context			Callback context argument.
--*/
EXTERN_C HRESULT CFIXCALLTYPE CfixutilSearch(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in CFIXUTIL_VISIT_ROUTINE Routine,
	__in_opt PVOID Context
	);

/*++
	Routine Description:
		Load event DLL and create event sink.

	Parameters:
		DllPath		- Name/path of DLL.
		Flags		- CFIX_EVENT_SINK_FLAG_*.
		Options		- Options to pass to event DLL.
		Sink		- Result.
--*/
EXTERN_C HRESULT CfixutilLoadEventSinkFromDll(
	__in PCWSTR DllPath,
	__in ULONG Flags,
	__in_opt PCWSTR Options,
	__out PCFIX_EVENT_SINK *Sink
	);