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

static int Flag = 0;

FIXTURE( TestFixture );

SETUP( TestFixture )
{    
	WIN_TRACE( L"Setting %s", L"up" );
    WIN_ASSERT_TRUE( Flag == 0, "Setup not called yet " );
    WIN_ASSERT_FALSE( Flag == 1, "Setup not called yet " );
    WIN_ASSERT_ZERO( Flag );

	Flag = 1;
}

TEARDOWN( TestFixture )
{
	WIN_ASSERT_EQUAL( 2, Flag );
	Flag = 0;
}

BEGIN_TESTF( Test01, TestFixture )
{
    WIN_ASSERT_EQUAL( 1, Flag );
	Flag = 2;
}
END_TESTF

BEGIN_TESTF( Test02, TestFixture )
{
    WIN_ASSERT_EQUAL( 1, Flag );
	Flag = 2;
}
END_TESTF
