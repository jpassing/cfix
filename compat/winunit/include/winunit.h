#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Main header file for WinUnit compatibility.
 *		Compatible to WinUnit 1.0.1125.0.
 *
 * Compatibility Limitations:
 *		- Winunit::Assert not supported, use the appropriate Macros.
 *		- The cfix implementation of WIN_ASSERT_EQUAL compares strings 
 *		  by-value, which WinUnit does not.
 *		- cfix allows mixing of ANSI and UNICODE, WinUnit is more strict 
 *		  in that it relies on TSTRs.
 *		- No support for custom loggers.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * N.B. This file does not include any of the original nor directly derived 
 *		parts of the WinUnit source code and is therefore not subject to the
 *		MS EULA.
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
// We are redefining cfixcc::operator<<.
//
#define CFIXCC_OMIT_DEFAULT_OUTPUT_OPERATOR

#if _MSC_VER < 1400 
#error cl 14.00 or better required
#elif defined( __cplusplus ) && ! defined( CFIX_KERNELMODE )
#include <tchar.h>
#include <cfixcc.h>
#include <vector>

/*----------------------------------------------------------------------
 *
 * Standalone Test Case Definition.
 *
 */

/*++
	Macro Description:
		Standalone test case. Exposed as cfix fixture containing a single
		test case only.
--*/
#define BEGIN_TEST( TestCaseName )										\
	static void CFIXCALLTYPE TestCaseName();							\
	EXTERN_C __declspec(dllexport)										\
	PCFIX_TEST_PE_DEFINITION CFIXCALLTYPE __CfixFixturePe##TestCaseName()	\
	{																	\
		static CFIX_PE_DEFINITION_ENTRY Entries[] = {					\
		{ CfixEntryTypeTestcase, __CFIX_WIDE( #TestCaseName ), TestCaseName },	\
		{ CfixEntryTypeEnd, NULL, NULL }								\
		};																\
		static CFIX_TEST_PE_DEFINITION Fixture = {						\
			CFIX_PE_API_MAKEAPIVERSION( CfixApiTypeWinUnit, 0 ),		\
			Entries														\
		};																\
		return &Fixture;												\
	}																	\
	static void CFIXCALLTYPE TestCaseName()								\

#define END_TEST

/*----------------------------------------------------------------------
 *
 * Fixture Definition.
 *
 */

namespace cfixwu
{
	/*++
		Class Description:
			Helper class that allows dynamic composition of fixtures.
	--*/
	class DynamicFixture
	{
	private:
		struct SELF_RELATIVE_DEFINITION
		{
			CFIX_TEST_PE_DEFINITION Header;
			CFIX_PE_DEFINITION_ENTRY Entries[ ANYSIZE_ARRAY ];
		};

		std::vector< CFIX_PE_DEFINITION_ENTRY > Entries;
		SELF_RELATIVE_DEFINITION *Definition;
		
		bool IsSealed()
		{
			return Definition != NULL;
		}

	public:
		DynamicFixture() : Definition( NULL )
		{
		}

		~DynamicFixture()
		{
			if ( this->Definition != NULL )
			{
				free( this->Definition );
			}
		}

		void Add( 
			__in CFIX_ENTRY_TYPE Type,
			__in CFIX_PE_TESTCASE_ROUTINE Routine,
			__in PCWSTR Name
			)
		{
			if ( ! this->IsSealed() )
			{
				CFIX_PE_DEFINITION_ENTRY Entry;
				Entry.Type		= Type; 
				Entry.Name		= Name;
				Entry.Routine	= Routine;

				this->Entries.push_back( Entry );
			}
		}

		PCFIX_TEST_PE_DEFINITION GetDefinition()
		{
			if ( this->Definition != NULL )
			{
				return &this->Definition->Header;
			}

			//
			// Derive definition.
			//
			this->Definition = ( SELF_RELATIVE_DEFINITION* ) malloc(
				RTL_SIZEOF_THROUGH_FIELD(
					SELF_RELATIVE_DEFINITION,
					Entries[ this->Entries.size() ] ) );
			if ( this->Definition == NULL )
			{
				return NULL;
			}

			//
			// Base structure.
			//
			this->Definition->Header.ApiVersion	= 
				CFIX_PE_API_MAKEAPIVERSION( CfixApiTypeWinUnit, 0 );
			this->Definition->Header.Entries	= this->Definition->Entries;

			//
			// Copy entries.
			//
			size_t Index = 0;
			for ( std::vector< CFIX_PE_DEFINITION_ENTRY >::iterator it = this->Entries.begin();
				  it != this->Entries.end();
				  it++ )
			{
				this->Definition->Entries[ Index++ ] = *it;
			}

			//
			// Mark end.
			//
			this->Definition->Entries[ Index ].Type = CfixEntryTypeEnd;

			return &this->Definition->Header;
		}

	private:
		//
		// Objects should never be copied.
		//
		DynamicFixture( const DynamicFixture& );
		const DynamicFixture& operator = ( const DynamicFixture& );
	};

	/*++
		Class Description:
			Helper class that allows initialization-time addition of 
			a routine to a dynamic fixture.
	--*/
	class DynamicFixtureAdder
	{
	public:
		DynamicFixtureAdder(
			__in DynamicFixture& Fixture,
			__in CFIX_ENTRY_TYPE Type,
			__in CFIX_PE_TESTCASE_ROUTINE Routine,
			__in PCWSTR Name
			)
		{
			Fixture.Add( Type, Routine, Name );
		}

	private:
		//
		// Objects should never be copied.
		//
		DynamicFixtureAdder( const DynamicFixtureAdder& );
		const DynamicFixtureAdder& operator = ( const DynamicFixtureAdder& );
	};
}

/*++
	Macro Description:
		Created an accumulator along with an appropriate cfix export that
		serves as the basis for a fixture definition.
--*/
#define FIXTURE( FixtureName )											\
	static cfixwu::DynamicFixture& __GetDefinition##FixtureName()		\
	{																	\
		static cfixwu::DynamicFixture Fixture;							\
		return Fixture;													\
	}																	\
	EXTERN_C __declspec(dllexport)										\
	PCFIX_TEST_PE_DEFINITION CFIXCALLTYPE __CfixFixturePe##FixtureName()\
	{																	\
		return __GetDefinition##FixtureName##().GetDefinition();								\
	}																	

/*++
	Macro Description:
		Defines the setup routine for a given fixture.

		N.B. WinUnit Setup routines correspond to cfix Before routines.
--*/
#define SETUP( FixtureName )											\
	static void CFIXCALLTYPE FixtureName##_Setup();						\
	static cfixwu::DynamicFixtureAdder __FixtureName##_SetupAdder(		\
		__GetDefinition##FixtureName(),									\
		CfixEntryTypeBefore,											\
		FixtureName##_Setup,											\
		L"Setup" );														\
	static void CFIXCALLTYPE FixtureName##_Setup()						

/*++
	Macro Description:
		Defines the setup routine for a given fixture.
		
		N.B. WinUnit Teardown routines correspond to cfix After routines.
--*/
#define TEARDOWN( FixtureName )											\
	static void CFIXCALLTYPE FixtureName##_Teardown();					\
	static cfixwu::DynamicFixtureAdder __FixtureName##_TeardownAdder(	\
		__GetDefinition##FixtureName(),									\
		CfixEntryTypeAfter,												\
		FixtureName##_Teardown,											\
		L"Teardown" );													\
	static void CFIXCALLTYPE FixtureName##_Teardown()					

/*++
	Macro Description:
		Defines a test routine for a given fixture.
--*/
#define BEGIN_TESTF( TestName, FixtureName )							\
	static void CFIXCALLTYPE FixtureName##_##TestName();				\
	static cfixwu::DynamicFixtureAdder __FixtureName##_##TestName##Adder(	\
		__GetDefinition##FixtureName(),									\
		CfixEntryTypeTestcase,											\
		FixtureName##_##TestName,										\
		__CFIX_WIDE( #TestName ) );										\
	static void CFIXCALLTYPE FixtureName##_##TestName()	

#define END_TESTF

/*----------------------------------------------------------------------
 *
 * Assertions.
 *
 */

namespace cfixwu
{
	/*++
		Class Description:
			Wrapper for assertion messages. Supports NULL messages
			and expects a dummy first parameter which is required as 
			a workaround for certain cl 14 strangenesses regarding 
			empty __VA_ARGS__.
	--*/
	class WinunitMessage : public cfixcc::FormattedMessage
	{
	public:
		WinunitMessage(
			int
			)
		{
			MessageString[ 0 ] = L'\0';
		}

		WinunitMessage(
			int,
			__in __format_string PCWSTR Format,
			... 
			)
		{
			va_list Lst;
			va_start( Lst, Format );
			Initialize( Format, Lst );
			va_end( Lst );
		}

		WinunitMessage(
			int,
			__in __format_string PCSTR Format,
			... 
			)
		{
			va_list Lst;
			va_start( Lst, Format );
			Initialize( Format, Lst );
			va_end( Lst );
		}
	};
}

#define WIN_TRACE( Format, ... )										\
	CfixPeReportLog( L"%s",												\
		cfixwu::WinunitMessage( 0, Format, __VA_ARGS__ ).GetMessage() )

#define WIN_ASSERT_EQUAL( Expected, Actual, ... )						\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::Equal >::RelateCompatible(	\
			( Expected ),												\
			( Actual ),													\
			__CFIX_WIDE( #Expected ),									\
			__CFIX_WIDE( #Actual ),										\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )


#define WIN_ASSERT_NOT_EQUAL( NotExpected, Actual, ... )				\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::NotEqual >::RelateCompatible(	\
			( NotExpected ),											\
			( Actual ),													\
			__CFIX_WIDE( #NotExpected ),								\
			__CFIX_WIDE( #Actual ),										\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )

//
// N.B. cfixcc does by-value comparisons of strings by default.
//
#define WIN_ASSERT_STRING_EQUAL( Expected, Actual, ... )				\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::Equal >::RelateStrings(	\
			( Expected ),												\
			( Actual ),													\
			__CFIX_WIDE( #Expected ),									\
			__CFIX_WIDE( #Actual ),										\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )


#define WIN_ASSERT_ZERO( ZeroExpression, ... )							\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::Equal >::Relate< ULONGLONG >(	\
			0ULL,															\
			( ZeroExpression ),											\
			L"0",														\
			__CFIX_WIDE( #ZeroExpression ),								\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	

#define WIN_ASSERT_NOT_ZERO( NonzeroExpression, ... ) 					\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::NotEqual >::Relate< ULONGLONG >(	\
			0ULL,															\
			( NonzeroExpression ),										\
			L"0",														\
			__CFIX_WIDE( #NonzeroExpression ),							\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	

#define WIN_ASSERT_NULL( NullExpression, ... ) 							\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::Equal >::Relate< PVOID >(	\
			NULL,														\
			( NullExpression ),											\
			L"(NULL)",													\
			__CFIX_WIDE( #NullExpression ),								\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	

#define WIN_ASSERT_NOT_NULL( NotNullExpression, ... )  					\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::NotEqual >::Relate< PVOID >(	\
			NULL,														\
			( NotNullExpression ),										\
			L"(NULL)",													\
			__CFIX_WIDE( #NotNullExpression ),							\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	


#define WIN_ASSERT_FAIL( Message, ... )									\
	( void ) ( ( CfixBreak != CfixPeReportFailedAssertion(				\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__,													\
			cfixwu::WinunitMessage( 0, Message, __VA_ARGS__ ).GetMessage() ) ||	\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	


#define WIN_ASSERT_TRUE( TrueExpression, ... )   							\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::Equal >::Relate< bool >(	\
			true,														\
			( bool ) ( !! ( TrueExpression ) ),							\
			L"true",													\
			__CFIX_WIDE( #TrueExpression ),								\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	

#define WIN_ASSERT_FALSE( FalseExpression, ... )						\
	( void ) ( ( CfixBreak != cfixcc::Assertion< cfixcc::Equal >::Relate< bool >(	\
			false,														\
			( bool ) ( !! ( FalseExpression ) ),						\
			L"false",													\
			__CFIX_WIDE( #FalseExpression ),							\
			cfixwu::WinunitMessage( 0, __VA_ARGS__ ),					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__ ) ||												\
		( __debugbreak(), CfixPeFail(), 0 ) ) )	

//
// N.B. The last Win32 error string is printed by default anyway - no
// special handling required here.
//
#define WIN_ASSERT_WINAPI_SUCCESS	WIN_ASSERT_TRUE

#define WIN_ASSERT_THROWS( Expression, ExceptionType, ... )				\
{																		\
    try																	\
	{																	\
		( VOID ) ( Expression );										\
		if ( CfixBreak == CfixPeReportFailedAssertion(					\
			__CFIX_WIDE( __FILE__ ),									\
			__CFIX_WIDE( __FUNCTION__ ),								\
			__LINE__,													\
			L"Expected exception, but none has been raised" ) )			\
		{																\
			__debugbreak();												\
		}																\
	}																	\
    catch( ExceptionType& )												\
	{}																	\
}

/*----------------------------------------------------------------------
 *
 * Output.
 *
 */

namespace WinUnit
{
	/*++
		Template Description:
			Template that is to be explicitly specialized in order to
			output non-primitive objects.
	--*/
	template< typename T >
	inline const TCHAR* ToString(
		__in const T& Object,
		__inout_ecount( BufferCch ) PTSTR Buffer,
		__in size_t BufferCch
		)
	{
		UNREFERENCED_PARAMETER( Object );

		StringCchCopy( Buffer, BufferCch, __TEXT( "(object)" ) );
		return Buffer;
	}
}

namespace cfixcc
{
	/*++
		Template Description:
			Overrides the cfixcc operator<< used for assertion messages
			for non-primitive objects.
	--*/
	template< typename ValueT >
	inline std::wostream& operator <<( 
		__in std::wostream& os, 
		__in const cfixcc::ValueWrapper< ValueT >& val )
	{
		UNREFERENCED_PARAMETER( val );

		TCHAR Buffer[ 256 ];
		return os << WinUnit::ToString( val.Value, Buffer, _countof( Buffer ) );
	}
}

/*----------------------------------------------------------------------
 *
 * Helper Classes.
 *
 */
namespace WinUnit
{
    class Environment
    {
    public:
        static bool GetVariable(
			__in PCTSTR Name, 
            __inout_ecount( BufferCch ) PTSTR Buffer, 
			__in ULONG BufferCch,
            __out_opt ULONG* SizeNeeded = NULL
			);
    private:
        Environment();
		Environment( const Environment& );
		const Environment& operator = ( const Environment& );
    };

    inline bool Environment::GetVariable(
        __in PCTSTR Name, 
        __in_ecount( BufferCch ) PTSTR Buffer, 
		__in ULONG BufferCch,
        __out_opt ULONG* SizeNeeded
		)
    {
        ULONG Count = GetEnvironmentVariable(
			Name, 
			Buffer, 
			BufferCch );
        if ( SizeNeeded )
        {
            *SizeNeeded = Count;
        }
        return Count > 0 && Count < BufferCch;
    }
}

#endif // cl 1400
