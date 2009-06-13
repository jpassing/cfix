/*----------------------------------------------------------------------
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

#include "stdafx.h"
#include "iatpatch.h"

VOID TestCdiagCreateRegistryStore();
VOID TestCdiagCreateRegistryStoreMethod();
VOID TestCdiagCreateRegistryStoreCallback();
VOID TestResolver();
VOID TestInternals();
VOID TestFormatter();
VOID TestHandlers();
VOID TestSession();
VOID TestGetVersion();

int __cdecl wmain()
{
	_CrtSetDbgFlag( 
		_CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
	//_CrtSetBreakAlloc( 111 );

	//TestResolver();
	//TestGetVersion();
	//TestHandlers();
	//TestSession();
	//TestCdiagCreateRegistryStore();
	//TestCdiagCreateRegistryStoreMethod();
	//TestCdiagCreateRegistryStoreCallback();
	//TestFormatter();
	//TestInternals();

	return 0;
}