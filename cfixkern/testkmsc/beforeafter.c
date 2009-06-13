/*----------------------------------------------------------------------
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

#include <cfix.h>

static ULONG BeforeCalls = 0;
static ULONG AfterCalls = 0;
static ULONG TestCalls = 0;

static void Before()
{
	BeforeCalls++;
}

static void After()
{
	AfterCalls++;
}

static void Test()
{
	TestCalls++;
}

static void Teardown()
{
	CFIX_ASSERT_EQUALS_ULONG( 2, TestCalls );
	CFIX_ASSERT_EQUALS_ULONG( 2, BeforeCalls );
	CFIX_ASSERT_EQUALS_ULONG( 2, AfterCalls );
}

CFIX_BEGIN_FIXTURE( BeforeAfter )
	CFIX_FIXTURE_BEFORE( Before )
	CFIX_FIXTURE_AFTER( After )
	CFIX_FIXTURE_TEARDOWN( Teardown )
	CFIX_FIXTURE_ENTRY( Test )
	CFIX_FIXTURE_ENTRY( Test )
CFIX_END_FIXTURE()