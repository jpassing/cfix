/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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

#include "test.h"

#define TEST CFIX_ASSERT

template< int Id >
static VOID Test()
{
	NotifyRunning( Id );
	if ( ShouldFail( Id ) )
	{
		CFIX_ASSERT( !"Fail" );
	}
	else
	{
		CFIX_LOG( NULL );
	}
}

template< int Id >
static VOID BeforeAfter()
{
	NotifyRunning( Id );
	if ( ShouldFail( Id ) )
	{
		CFIX_ASSERT( !"Fail" );
	}
}

//
// N.B. Suites in alphabetic order!
//

CFIX_BEGIN_FIXTURE( A )
	CFIX_FIXTURE_SETUP( Test< 1 > )
	CFIX_FIXTURE_TEARDOWN( Test< 2 > )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( B )
	CFIX_FIXTURE_SETUP( Test< 4 > )
	CFIX_FIXTURE_ENTRY( Test< 8 > )
	CFIX_FIXTURE_ENTRY( Test< 16 > )
	CFIX_FIXTURE_TEARDOWN( Test< 32 > )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( C )
	CFIX_FIXTURE_SETUP( Test< 64 > )
	CFIX_FIXTURE_ENTRY( Test< 128 > )
	CFIX_FIXTURE_ENTRY( Test< 256 > )
	CFIX_FIXTURE_BEFORE( BeforeAfter< 512 > )
	CFIX_FIXTURE_AFTER( BeforeAfter< 1024 > )
	CFIX_FIXTURE_TEARDOWN( Test< 2048 > )
CFIX_END_FIXTURE()