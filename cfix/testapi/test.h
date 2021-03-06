#pragma once

/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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
				

#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>
#include <shlwapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#include <cfix.h>
#include <cfixapi.h>

#define TEST CFIX_ASSERT
#define TEST_HR( expr ) CFIX_ASSERT_EQUALS_DWORD( S_OK, ( expr ) )
#define TEST_RETURN( Expected, Expr ) \
	CFIX_ASSERT_EQUALS_DWORD( ( DWORD ) Expected, ( DWORD ) ( Expr ) )
#define TEST_EQ CFIX_ASSERT_EQUALS_DWORD

//
// Handle to this module.
//
extern HMODULE ModuleHandle;