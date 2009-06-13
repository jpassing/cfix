#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		Assertions.
 *
 *		Adapted from CppUnit 1.12.
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

CPPUNIT_NS_BEGIN

#define CPPUNIT_ASSERT( Expression )									\
	CFIXCC_ASSERT( Expression )

#define CPPUNIT_ASSERT_MESSAGE( Message, Expression )					\
	CFIXCC_ASSERT_MESSAGE( Message, Expression )

#define CPPUNIT_FAIL( Message )											\
	CPPUNIT_ASSERT( ! Message )

#define CPPUNIT_ASSERT_EQUAL( Expected, Actual )						\
	CFIXCC_ASSERT_EQUALS( Expected, Actual )

#define CPPUNIT_ASSERT_EQUAL_MESSAGE( Message, Expected, Actual )		\
	CFIXCC_ASSERT_EQUALS_MESSAGE( Expected, Actual, Message )

//#define CPPUNIT_ASSERT_DOUBLES_EQUAL(expected,actual,delta)        \
//
//#define CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message,expected,actual,delta)  \
//
//#define CPPUNIT_ASSERT_THROW( expression, ExceptionType )              \
//   CPPUNIT_ASSERT_THROW_MESSAGE( CPPUNIT_NS::AdditionalMessage(),       \
//                                 expression,                            \
//                                 ExceptionType )
//
//
//// implementation detail
//#if CPPUNIT_USE_TYPEINFO_NAME
//#define CPPUNIT_EXTRACT_EXCEPTION_TYPE_( exception, no_rtti_message ) \
//   CPPUNIT_NS::TypeInfoHelper::getClassName( typeid(exception) )
//#else
//#define CPPUNIT_EXTRACT_EXCEPTION_TYPE_( exception, no_rtti_message ) \
//   std::string( no_rtti_message )
//#endif // CPPUNIT_USE_TYPEINFO_NAME
//
//// implementation detail
//#define CPPUNIT_GET_PARAMETER_STRING( parameter ) #parameter
//
///** Asserts that the given expression throws an exception of the specified type, 
// * setting a user supplied message in case of failure. 
// * \ingroup Assertions
// * Example of usage:
// * \code
// *   std::vector<int> v;
// *  CPPUNIT_ASSERT_THROW_MESSAGE( "- std::vector<int> v;", v.at( 50 ), std::out_of_range );
// * \endcode
// */
//# define CPPUNIT_ASSERT_THROW_MESSAGE( message, expression, ExceptionType )   \
//   do {                                                                       \
//      bool cpputCorrectExceptionThrown_ = false;                              \
//      CPPUNIT_NS::Message cpputMsg_( "expected exception not thrown" );       \
//      cpputMsg_.addDetail( message );                                         \
//      cpputMsg_.addDetail( "Expected: "                                       \
//                           CPPUNIT_GET_PARAMETER_STRING( ExceptionType ) );   \
//                                                                              \
//      try {                                                                   \
//         expression;                                                          \
//      } catch ( const ExceptionType & ) {                                     \
//         cpputCorrectExceptionThrown_ = true;                                 \
//      } catch ( const std::exception &e) {                                    \
//         cpputMsg_.addDetail( "Actual  : " +                                  \
//                              CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e,             \
//                                          "std::exception or derived") );     \
//         cpputMsg_.addDetail( std::string("What()  : ") + e.what() );         \
//      } catch ( ... ) {                                                       \
//         cpputMsg_.addDetail( "Actual  : unknown.");                          \
//      }                                                                       \
//                                                                              \
//      if ( cpputCorrectExceptionThrown_ )                                     \
//         break;                                                               \
//                                                                              \
//      CPPUNIT_NS::Asserter::fail( cpputMsg_,                                  \
//                                  CPPUNIT_SOURCELINE() );                     \
//   } while ( false )
//
//
///** Asserts that the given expression does not throw any exceptions.
// * \ingroup Assertions
// * Example of usage:
// * \code
// *   std::vector<int> v;
// *   v.push_back( 10 );
// *  CPPUNIT_ASSERT_NO_THROW( v.at( 0 ) );
// * \endcode
// */
//# define CPPUNIT_ASSERT_NO_THROW( expression )                             \
//   CPPUNIT_ASSERT_NO_THROW_MESSAGE( CPPUNIT_NS::AdditionalMessage(),       \
//                                    expression )
//
//
///** Asserts that the given expression does not throw any exceptions, 
// * setting a user supplied message in case of failure. 
// * \ingroup Assertions
// * Example of usage:
// * \code
// *   std::vector<int> v;
// *   v.push_back( 10 );
// *  CPPUNIT_ASSERT_NO_THROW( "std::vector<int> v;", v.at( 0 ) );
// * \endcode
// */
//# define CPPUNIT_ASSERT_NO_THROW_MESSAGE( message, expression )               \
//   do {                                                                       \
//      CPPUNIT_NS::Message cpputMsg_( "unexpected exception caught" );         \
//      cpputMsg_.addDetail( message );                                         \
//                                                                              \
//      try {                                                                   \
//         expression;                                                          \
//      } catch ( const std::exception &e ) {                                   \
//         cpputMsg_.addDetail( "Caught: " +                                    \
//                              CPPUNIT_EXTRACT_EXCEPTION_TYPE_( e,             \
//                                          "std::exception or derived" ) );    \
//         cpputMsg_.addDetail( std::string("What(): ") + e.what() );           \
//         CPPUNIT_NS::Asserter::fail( cpputMsg_,                               \
//                                     CPPUNIT_SOURCELINE() );                  \
//      } catch ( ... ) {                                                       \
//         cpputMsg_.addDetail( "Caught: unknown." );                           \
//         CPPUNIT_NS::Asserter::fail( cpputMsg_,                               \
//                                     CPPUNIT_SOURCELINE() );                  \
//      }                                                                       \
//   } while ( false )
//
//
///** Asserts that an assertion fail.
// * \ingroup Assertions
// * Use to test assertions.
// * Example of usage:
// * \code
// *   CPPUNIT_ASSERT_ASSERTION_FAIL( CPPUNIT_ASSERT( 1 == 2 ) );
// * \endcode
// */
//# define CPPUNIT_ASSERT_ASSERTION_FAIL( assertion )                 \
//   CPPUNIT_ASSERT_THROW( assertion, CPPUNIT_NS::Exception )
//
//
///** Asserts that an assertion fail, with a user-supplied message in 
// * case of error.
// * \ingroup Assertions
// * Use to test assertions.
// * Example of usage:
// * \code
// *   CPPUNIT_ASSERT_ASSERTION_FAIL_MESSAGE( "1 == 2", CPPUNIT_ASSERT( 1 == 2 ) );
// * \endcode
// */
//# define CPPUNIT_ASSERT_ASSERTION_FAIL_MESSAGE( message, assertion )    \
//   CPPUNIT_ASSERT_THROW_MESSAGE( message, assertion, CPPUNIT_NS::Exception )
//
//
///** Asserts that an assertion pass.
// * \ingroup Assertions
// * Use to test assertions.
// * Example of usage:
// * \code
// *   CPPUNIT_ASSERT_ASSERTION_PASS( CPPUNIT_ASSERT( 1 == 1 ) );
// * \endcode
// */
//# define CPPUNIT_ASSERT_ASSERTION_PASS( assertion )                 \
//   CPPUNIT_ASSERT_NO_THROW( assertion )
//
//
///** Asserts that an assertion pass, with a user-supplied message in 
// * case of failure. 
// * \ingroup Assertions
// * Use to test assertions.
// * Example of usage:
// * \code
// *   CPPUNIT_ASSERT_ASSERTION_PASS_MESSAGE( "1 != 1", CPPUNIT_ASSERT( 1 == 1 ) );
// * \endcode
// */
//# define CPPUNIT_ASSERT_ASSERTION_PASS_MESSAGE( message, assertion )    \
//   CPPUNIT_ASSERT_NO_THROW_MESSAGE( message, assertion )
//
//
//
//
//// Backwards compatibility
//
//#if CPPUNIT_ENABLE_NAKED_ASSERT
//
//#undef assert
//#define assert(c)                 CPPUNIT_ASSERT(c)
//#define assertEqual(e,a)          CPPUNIT_ASSERT_EQUAL(e,a)
//#define assertDoublesEqual(e,a,d) CPPUNIT_ASSERT_DOUBLES_EQUAL(e,a,d)
//#define assertLongsEqual(e,a)     CPPUNIT_ASSERT_EQUAL(e,a)
//
//#endif


CPPUNIT_NS_END

