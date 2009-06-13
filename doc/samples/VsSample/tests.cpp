/*----------------------------------------------------------------------
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */
 
//
// Always include cfix.h!
//
#include "stdafx.h"
#include <cfix.h>

/*++
	Routine Description:
		Setup Routine -- will be called at the very beginning of the
		testrun. If the setup routine fails, no testcase of this 
		fixture will be executed.

		If your default calling convention is cdecl (this is the 
		case for Visual Studio projects), be sure to add __stdcall 
		to all routine decarations -- otheriwse you will get the 
		following warning:
		   cannot convert from 'void (__cdecl *)(void)' 
		   to 'CFIX_PE_TESTCASE_ROUTINE'

		As the routine is referenced only by the fixture definition 
		(see below), you may declare this routine as static.
--*/
static void __stdcall FixtureSetup()
{
	CFIX_ASSERT( 0 != 1 );
}

/*++
	Routine Description:
		Teardown Routine -- will be called at the end of the
		testrun. 
--*/
static void __stdcall FixtureTeardown()
{
	CFIX_LOG( L"Tearing down..." );
}

/*++
	Routine Description:
		Test routine -- do the actual testing.
--*/
static void __stdcall Test1()
{
	DWORD a = 1;
	DWORD b = 1;
	CFIX_ASSERT_EQUALS_DWORD( a, b );
	CFIX_ASSERT( a + b == 2 );

	//
	// Log a message -- printf-style formatting may be used.
	//
	CFIX_LOG( L"a=%d, b=%d", a, b );
}

/*++
	Routine Description:
		Another test routine -- one that will trigger a failed 
		assertion.
--*/
static void __stdcall Test2()
{
	DWORD a = 17;

	//
	// This one will fail. If run in the debugger, it will break here.
	// if run outside the debugger, the failure will be logged and the
	// testcase is aborted.
	//
	CFIX_ASSERT( a == 42 );
}

/*++
	Description:
		These lines define a test fixture. It is best to put this at the
		end of the file.

		'MyFixture' is the name of the fixture. It must be unique across
		all fixtures of this DLL. The same restrictions as for naming
		routines apply, i.e. no spaces, no special characters etc.

		The order of CFIX_FIXTURE_ENTRY, CFIX_FIXTURE_SETUP and 
		CFIX_FIXTURE_TEARDOWN is irrelevant. For the current release, 
		however, (1.0), the order of FIXTURE_ENTRYs defines the 
		execution order.
--*/
CFIX_BEGIN_FIXTURE( MyFixture )
	//
	// Define Test1 and Test2 to be test routines. You can define any 
	// number of test routines.
	//
	CFIX_FIXTURE_ENTRY( Test1 )
	CFIX_FIXTURE_ENTRY( Test2 )

	//
	// Define FixtureSetup to be a setup routine. At most one
	// CFIX_FIXTURE_SETUP line may be declared per fixture.  If the
	// fixture does not require setup, omit this line.
	//
	CFIX_FIXTURE_SETUP( FixtureSetup )

	//
	// Define FixtureTeardown to be a teardown routine. As with
	// setup routines, you can declare at most one such routine
	// per fixture. If the fixture does not require teardown, 
	// omit this line.
	//
	CFIX_FIXTURE_TEARDOWN( FixtureTeardown )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( MyOtherFixture )
	//
	// You are free to reuse certain routines in another fixture if
	// it makes sense.
	//
	CFIX_FIXTURE_ENTRY( Test1 )
CFIX_END_FIXTURE()
