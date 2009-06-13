/*----------------------------------------------------------------------
 * Purpose:
 *		Minimal test for COM macros.
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
#include "test.h"

static void TestComMacros()
{
	CFIX_ASSERT_OK( S_OK );
	CFIX_ASSERT_SUCCEEDED( S_OK );
	CFIX_ASSERT_SUCCEEDED( S_FALSE );
	CFIX_ASSERT_FAILED( E_FAIL );
}

CFIX_BEGIN_FIXTURE( Misc )
	CFIX_FIXTURE_ENTRY(TestComMacros)
CFIX_END_FIXTURE()