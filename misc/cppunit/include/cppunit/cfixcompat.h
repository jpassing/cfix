#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		cfix/cppunit bridge.
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
#include <vector>
#include <cppunit/TestFixture.h>

namespace cfixcu
{
	//
	// N.B. CU setUp/tearDown methods correspond to before/after methods
	// in cfix.
	//

	/*++
		Routine Description:
			Adapter for Before-method.
	--*/
	template< class Cls >
	void __stdcall InvokeBefore()
	{
		Cls* TestObject = new Cls();
		CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, TestObject );
		TestObject->setUp();
	}

	/*++
		Routine Description:
			Adapter for After-method.
	--*/
	template< class Cls >
	void __stdcall InvokeAfter()
	{
		Cls* TestObject = static_cast< Cls* >(
			CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) );
		TestObject->tearDown();
		delete TestObject;
	}
	
	/*++
		Routine Description:
			Adapter for test methods.
	--*/
	template< class Cls, void ( Cls::*Method )() >
	void __stdcall InvokeMethod()
	{
		Cls* TestObject = static_cast< Cls* >(
			CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) );
		( TestObject->*Method )();
	}

	/*++
		Class Description:
			Adapter between CppUnit and cfix fixtures.

			Allows incremental definition of a fixture.
			Once read by GetDefinition, the definition is sealed, i.e.
			further modifications are ignored.

		Parameter:
			Cls concrete class of TestFixture
	--*/
	template< class Cls >
	class CppUnitFixture
	{
	private:
		std::vector< CFIX_PE_DEFINITION_ENTRY > Entries;
		PCFIX_TEST_PE_DEFINITION Definition;

		bool IsSealed()
		{
			return Definition != NULL;
		}

	public:
		CppUnitFixture()
			: Definition( NULL )
		{
		}

		~CppUnitFixture()
		{
			if ( this->Definition != NULL )
			{
				free( this->Definition );
			}
		}

		void AddEntry( CFIX_PE_TESTCASE_ROUTINE Routine, PCWSTR Name )
		{
			if ( ! this->IsSealed() )
			{
				CFIX_PE_DEFINITION_ENTRY Entry;
				Entry.Type		= CfixEntryTypeTestcase; 
				Entry.Name		= Name;
				Entry.Routine	= Routine;

				this->Entries.push_back( Entry );
			}
		}

		PCFIX_TEST_PE_DEFINITION GetDefinition()
		{
			if ( this->Definition != NULL )
			{
				return this->Definition;
			}

			//
			// Derive definition.
			//
			this->Definition = ( PCFIX_TEST_PE_DEFINITION ) malloc(
				sizeof( CFIX_TEST_PE_DEFINITION ) +
				sizeof( CFIX_PE_DEFINITION_ENTRY ) * ( 3 + this->Entries.size() ) );
			if ( this->Definition == NULL )
			{
				return NULL;
			}

			//
			// Base structure.
			//
			this->Definition->ApiVersion	= CFIX_PE_API_VERSION;
			this->Definition->Entries		= 
				( PCFIX_PE_DEFINITION_ENTRY )
				( ( PUCHAR ) this->Definition + sizeof( CFIX_TEST_PE_DEFINITION ) );

			//
			// Add Setup and Teardown.
			//
			this->Definition->Entries[ 0 ].Type = CfixEntryTypeBefore;
			this->Definition->Entries[ 0 ].Name = L"SetUp";
			this->Definition->Entries[ 0 ].Routine = InvokeBefore< Cls >;

			this->Definition->Entries[ 1 ].Type = CfixEntryTypeAfter;
			this->Definition->Entries[ 1 ].Name = L"TearDown";
			this->Definition->Entries[ 1 ].Routine = InvokeAfter< Cls >;

			//
			// Copy entries.
			//
			size_t Index = 2;
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

			return this->Definition;
		}
	};

	class NotImplementedException : public std::exception
	{
	public:
		NotImplementedException() : std::exception( "Not implemented" )
		{}
	};
}
