#pragma once

/*----------------------------------------------------------------------
 * Cfix CppUnit Compatibility.
 *
 * Purpose:
 *		Common warning disablings.
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

#ifndef CFIX_CPPU_REENABLE_WARNINGS
#pragma warning( disable: 4018 4284 4146 )
#if _MSC_VER >= 1400
	#pragma warning( disable: 4996 )
#endif
#endif