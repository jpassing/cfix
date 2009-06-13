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

class ImprovedAdderTest : public cfixcc::TestFixture
{
private:
	Adder adder;

public:
	void Before()
	{
		//
		// A freshly created Adder should have the sum 0. Mind you,
		// a new instance of ImprovedAdderTest is created for each
		// test
		//
		CFIXCC_ASSERT_EQUALS( 0, this->adder.GetSum() );
	}

	void TestBounds() 
	{
		this->adder.Add( INT_MAX );
		CFIXCC_ASSERT_LESS( 0, this->adder.GetSum() );
		CFIXCC_ASSERT_EQUALS( INT_MAX, this->adder.GetSum() );

		this->adder.Reset();

		this->adder.Add( INT_MIN );
		CFIXCC_ASSERT_GREATER( 0, this->adder.GetSum() );
		CFIXCC_ASSERT_EQUALS( INT_MIN, this->adder.GetSum() );
	}

	void TestOverflow() 
	{
		this->adder.Add( INT_MAX );
		this->adder.Add( 1 );
	}

	void TestUnderflow() 
	{
		this->adder.Add( INT_MIN );
		this->adder.Add( -1 );
	}
};

CFIXCC_BEGIN_CLASS( ImprovedAdderTest )
	CFIXCC_METHOD( TestBounds )
	CFIXCC_METHOD_EXPECT_EXCEPTION( TestOverflow, std::overflow_error )
	CFIXCC_METHOD_EXPECT_EXCEPTION( TestUnderflow, std::underflow_error )
CFIXCC_END_CLASS()