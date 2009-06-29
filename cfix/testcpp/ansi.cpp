/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
 *
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

#undef UNICODE
#undef _UNICODE

#include <cfixcc.h>

#define TEST_EQ( Expected, Actual )										\
	CFIXCC_ASSERT_EQUALS_MESSAGE( Expected, Actual, "%d%s", 1, "123" );	\
	CFIXCC_ASSERT_EQUALS( Expected, Actual );								\
	CFIXCC_ASSERT_LESS_OR_EQUAL( Expected, Actual );						\
	CFIXCC_ASSERT_LESS_OR_EQUAL_MESSAGE( Expected, Actual, "%d%s", 1, "123" );	\
	CFIXCC_ASSERT_GREATER_OR_EQUAL( Expected, Actual );					\
	CFIXCC_ASSERT_GREATER_OR_EQUAL_MESSAGE( Expected, Actual, "%d%s", 1, "123" );	\

#define TEST_LT( Expected, Actual )										\
	CFIXCC_ASSERT_NOT_EQUALS_MESSAGE( Expected, Actual, "%d%s", 1, "123" );	\
	CFIXCC_ASSERT_NOT_EQUALS( Expected, Actual );							\
	CFIXCC_ASSERT_LESS( Expected, Actual );								\
	CFIXCC_ASSERT_LESS_MESSAGE( Expected, Actual, "%d%s", 1, "123" );	\
	CFIXCC_ASSERT_GREATER( Actual, Expected );							\
	CFIXCC_ASSERT_GREATER_MESSAGE( Actual, Expected, "%d%s", 1, "123" );	\
	CFIXCC_ASSERT_LESS_OR_EQUAL( Expected, Actual );						\
	CFIXCC_ASSERT_LESS_OR_EQUAL_MESSAGE( Expected, Actual, "%d%s", 1, "123" );	\
	CFIXCC_ASSERT_GREATER_OR_EQUAL( Actual, Expected );					\
	CFIXCC_ASSERT_GREATER_OR_EQUAL_MESSAGE( Actual, Expected, "%d%s", 1, "123" );	\

#define TEST_SUITE_NAME Normal
#include <assertions.cpp>

CFIX_BEGIN_FIXTURE( AnsiAssertions )
	CFIX_FIXTURE_SETUP( Setup )
	CFIX_FIXTURE_ENTRY( SucceedingEquals )
	CFIX_FIXTURE_ENTRY( SucceedingLess )
	CFIXCC_FIXTURE_ENTRY_EXPECT_CPP_EXCEPTION( ThrowTestException, TestException )
	CFIX_FIXTURE_ENTRY( Nulls )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( AnsiFailedAssertions )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEqualsNull > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEqualsObject > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEqualsString > )
	CFIX_FIXTURE_ENTRY( EXPECT_MISSING_EXCEPTION( ThrowNoException, TestException ) )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( AnsiLog )
	CFIX_FIXTURE_ENTRY( TestLog )
CFIX_END_FIXTURE()
