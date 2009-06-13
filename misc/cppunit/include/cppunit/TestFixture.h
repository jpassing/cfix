#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		Fixture definition.
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
#include <cppunit/Portability.h>

CPPUNIT_NS_BEGIN

class CPPUNIT_API TestFixture
{
public:
	virtual ~TestFixture() {};

	virtual void setUp() {};

	virtual void tearDown() {};
};

CPPUNIT_NS_END
