/*----------------------------------------------------------------------
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

#include <cfix.h>

static VOID Setup()
{
}

static VOID Teardown()
{
}

static VOID Test01()
{
}

static VOID Test02()
{
}

CFIX_BEGIN_FIXTURE(SampleFixture1)
	CFIX_FIXTURE_TEARDOWN(Teardown)
	CFIX_FIXTURE_ENTRY(Test01)
	CFIX_FIXTURE_ENTRY(Test02)
	CFIX_FIXTURE_SETUP(Setup)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(SampleFixture2)
	CFIX_FIXTURE_TEARDOWN(Teardown)
	CFIX_FIXTURE_ENTRY(Test01)
	CFIX_FIXTURE_ENTRY(Test02)
	CFIX_FIXTURE_SETUP(Setup)
CFIX_END_FIXTURE()