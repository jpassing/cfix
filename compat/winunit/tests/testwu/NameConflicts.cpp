/*----------------------------------------------------------------------
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * N.B. This file does not include any source code from WinUnit.
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

//
// N.B. Building this test must not lead to multiply defined symbols-
// errors.
//

FIXTURE( Foo );

SETUP( Foo )
{
}

TEARDOWN( Foo )
{
}

BEGIN_TESTF( Test, Foo )
{
}
END_TESTF

FIXTURE( Bar );

SETUP( Bar )
{
}

TEARDOWN( Bar )
{
}

BEGIN_TESTF( Test, Bar)
{
}
END_TESTF

FIXTURE( Quux );

SETUP( Quux )
{
}

TEARDOWN( Quux )
{
}

BEGIN_TESTF( Test, Quux )
{
}
END_TESTF