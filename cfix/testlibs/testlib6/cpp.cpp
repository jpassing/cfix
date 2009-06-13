/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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

#include <cfix.h>

#define TEST CFIX_ASSERT

class Test
{
public:
	static BOOL DestructorCalled;

	Test()
	{
	}

	~Test()
	{
		DestructorCalled = TRUE;
	}

	static VOID CreateObjectAndFail()
	{
		Test t;

		TEST( !"fail" );

		//
		// Destructor should be called.
		//
	}
	
	static VOID Teardown()
	{
		TEST( DestructorCalled );
	}
};

BOOL Test::DestructorCalled = FALSE;

CFIX_BEGIN_FIXTURE(TestCpp)
	CFIX_FIXTURE_ENTRY(Test::CreateObjectAndFail)
	CFIX_FIXTURE_TEARDOWN(Test::Teardown)
CFIX_END_FIXTURE()

