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
				

#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <shlwapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#include <jpdiag.h>


#ifdef DBG
#define TEST( expr ) ( ( !! ( expr ) ) || ( \
	OutputDebugString( \
		L"Test failed: " _CRT_WIDE( __FILE__ ) L" - " \
		_CRT_WIDE( __FUNCTION__ ) L": " _CRT_WIDE( #expr ) L"\n" ), DebugBreak(), 0 ) )
#else
#define TEST( expr ) ( ( !! ( expr ) ) || ( \
	OutputDebugString( \
		L"Test failed: " _CRT_WIDE( __FILE__ ) L" - " \
		_CRT_WIDE( __FUNCTION__ ) L": " _CRT_WIDE( #expr ) L"\n" ), DebugBreak(), 0 ) )
#endif

#define TEST_HR( expr ) TEST( S_OK == ( expr ) )
