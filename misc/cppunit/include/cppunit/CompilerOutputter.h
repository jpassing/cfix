#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		Stub. Outputting is done by cfix itself.
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

#include <cppunit/Outputter.h>
#include <cppunit/SourceLine.h>
#include <cppunit/portability/Stream.h>

CPPUNIT_NS_BEGIN

class Exception;
class SourceLine;
class Test;
class TestFailure;
class TestResultCollector;

class CPPUNIT_API CompilerOutputter : public Outputter
{
public:
	CompilerOutputter()
	{}

	CompilerOutputter( 
		TestResultCollector*,
		 OStream&,
		 const std::string 
		 )
	{
		throw cfixcu::NotImplementedException();
	}

	/// Destructor.
	virtual ~CompilerOutputter()
	{}

	void setLocationFormat( 
		const std::string& 
		)
	{
	}

	static CompilerOutputter *defaultOutputter( 
		TestResultCollector*,
		OStream& 
		)
	{
		throw cfixcu::NotImplementedException();
	}

	void write()
	{}

	void setNoWrap()
	{}

	void setWrapColumn( int )
	{}

	int wrapColumn() const
	{}

	virtual void printSuccess()
	{}

	virtual void printFailureReport()
	{}
	virtual void printFailuresList()
	{}

	virtual void printStatistics()
	{}

	virtual void printFailureDetail( TestFailure* )
	{}

	virtual void printFailureLocation( SourceLine )
	{}

	virtual void printFailureType( TestFailure* )
	{}

	virtual void printFailedTestName( TestFailure* )
	{}

	virtual void printFailureMessage( TestFailure* )
	{}
};


CPPUNIT_NS_END

