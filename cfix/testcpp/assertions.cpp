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

//
// N.B. This file is for inclusion by other cpp files!
//

#include "iatpatch.h"

class SomeClass
{
private:
	int value;

public:
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

	bool operator < ( const SomeClass& other ) const
	{
		return this->value < other.value;
	}

	bool operator <= ( const SomeClass& other ) const
	{
		return this->value <= other.value;
	}

	bool operator > ( const SomeClass& other ) const
	{
		return this->value > other.value;
	}

	bool operator >= ( const SomeClass& other ) const
	{
		return this->value >= other.value;
	}

private:
	//
	// Never copy objects of this class!
	//
	SomeClass( const SomeClass& );
	const SomeClass& operator=( const SomeClass& );

};

static void Nulls()
{
	CFIXCC_ASSERT( TRUE );
	CFIXCC_LOG( NULL );

#ifndef CFIXCC_NO_VARIADIC_ASSERTIONS
	CFIXCC_ASSERT_MESSAGE( TRUE, NULL );
#endif
}

static void SucceedingEquals()
{
	CFIX_ASSERT_EXPR( TRUE, L"test" );

#ifdef UNICODE
#ifndef CFIXCC_NO_VARIADIC_ASSERTIONS
	CFIX_ASSERT_MESSAGE( TRUE, L"test" );
	CFIX_ASSERT_MESSAGE( TRUE, L"test %d%s", 1, L"123" );
#endif
	CFIX_LOG( L"test" );
	CFIX_LOG( L"test %d%s", 1, L"123" );
	//CFIX_INCONCLUSIVE( L"test" );
	//CFIX_INCONCLUSIVE( L"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789" );
#else
#ifndef CFIXCC_NO_VARIADIC_ASSERTIONS
	CFIX_ASSERT_MESSAGE( TRUE, "test %d%s", 1, "123" );
#endif
	CFIX_LOG( "test" );
	CFIX_LOG( "test %d%s", 1, "123" );
	//CFIX_INCONCLUSIVE( "test" );
	//CFIX_INCONCLUSIVE( "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789" );
#endif

	TEST_EQ( true, true );
	TEST_EQ( 1, 1 );
	TEST_EQ( -1, -1 );
	TEST_EQ( 0, 0 );
	TEST_EQ( 0xFFFFFFFFUL, 0xFFFFFFFFUL );
	TEST_EQ( 1.1f, 1.1f );
	TEST_EQ( 2.0, 2.0 );
	TEST_EQ( 2.0f, 2.0f );
	TEST_EQ( "", "" );
	TEST_EQ( "te  st", "te  st" );
	TEST_EQ( L"", L"" );
	TEST_EQ( L"te st", L"te st" );
	TEST_EQ( SomeClass( 1 ), SomeClass( 1 ) );

	SomeClass *p = new SomeClass( 1 );
	TEST_EQ( p, p );
	TEST_EQ( *p, *p );
	delete p;

	WCHAR w1[] = L"te st";
	WCHAR w2[] = L"te st";
	TEST_EQ( w1, w2 );
	
	CHAR c1[] = "te st";
	CHAR c2[] = "te st";
	TEST_EQ( c1, c2 );
	TEST_EQ( c1, ( PSTR ) "te st" );

	PWSTR pwstr1 = L"t";
	PWSTR pwstr2 = L"t";
	TEST_EQ( pwstr1, pwstr2 );

	PCWSTR pcwstr1 = L"t";
	PCWSTR pcwstr2 = L"t";
	TEST_EQ( pcwstr1, pcwstr2 );

	TEST_EQ( std::string( "" ), std::string( "" ) );
	TEST_EQ( std::wstring( L"" ), std::wstring( L"" ) );

	TEST_EQ( SomeClass( 1 ), SomeClass( 1 ) );

	//
	// Floating pt.
	//
	CFIXCC_ASSERT_EQUALS( 1.9999999f, 2.0f );
	CFIXCC_ASSERT_EQUALS( 2.0f, 1.9999999f );
	CFIXCC_ASSERT_EQUALS( 2.0, 2.0 );
	
	float negativeZero;
    *( int* )&negativeZero = 0x80000000;
    CFIXCC_ASSERT_EQUALS( negativeZero, 0.0f );
}

static void SucceedingLess()
{
	TEST_LT( false, true );
	TEST_LT( 1, 1000000 );
	TEST_LT( -3, -2 );
	TEST_LT( 0, 1 );
	TEST_LT( 0xFF1FFFFFUL, 0xFFFFFFFFUL );
	TEST_LT( 1.1f, 1.11f );
	TEST_LT( 987654.1, 987654.123 );
	TEST_LT( "", " " );
	TEST_LT( "a", "x" );
	TEST_LT( L"test1", L"test2" );
	TEST_LT( SomeClass( 1 ), SomeClass( 2 ) );
	
	SomeClass *p1 = new ( std::nothrow ) SomeClass( 1 );
	SomeClass *p2 = new ( std::nothrow ) SomeClass( 1 );
	if ( p1 < p2 )
	{
		TEST_LT( p1, p2 );
	}
	else
	{
		TEST_LT( p2, p1 );
	}

	delete p1;
	delete p2;

	WCHAR w1[] = L"test1";
	WCHAR w2[] = L"test2";
	TEST_LT( w1, w2 );
	
	CHAR c1[] = "test1";
	CHAR c2[] = "test2";
	TEST_LT( c1, c2 );
	TEST_LT( c1, ( PSTR ) "test3" );

	TEST_LT( std::string( "a" ), std::string( "b" ) );
	TEST_LT( std::wstring( L"a" ), std::wstring( L"b" ) );
}

static void FailingEqualsNull()
{
	TEST_EQ( L"", NULL );
}

static void FailingEqualsString()
{
	TEST_EQ( L"", L"1" );
}

static void FailingEqualsObject()
{
	TEST_EQ( SomeClass( 1 ), SomeClass( 2 ) );
}

#ifndef CFIXCC_NO_VARIADIC_ASSERTIONS

static void Fail01()
{
	CFIX_FAIL( NULL );
}

static void Fail02()
{
	CFIX_FAIL( __TEXT( "" ) );
}

static void Fail03()
{
	CFIX_FAIL( __TEXT( "!" ) );
}

#ifndef UNICODE
static void FailAnsi()
{
	CFIX_FAIL( "Ansi" );
}

#endif // UNICODE
#endif // CFIXCC_NO_VARIADIC_ASSERTIONS

struct TestException
{
};

struct UnexpectedException
{
};

static void ThrowTestException()
{
	throw TestException();
}

static void ThrowNoException()
{
}

static void Setup()
{
//	CFIX_ASSERT( FALSE );
}

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

static CFIX_REPORT_DISPOSITION CFIXCALLTYPE ExpectReportFailedAssertionFormatA(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in __format_string PCSTR Format,
	...
	)
{
	UNREFERENCED_PARAMETER( File );
	UNREFERENCED_PARAMETER( Routine );
	UNREFERENCED_PARAMETER( Line );
	UNREFERENCED_PARAMETER( Format );

	FailureReported = TRUE;
	return CfixContinue;
}

static CFIX_REPORT_DISPOSITION CFIXCALLTYPE ExpectReportFailedAssertionFormatW(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in __format_string PCWSTR Format,
	...
	)
{
	UNREFERENCED_PARAMETER( File );
	UNREFERENCED_PARAMETER( Routine );
	UNREFERENCED_PARAMETER( Line );
	UNREFERENCED_PARAMETER( Format );

	FailureReported = TRUE;
	return CfixContinue;
}

template< CFIX_PE_TESTCASE_ROUTINE RoutineT >
void ExpectFailure()
{
	PVOID OldReportFailedAssertion;
	PVOID OldReportFailedAssertionFormatA;
	PVOID OldReportFailedAssertionFormatW;
	HMODULE TestcppHandle = GetModuleHandleW( L"testcpp" );

	CFIX_ASSUME( TestcppHandle != NULL );
	
	CFIX_ASSERT( S_OK == PatchIat(
		TestcppHandle,
		"cfix.dll",
		"CfixPeReportFailedAssertion",
		( PVOID ) ExpectReportFailedAssertion,
		&OldReportFailedAssertion ) );
	CFIX_ASSERT( S_OK == PatchIat(
		TestcppHandle,
		"cfix.dll",
		"CfixPeReportFailedAssertionFormatA",
		( PVOID ) ExpectReportFailedAssertionFormatA,
		&OldReportFailedAssertionFormatA ) );
	CFIX_ASSERT( S_OK == PatchIat(
		TestcppHandle,
		"cfix.dll",
		"CfixPeReportFailedAssertionFormatW",
		( PVOID ) ExpectReportFailedAssertionFormatW,
		&OldReportFailedAssertionFormatW ) );

	__try
	{
		FailureReported = FALSE;
		RoutineT();
	}
	__finally
	{
		CFIX_ASSERT( S_OK == PatchIat(
			TestcppHandle,
			"cfix.dll",
			"CfixPeReportFailedAssertion",
			OldReportFailedAssertion,
			NULL ) );
		CFIX_ASSERT( S_OK == PatchIat(
			TestcppHandle,
			"cfix.dll",
			"CfixPeReportFailedAssertionFormatA",
			OldReportFailedAssertionFormatA,
			NULL ) );
		CFIX_ASSERT( S_OK == PatchIat(
			TestcppHandle,
			"cfix.dll",
			"CfixPeReportFailedAssertionFormatW",
			OldReportFailedAssertionFormatW,
			NULL ) );
	}

	CFIX_ASSERT( FailureReported );
}


#define EXPECT_MISSING_EXCEPTION( Routine, Exception )					\
	ExpectFailure<														\
		cfixcc::ExpectExceptionTestRoutineWrapper<						\
			Exception,													\
			Routine > >


static void TestLog()
{
	CFIXCC_LOG( L"log unicode" );
	CFIXCC_LOG( L"%s", L"log unicode %s%s%s%p%p%p" );
	
	CFIXCC_LOG( "log ansi" );
	CFIXCC_LOG( "%s", "log ansi %s%s%s%p%p%p" );

	CFIXCC_LOG( std::wstring( L"log unicode stl %s%s%s%p%p%p" ) );
	CFIXCC_LOG( std::string( "log ansi stl %s%s%s%p%p%p" ) );

	CFIX_LOG( L"log unicode" );
	CFIX_LOG( L"%s", L"log unicode %s%s%s%p%p%p" );
	
	CFIX_LOG( "log ansi" );
	CFIX_LOG( "%s", "log ansi %s%s%s%p%p%p" );

	CFIX_LOG( std::wstring( L"log unicode stl %s%s%s%p%p%p" ) );
	CFIX_LOG( std::string( "log ansi stl %s%s%s%p%p%p" ) );
}