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
 
//
// stdafx.h includes cfixcc.h for us.
//
#include "stdafx.h"
#include "Adder.h"

class SimpleAdderTest : public cfixcc::TestFixture
{
public:
	void TestBasicAddition() 
	{
		Adder a;
		a.Add( 0 );
		CFIXCC_ASSERT_EQUALS( 0, a.GetSum() );

		a.Add( 1 );
		CFIXCC_ASSERT_EQUALS( 1, a.GetSum() );
	}

	void TestBasicSubtraction() 
	{
		Adder a;
		a.Add( -3 );
		CFIXCC_ASSERT_EQUALS( -3, a.GetSum() );

		a.Add( 3 );
		CFIXCC_ASSERT_EQUALS( 0, a.GetSum() );
	}
};

CFIXCC_BEGIN_CLASS( SimpleAdderTest )
	CFIXCC_METHOD( TestBasicAddition )
	CFIXCC_METHOD( TestBasicSubtraction )
CFIXCC_END_CLASS()