#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		Platform compatibility - mostly does not apply as cfix is 
 *		Windows-only.
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

#include <sstream>
#include <fstream>

CPPUNIT_NS_BEGIN
typedef std::ostringstream OStringStream;      // The standard C++ way
typedef std::ofstream OFileStream;

typedef std::ostream OStream;
CPPUNIT_NS_END

