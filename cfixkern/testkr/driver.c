/*----------------------------------------------------------------------
 *	Purpose:
 *		Test of driver loading & unloading.
 *
 *	Copyright:
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

#include <cfix.h>
#include <cfixkrio.h>
#include <stdlib.h>
#include "util.h"

/*----------------------------------------------------------------------
	TO TEST:
		bad exports --> STATUS_DRIVER_UNABLE_TO_LOAD

*/

static LoadTestDriverWithCfixkrBeingUnloaded()
{
	CFIX_ASSERT_EQUALS_DWORD( ERROR_FILE_NOT_FOUND, LoadDriver( L"testklib1", L"testklib1.sys" ) );
}

static void LoadUnloadCfixkr()
{
	LoadReflector();
	UnloadDriver( L"cfixkr" );
}

static LoadUnloadTestDriver()
{
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadDriver( L"testklib1", L"testklib1.sys" ) );

	UnloadDriver( L"testklib1" );
	UnloadDriver( L"cfixkr" );
}

static LoadUnloadTestDriverReverseOrder()
{
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadDriver( L"testklib1", L"testklib1.sys" ) );

	UnloadDriver( L"cfixkr" );
	UnloadDriver( L"testklib1" );
}



CFIX_BEGIN_FIXTURE( DriverLoading )
	CFIX_FIXTURE_ENTRY( LoadTestDriverWithCfixkrBeingUnloaded )
	CFIX_FIXTURE_ENTRY( LoadUnloadCfixkr )
	CFIX_FIXTURE_ENTRY( LoadUnloadTestDriver )
	CFIX_FIXTURE_ENTRY( LoadUnloadTestDriverReverseOrder )
CFIX_END_FIXTURE()

