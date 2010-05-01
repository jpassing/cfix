/*----------------------------------------------------------------------
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

#include <cfix.h>

static void TestApiIsExternC()
{
	CFIX_ASSERT( TRUE );
	CFIX_ASSERT_STATUS( STATUS_SUCCESS, STATUS_SUCCESS );
	CFIX_ASSERT_EQUALS_ULONG( 0UL, 0UL );
	CFIX_LOG( NULL );

#pragma warning( push )
#pragma warning( disable: 6309; disable: 6387 )
	CFIX_ASSERT( STATUS_SUCCESS != CfixCreateSystemThread(
		NULL,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		0 ) );
#pragma warning( pop )
}

CFIX_BEGIN_FIXTURE( Cpp )
	CFIX_FIXTURE_ENTRY( TestApiIsExternC )
CFIX_END_FIXTURE()