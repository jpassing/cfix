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
#include <cfixcc.h>

class SimpleFixture : public cfixcc::TestFixture
{
private:
	int counter;

public:
	SimpleFixture() : counter( 0 )
	{}

	~SimpleFixture()
	{
		CFIXCC_ASSERT_EQUALS( counter, 3 );
	}

	virtual void Before()
	{
		CFIXCC_ASSERT_EQUALS( counter, 0 );
		counter++;
	}

	void Method01() 
	{
		CFIXCC_ASSERT_EQUALS( counter, 1 );
		counter++;
	}

	void Method02() 
	{
		CFIXCC_ASSERT_EQUALS( counter, 1 );
		counter++;
	}

	virtual void After()
	{
		CFIXCC_ASSERT_EQUALS( counter, 2 );
		counter++;
	}
};

class TestException {};

class FixtureWithSetUpAndTearDown : public cfixcc::TestFixture
{
private:
	int counter;
	static bool setupCalled;

public:
	FixtureWithSetUpAndTearDown() : counter( 0 )
	{}

	~FixtureWithSetUpAndTearDown()
	{
		CFIXCC_ASSERT_EQUALS( counter, 3 );
	}

	static void SetUp()
	{
		CFIX_ASSERT( ! setupCalled );
		setupCalled = true;
	}

	static void TearDown()
	{
		CFIX_ASSERT( setupCalled );
	}

	virtual void Before()
	{
		CFIX_ASSERT( setupCalled );
		CFIXCC_ASSERT_EQUALS( counter, 0 );
		counter++;
	}

	void Method01() 
	{
		CFIX_ASSERT( setupCalled );
		CFIXCC_ASSERT_EQUALS( counter, 1 );
		counter++;
	}

	void Method02() 
	{
		CFIX_ASSERT( setupCalled );
		CFIXCC_ASSERT_EQUALS( counter, 1 );
		counter++;
	}

	void Throw()
	{
		CFIXCC_ASSERT( setupCalled );
		CFIXCC_ASSERT_EQUALS( counter, 1 );
		counter++;
		throw TestException();
	}

	virtual void After()
	{
		CFIXCC_ASSERT_EQUALS( counter, 2 );
		counter++;
	}
};

bool FixtureWithSetUpAndTearDown::setupCalled = false;

CFIXCC_BEGIN_CLASS( SimpleFixture )
	CFIXCC_METHOD( Method01 )
	CFIXCC_METHOD( Method02 )
CFIXCC_END_CLASS()

CFIXCC_BEGIN_CLASS( FixtureWithSetUpAndTearDown )
	CFIXCC_METHOD( Method01 )
	CFIXCC_METHOD( Method02 )
	CFIXCC_METHOD_EXPECT_EXCEPTION( Throw, TestException )
CFIXCC_END_CLASS()