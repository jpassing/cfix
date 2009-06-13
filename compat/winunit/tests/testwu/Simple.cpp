/*----------------------------------------------------------------------
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * N.B. This file does not include any source code from WinUnit.
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

#include <WinUnit.h>

#pragma warning( disable: 4702 )	// Unreachable code

class SimpleException {};
static void RaiseSimpleException()
{
	throw SimpleException();
}

BEGIN_TEST( TestEqual )
{
    WIN_ASSERT_EQUAL( 1, 1 );
    WIN_ASSERT_EQUAL( 1, 1 );
    WIN_ASSERT_EQUAL( 1.0f, 1.0f, "floats" );
    WIN_ASSERT_EQUAL( true, true, L"bools" );
    WIN_ASSERT_EQUAL( -1, -1, "[%d] is 1", 1 );
    WIN_ASSERT_EQUAL( 1, 1, L"%d == 1", 1 );
    WIN_ASSERT_STRING_EQUAL( "foo bar", "foo bar" );
    WIN_ASSERT_STRING_EQUAL( L"foo bar", L"foo bar", "" );
}
END_TEST

BEGIN_TEST( TestNotEqual )
{
    WIN_ASSERT_NOT_EQUAL( 1, 2 );
    WIN_ASSERT_NOT_EQUAL( 2.0f, 1.0f, "floats" );
    WIN_ASSERT_NOT_EQUAL( false, true, L"bools" );
    WIN_ASSERT_NOT_EQUAL( 1, -1, "[%d] is 1", 1 );
    WIN_ASSERT_NOT_EQUAL( 2, 1, L"%d == 1", 1 );
}
END_TEST

BEGIN_TEST( TestZero )
{
    WIN_ASSERT_ZERO( 0 );
    WIN_ASSERT_ZERO( 0, "zero" );
	WIN_ASSERT_ZERO( 0, L"zero" );
    WIN_ASSERT_NOT_ZERO( 1 );
    WIN_ASSERT_NOT_ZERO( 1, "zero" );
	WIN_ASSERT_NOT_ZERO( 1, L"zero" );
}
END_TEST

BEGIN_TEST( TestNull )
{
	PVOID NullPtr = NULL;
    WIN_ASSERT_NULL( NullPtr );
    WIN_ASSERT_NOT_NULL( &RaiseSimpleException );
}
END_TEST

BEGIN_TEST( TestThrow )
{
    WIN_ASSERT_THROWS(
		RaiseSimpleException(), 
		SimpleException,
        _T( "Expecting SimpleException" ) );
}
END_TEST
