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

#include <winunit.h>
#include "iatpatch.h"

static PCWSTR ExpectedAssertionExpression;

static void FailCompareInt()
{
	ExpectedAssertionExpression = L"test: [1] == [2] (Expression: 1 == 2)";
	WIN_ASSERT_EQUAL( 1, 2, "test" );
}

static void FailCompareULong()
{
	ExpectedAssertionExpression = L"test: [1] == [2] (Expression: 1UL == 2UL)";
	WIN_ASSERT_EQUAL( 1UL, 2UL, "test" );
}

static void FailCompareFloat()
{
	ExpectedAssertionExpression = L"test: [1.1] == [2.2] (Expression: 1.1f == 2.2f)";
	WIN_ASSERT_EQUAL( 1.1f, 2.2f, "test" );
}

static void FailCompareDouble()
{
	ExpectedAssertionExpression = L"test: [1.1] == [2.2] (Expression: 1.1 == 2.2)";
	WIN_ASSERT_EQUAL( 1.1, 2.2, "test" );
}

static void FailComparePcstr()
{
	ExpectedAssertionExpression = L"test: [a] == [b] (Expression: \"a\" == \"b\")";
	WIN_ASSERT_STRING_EQUAL( "a", "b", "test" );
}

static void FailComparePcwstr()
{
	ExpectedAssertionExpression = L"test: [a] == [b] (Expression: L\"a\" == L\"b\")";
	WIN_ASSERT_STRING_EQUAL( L"a", L"b", "test" );
}


/*----------------------------------------------------------------------
 *
 * Helper constructs for expect-failure cases.
 *
 */

static WCHAR ActualAssertionExpression[ 100 ] = { 0 };

static CFIX_REPORT_DISPOSITION CFIXCALLTYPE ExpectFailedAssertionMessage(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in UINT Line,
	__in PCWSTR Expression
	)
{
	UNREFERENCED_PARAMETER( File );
	UNREFERENCED_PARAMETER( Routine );
	UNREFERENCED_PARAMETER( Line );

	StringCchCopy(
		ActualAssertionExpression,
		_countof( ActualAssertionExpression ),
		Expression );

	return CfixContinue;
}

static void Check( __in PCWSTR ExpectedExpr, __in PCWSTR ActualExpr )
{
	WIN_ASSERT_STRING_EQUAL( ExpectedExpr, ActualExpr );
}

template< CFIX_PE_TESTCASE_ROUTINE RoutineT >
static void ExpectAssertionExpression()
{
	PVOID OldProc;
	CFIX_ASSERT( S_OK == PatchIat(
		GetModuleHandleW( L"testwu" ),
		"cfix.dll",
		"CfixPeReportFailedAssertion",
		( PVOID ) ExpectFailedAssertionMessage,
		&OldProc ) );

	__try
	{
		ActualAssertionExpression[ 0 ] = L'\0';
		RoutineT();
	}
	__finally
	{
		CFIX_ASSERT( S_OK == PatchIat(
			GetModuleHandleW( L"testwu" ),
			"cfix.dll",
			"CfixPeReportFailedAssertion",
			OldProc,
			NULL ) );
	}

	Check( ExpectedAssertionExpression, ActualAssertionExpression );
}



CFIX_BEGIN_FIXTURE( AssertionMessages )
	CFIX_FIXTURE_ENTRY( ( ExpectAssertionExpression< FailCompareInt > ) )
	CFIX_FIXTURE_ENTRY( ( ExpectAssertionExpression< FailCompareULong > ) )
	CFIX_FIXTURE_ENTRY( ( ExpectAssertionExpression< FailCompareDouble > ) )
	CFIX_FIXTURE_ENTRY( ( ExpectAssertionExpression< FailCompareFloat > ) )
	CFIX_FIXTURE_ENTRY( ( ExpectAssertionExpression< FailComparePcstr > ) )
	CFIX_FIXTURE_ENTRY( ( ExpectAssertionExpression< FailComparePcwstr > ) )
CFIX_END_FIXTURE()
