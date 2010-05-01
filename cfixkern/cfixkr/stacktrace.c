/*----------------------------------------------------------------------
 *	Purpose:
 *		Stack Trace Capturing.
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
#include <ntddk.h>
#include <stdlib.h>
#include "cfixkrp.h"

NTSTATUS CfixkrpCaptureStackTrace(
	__in_opt CONST PCONTEXT Context,
	__in PCFIX_STACKTRACE StackTrace,
	__in ULONG MaxFrames 
	)
{
#ifdef _WIN32
	ULONG Index;
	PVOID FramesBuffer[ 64 ];
#endif

	if ( Context != NULL )
	{
		//
		// Unfortunately, RtlWalkFrameChain cannot be passed
		// a CONTEXT.
		//
		return STATUS_NOT_IMPLEMENTED;
	}

	StackTrace->GetInformationStackFrame = NULL;

	//
	// N.B. CFIX_STACKTRACE uses ULONGLONGs, RtlWalkFrameChain
	// uses PVOIDs, so conversion is necessary on i386.
	//
#ifdef _WIN32
	StackTrace->FrameCount = RtlWalkFrameChain(
		FramesBuffer,
		min( MaxFrames, _countof( FramesBuffer ) ),
		0 );
	
	for ( Index = 0; Index < StackTrace->FrameCount; Index++ )
	{
		StackTrace->Frames[ Index ] = ( ULONG_PTR ) FramesBuffer[ Index ];
	}

#else
	StackTrace->FrameCount = RtlWalkFrameChain(
		StackTrace->Frames,
		MaxFrames,
		0 );
#endif

	return STATUS_SUCCESS;
}