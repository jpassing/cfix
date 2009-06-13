#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Shared header file. Do not include directly!
 *
 *            cfixaux.h
 *              ^ ^
 *             /   \
 *            /     \
 *		cfixapi.h  cfixpe.h
 *			^	  ^	 ^
 *			|	 /	  \
 *			|	/	   \
 *		  [Impl]	 cfix.h
 *
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


#define ___CFIX_WIDE( str ) L ##  str 
#define __CFIX_WIDE( str ) ___CFIX_WIDE( str )

typedef enum 
{
	//
	// Continue execution.
	//
	CfixContinue		= 0,

	//
	// Break if in debugger.
	//
	CfixBreak			= 1,

	//
	// Always break.
	//
	CfixBreakAlways		= 2,

	//
	// Abort testrun.
	//
	CfixAbort			= 3
} CFIX_REPORT_DISPOSITION;

typedef enum
{
	CfixEventFailedAssertion		= 0,
	CfixEventUncaughtException		= 1,
	CfixEventInconclusivess			= 2,
	CfixEventLog					= 3
} CFIX_EVENT_TYPE;

#define CFIX_EXIT_THREAD_ABORTED 0xffffffff