#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		Macros for fixture definition.
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

#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/cfixcompat.h>

#define CPPUNIT_TEST_SUITE( FixtureType )								\
	private:															\
		typedef FixtureType __FixtureType;								\
																		\
	public:																\
		static PCFIX_TEST_PE_DEFINITION GetDefinition()					\
		{																\
			static cfixcu::CppUnitFixture< FixtureType > Definition		\

#define CPPUNIT_TEST_SUITE_END											\
			return Definition.GetDefinition();							\
		}																\
		typedef int __DummyTypedefToAllowSemicolon
	

#define CPPUNIT_TEST( TestMethod )										\
	Definition.AddEntry(												\
		cfixcu::InvokeMethod< __FixtureType, &TestMethod  >,			\
		__CFIX_WIDE( #TestMethod ) )

#define CPPUNIT_TEST_EXCEPTION( testMethod, ExceptionType )				\
	Definition.AddEntry(												\
		cfixcc::ExpectExceptionTestRoutineWrapper<						\
			ExceptionType,												\
			cfixcu::InvokeMethod< __FixtureType, &testMethod  > >,		\
			__CFIX_WIDE( #testMethod ) )

//#define CPPUNIT_TEST_FAIL( testMethod )								
#define CPPUNIT_TEST_FAIL												\
#error CPPUNIT_TEST_FAIL not supported

#define CPPUNIT_TEST_SUITE_REGISTRATION( FixtureType )					\
EXTERN_C __declspec(dllexport)											\
PCFIX_TEST_PE_DEFINITION CFIXCALLTYPE __CfixFixturePe##FixtureType()	\
{																		\
	return FixtureType::GetDefinition();								\
}																		\
typedef int __DummyTypedef##FixtureType##ToAllowSemicolon				


//
// Named registrations not supported, fall back to default naming.
//
#define CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( FixtureType, Name )		\
	CPPUNIT_TEST_SUITE_REGISTRATION( FixtureType

#define CPPUNIT_TEST_SUITE_ADD_TEST										\
#error CPPUNIT_TEST_SUITE_ADD_TEST not supported

#define CPPUNIT_TEST_SUB_SUITE \
#error Subsuites not supported

#define CPPUNIT_TEST_SUITE_END_ABSTRACT()	\
#error Abstract suites not supported

#define CPPUNIT_TEST_SUITE_ADD_CUSTOM_TESTS \
#error Custom tests not supported

#define CPPUNIT_TEST_SUITE_PROPERTY \
#error Properties not supported

#define CPPUNIT_REGISTRY_ADD \
#error Explicit registration not supported

#define CPPUNIT_REGISTRY_ADD_TO_DEFAULT \
#error Explicit registration not supported


#if CPPUNIT_ENABLE_CU_TEST_MACROS

#define CU_TEST_SUITE(tc) CPPUNIT_TEST_SUITE(tc)
#define CU_TEST_SUB_SUITE(tc,sc) CPPUNIT_TEST_SUB_SUITE(tc,sc)
#define CU_TEST(tm) CPPUNIT_TEST(tm)
#define CU_TEST_SUITE_END() CPPUNIT_TEST_SUITE_END()
#define CU_TEST_SUITE_REGISTRATION(tc) CPPUNIT_TEST_SUITE_REGISTRATION(tc)

#endif
