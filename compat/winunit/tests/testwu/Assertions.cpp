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

static LONG ToStringCalls = 0;

#pragma warning( disable: 4702 )	// Unreachable code

#define TEST_EQ( Expected, Actual )										\
	WIN_ASSERT_EQUAL( Expected, Actual );								\
	WIN_ASSERT_EQUAL( Expected, Actual, "test" );						\
	WIN_ASSERT_EQUAL( Expected, Actual, "test %d, %s", 1, "" );			\
	WIN_ASSERT_EQUAL( Expected, Actual, L"test" );						\
	WIN_ASSERT_EQUAL( Expected, Actual, L"test %d, %s", 1, L"" );		\
	WIN_ASSERT_TRUE( Expected == Actual );								\
	WIN_ASSERT_TRUE( Expected == Actual, "test" );						\
	WIN_ASSERT_TRUE( Expected == Actual, "test %d", L"%s", 1, "" );		\
	WIN_ASSERT_TRUE( Expected == Actual, L"test" );						\
	WIN_ASSERT_TRUE( Expected == Actual, L"test %d", L"%s", 1, "" );	\
	WIN_ASSERT_FALSE( Expected != Actual );								\
	WIN_ASSERT_FALSE( Expected != Actual, "test" );						\
	WIN_ASSERT_FALSE( Expected != Actual, "test %d", "%s", 1, "" );		\
	WIN_ASSERT_FALSE( Expected != Actual, L"test" );					\
	WIN_ASSERT_FALSE( Expected != Actual, L"test %d", L"%s", 1, "" );	


#define TEST_EQ_STR( Expected, Actual )									\
	WIN_ASSERT_STRING_EQUAL( Expected, Actual );						\
	WIN_ASSERT_STRING_EQUAL( Expected, Actual, "test" );				\
	WIN_ASSERT_STRING_EQUAL( Expected, Actual, "test %d", "%s", 1, "" );\
	WIN_ASSERT_STRING_EQUAL( Expected, Actual, L"test" );				\
	WIN_ASSERT_STRING_EQUAL( Expected, Actual, L"test %d", L"%s", 1, "" );

#define TEST_EQ_STR_A( Expected, Actual )								\
	TEST_EQ_STR( Expected, Actual );									\
	WIN_ASSERT_ZERO( strcmp( Expected, Expected ) );					\
	WIN_ASSERT_ZERO( strcmp( Expected, Actual ) );						\
	WIN_ASSERT_ZERO( strcmp( Expected, Actual ), "test"  );				\
	WIN_ASSERT_ZERO( strcmp( Expected, Actual ), "test %d", "%s", 1, "" );		\
	WIN_ASSERT_ZERO( strcmp( Expected, Actual ), L"test"  );			\
	WIN_ASSERT_ZERO( strcmp( Expected, Actual ), L"test %d", L"%s", 1, "" );	\
	WIN_ASSERT_TRUE( 0 == strcmp( Expected, Expected ) );				\
	WIN_ASSERT_FALSE( 0 != strcmp( Expected, Expected ) );				\
	WIN_ASSERT_NOT_ZERO( strcmp( Expected, "____________" ) );			

#define TEST_EQ_STR_W( Expected, Actual )								\
	TEST_EQ_STR( Expected, Actual );									\
	WIN_ASSERT_ZERO( wcscmp( Expected, Expected ) );					\
	WIN_ASSERT_ZERO( wcscmp( Expected, Actual ) );						\
	WIN_ASSERT_ZERO( wcscmp( Expected, Actual ), "test"  );				\
	WIN_ASSERT_ZERO( wcscmp( Expected, Actual ), "test %d", "%s", 1, "" );		\
	WIN_ASSERT_ZERO( wcscmp( Expected, Actual ), L"test"  );			\
	WIN_ASSERT_ZERO( wcscmp( Expected, Actual ), L"test %d", L"%s", 1, "" );	\
	WIN_ASSERT_TRUE( 0 == wcscmp( Expected, Expected ) );				\
	WIN_ASSERT_FALSE( 0 != wcscmp( Expected, Expected ) );				\
	WIN_ASSERT_NOT_ZERO( wcscmp( Expected, L"____________" ) );			


class SomeClass
{
public:
	int value;

	explicit SomeClass( int val ) : value( val )
	{
	}

	bool operator == ( const SomeClass& other ) const
	{
		return this->value == other.value;
	}

	bool operator != ( const SomeClass& other ) const
	{
		return this->value != other.value;
	}

private:
	//
	// Never copy objects of this class!
	//
	SomeClass( const SomeClass& );
	const SomeClass& operator=( const SomeClass& );
};

class SomeOtherClass
{
public:
	int value;

	explicit SomeOtherClass( int val ) : value( val )
	{
	}

	bool operator == ( const SomeOtherClass& other ) const
	{
		return this->value == other.value;
	}

	bool operator != ( const SomeOtherClass& other ) const
	{
		return this->value != other.value;
	}

private:
	//
	// Never copy objects of this class!
	//
	SomeOtherClass( const SomeOtherClass& );
	const SomeOtherClass& operator=( const SomeOtherClass& );
};

template<> 
PCTSTR WinUnit::ToString(
	__in const SomeClass& Object, 
    __in_ecount( BufferSize ) PTSTR Buffer, 
	__in size_t BufferSize 
	)
{
	ToStringCalls++;

	StringCchPrintf( Buffer, BufferSize, L"{{%d}}", Object.value );
    return Buffer;
}

static void Trace()
{
	WIN_TRACE( "" );
	WIN_TRACE( "(%s)", "test" );

	WIN_TRACE( L"" );
	WIN_TRACE( L"(%s)", L"test" );
}

static void SucceedingEquals()
{
	TEST_EQ( true, true );
	TEST_EQ( 1, 1 );
	TEST_EQ( -1, -1 );
	TEST_EQ( 0, 0 );
	TEST_EQ( 0xFFFFFFFFUL, 0xFFFFFFFFUL );
	TEST_EQ( 1.1f, 1.1f );
	TEST_EQ( 2.0, 2.0 );
	TEST_EQ( 2.0f, 2.0f );
	TEST_EQ_STR_A( "", "" );
	TEST_EQ_STR_A( "te  st", "te  st" );
	TEST_EQ_STR_W( L"", L"" );
	TEST_EQ_STR_W( L"te st", L"te st" );
	TEST_EQ( SomeClass( 1 ), SomeClass( 1 ) );

	SomeClass *p = new SomeClass( 1 );
	TEST_EQ( p, p );
	TEST_EQ( *p, *p );
	delete p;

	WCHAR w1[] = L"te st";
	WCHAR w2[] = L"te st";
	TEST_EQ_STR_W( w1, w2 );
	
	CHAR c1[] = "te st";
	CHAR c2[] = "te st";
	TEST_EQ_STR_A( c1, c2 );
	TEST_EQ_STR_A( c1, ( PSTR ) "te st" );

	PWSTR pwstr1 = L"t";
	PWSTR pwstr2 = L"t";
	TEST_EQ_STR_W( pwstr1, pwstr2 );

	PCWSTR pcwstr1 = L"t";
	PCWSTR pcwstr2 = L"t";
	TEST_EQ_STR_W( pcwstr1, pcwstr2 );

	TEST_EQ( std::string( "" ), std::string( "" ) );
	TEST_EQ( std::wstring( L"" ), std::wstring( L"" ) );

	TEST_EQ( SomeClass( 1 ), SomeClass( 1 ) );

	//
	// Floating pt.
	//
	WIN_ASSERT_EQUAL( 1.9999999f, 2.0f );
	WIN_ASSERT_EQUAL( 1.99999999999999f, 2.0 );
	WIN_ASSERT_EQUAL( 2.0, 1.99999999999999f );
	WIN_ASSERT_EQUAL( 2.0f, 1.9999999f );
	WIN_ASSERT_EQUAL( 2.0, 2 );
	WIN_ASSERT_EQUAL( 2.0, 2UL );
	WIN_ASSERT_EQUAL( 2.0, 2.0 );

	float negativeZero;
    *( int* )&negativeZero = 0x80000000;
    WIN_ASSERT_EQUAL( negativeZero, 0.0f );
}

static void CompareStrings()
{
	PSTR PstrFoo = "foo";
	PCSTR PcstrFoo = "foo";
	PWSTR PwstrFoo = L"foo";
	PCWSTR PcwstrFoo = L"foo";

	WIN_ASSERT_STRING_EQUAL( PstrFoo, PstrFoo );
	WIN_ASSERT_STRING_EQUAL( PstrFoo, PcstrFoo );

	WIN_ASSERT_STRING_EQUAL( PcstrFoo, PstrFoo );
	WIN_ASSERT_STRING_EQUAL( PcstrFoo, PcstrFoo );

	WIN_ASSERT_STRING_EQUAL( PwstrFoo, PwstrFoo );
	WIN_ASSERT_STRING_EQUAL( PwstrFoo, PcwstrFoo );

	WIN_ASSERT_STRING_EQUAL( PcwstrFoo, PwstrFoo );
	WIN_ASSERT_STRING_EQUAL( PcwstrFoo, PcwstrFoo );
}

static void CompareZero()
{
	SHORT s = 0;
	USHORT us = 0;
	LONG l = 0L;
	ULONG ul = 0UL;
	LONGLONG ll = 0LL;
	ULONGLONG ull = 0ULL;

	WIN_ASSERT_ZERO( s );
	WIN_ASSERT_ZERO( us );
	WIN_ASSERT_ZERO( l );
	WIN_ASSERT_ZERO( ul );
	WIN_ASSERT_ZERO( ll );
	WIN_ASSERT_ZERO( ull );
}

static void CompareNonZero()
{
	SHORT s = -1;
	USHORT us = 1;
	LONG l = -1L;
	ULONG ul = 1UL;
	LONGLONG ll = -1LL;
	ULONGLONG ull = 1ULL;

	WIN_ASSERT_NOT_ZERO( s );
	WIN_ASSERT_NOT_ZERO( us );
	WIN_ASSERT_NOT_ZERO( l );
	WIN_ASSERT_NOT_ZERO( ul );
	WIN_ASSERT_NOT_ZERO( ll );
	WIN_ASSERT_NOT_ZERO( ull );
}

static void CompareNull()
{
	PWSTR p = NULL;
	WIN_ASSERT_NULL( p );
	WIN_ASSERT_NULL( NULL );
}

static void CompareNotNull()
{
	PWSTR p = L"";
	WIN_ASSERT_NOT_NULL( &CompareNotNull );
	WIN_ASSERT_NOT_NULL( p );
	WIN_ASSERT_NOT_NULL( L"test" );
}

static void CompareTrue()
{
	int a = 0;
	WIN_ASSERT_TRUE( TRUE );
	WIN_ASSERT_TRUE( true );
	WIN_ASSERT_TRUE( a == 0 );
}

static void CompareFalse()
{
	int a = 0;
	WIN_ASSERT_FALSE( FALSE );
	WIN_ASSERT_FALSE( false );
	WIN_ASSERT_FALSE( a != 0 );
}

static void FailingEquals01()
{
	WIN_ASSERT_EQUAL( L"", L" " );
}

static void FailingEquals02()
{
	WIN_ASSERT_EQUAL( L"", ( PCWSTR ) NULL, "" );
}

static void FailingEquals03()
{
	WIN_ASSERT_STRING_EQUAL( L"", ( PCWSTR ) NULL, "" );
}

static void FailingEqualsWithToString()
{
	//
	// Must call ToString twice.
	//
	WIN_ASSERT_EQUAL( SomeClass( 1 ), SomeClass( 2 ) );
}

static void FailingEqualsWithMissingToString()
{
	//
	// No appropriate ToString available.
	//
	WIN_ASSERT_EQUAL( SomeOtherClass( 1 ), SomeOtherClass( 2 ) );
}

static void FailingNotEquals01()
{
	WIN_ASSERT_NOT_EQUAL( L"", L"", "" );
}

static void FailingZero()
{
	WIN_ASSERT_ZERO( 1, "" );
}

static void FailingNotZero()
{
	WIN_ASSERT_NOT_ZERO( 0, "" );
}

static void FailingNull()
{
	WIN_ASSERT_NULL( &FailingNull, "" );
}

static void FailingNotNull()
{
	WIN_ASSERT_NOT_NULL( NULL, "" );
}

static void Fail()
{
	WIN_ASSERT_FAIL( L"" );
}

static void FailingTrue()
{
#pragma warning( push )
#pragma warning( disable: 4800 )
	int *a = NULL;
	WIN_ASSERT_TRUE( a );
#pragma warning( pop )
}

static void FailingFalse()
{
	WIN_ASSERT_FALSE( true );
}


class TestException {};
static void RaiseTestException()
{
	throw TestException();
}

static void SucceedingThrows()
{
	WIN_ASSERT_THROWS( RaiseTestException(), TestException );
}

static void FailingThrows()
{
	WIN_ASSERT_THROWS( NULL, TestException );
}

static void Teardown()
{
	WIN_ASSERT_EQUAL( 2, ToStringCalls );
}

static void FailingFails()
{
	WIN_ASSERT_FAIL( L"" );
}


/*----------------------------------------------------------------------
 *
 * Helper constructs for expect-failure cases.
 *
 */

static BOOL FailureReported = FALSE;

static CFIX_REPORT_DISPOSITION CFIXCALLTYPE ExpectReportFailedAssertion(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in UINT Line,
	__in PCWSTR Expression
	)
{
	UNREFERENCED_PARAMETER( File );
	UNREFERENCED_PARAMETER( Routine );
	UNREFERENCED_PARAMETER( Line );

#ifdef UNICODE
	CFIX_LOG( L"Expression: %s", Expression );
#else
	CFIX_LOG( "Expression: %S", Expression );
#endif

	FailureReported = TRUE;
	return CfixContinue;
}

template< CFIX_PE_TESTCASE_ROUTINE RoutineT >
static void ExpectFailure()
{
	PVOID OldProc;
	CFIX_ASSERT( S_OK == PatchIat(
		GetModuleHandleW( L"testwu" ),
		"cfix.dll",
		"CfixPeReportFailedAssertion",
		( PVOID ) ExpectReportFailedAssertion,
		&OldProc ) );

	__try
	{
		FailureReported = FALSE;
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

	CFIX_ASSERT( FailureReported );
}

CFIX_BEGIN_FIXTURE( Assertions )
	CFIX_FIXTURE_ENTRY( Trace )
	CFIX_FIXTURE_ENTRY( SucceedingEquals )
	CFIX_FIXTURE_ENTRY( CompareStrings )
	CFIX_FIXTURE_ENTRY( CompareZero )
	CFIX_FIXTURE_ENTRY( CompareNonZero )
	CFIX_FIXTURE_ENTRY( CompareNull )
	CFIX_FIXTURE_ENTRY( CompareNotNull )
	CFIX_FIXTURE_ENTRY( CompareTrue )
	CFIX_FIXTURE_ENTRY( CompareFalse )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEquals01 > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEquals02 > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEquals03 > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEqualsWithToString > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingEqualsWithMissingToString > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingNotEquals01 > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingZero > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingNotZero > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingNull > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingNotNull > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< Fail > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingTrue > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingFalse > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingThrows > )
	CFIX_FIXTURE_ENTRY( ExpectFailure< FailingFails > )
	CFIX_FIXTURE_ENTRY( SucceedingThrows )
	CFIX_FIXTURE_TEARDOWN( Teardown )
CFIX_END_FIXTURE()