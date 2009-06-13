/*----------------------------------------------------------------------
 * Purpose:
 *		Test jpurun.
 *
 * Copyright:
 *		2008-2009, Johannes Passing (passing at users.sourceforge.net)
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
#include "test.h"
#include <shlwapi.h>
#include <cfixrun.h>

#define E_FNF HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND )

UINT SearchCallbackCalls = 0;

HRESULT SearchCallback(
	__in PCWSTR Path,
	__in CFIXRUN_MODULE_TYPE Type,
	__in_opt PVOID Context,
	__in BOOL SearchPerformed
	)
{
	UNREFERENCED_PARAMETER( Path );
	UNREFERENCED_PARAMETER( Type );
	UNREFERENCED_PARAMETER( Context );
	UNREFERENCED_PARAMETER( SearchPerformed );
	SearchCallbackCalls++;
	return S_OK;
}

void TestSearchDll()
{
	//
	// Get own path.
	//
	WCHAR BasePath[ MAX_PATH ];
	WCHAR TestPath[ MAX_PATH ];
	WCHAR TestPath2[ MAX_PATH ];
	TEST( GetModuleFileName( GetModuleHandle( NULL ), BasePath, _countof( BasePath ) ) );

	TEST( PathRemoveFileSpec( BasePath ) );

	SearchCallbackCalls = 0;
	TEST( E_FNF == CfixrunSearchModules(
		L"idonotexist",
		TRUE,
		TRUE,
		SearchCallback,
		NULL ) );
	TEST( SearchCallbackCalls == 0 );

#ifdef _WIN64
	TEST_HR( StringCchPrintf( TestPath, _countof( TestPath ), L"%s\\cfix64.exe", BasePath ) );
#else
	TEST_HR( StringCchPrintf( TestPath, _countof( TestPath ), L"%s\\cfix32.exe", BasePath ) );
#endif
	SearchCallbackCalls = 0;
	TEST( E_INVALIDARG == CfixrunSearchModules(
		TestPath,
		TRUE,
		TRUE,
		SearchCallback,
		NULL ) );
	TEST( SearchCallbackCalls == 0 );


	TEST_HR( StringCchPrintf( TestPath, _countof( TestPath ), L"%s\\testlib1.dll", BasePath ) );
	SearchCallbackCalls = 0;
	TEST_HR( CfixrunSearchModules(
		TestPath,
		TRUE,
		TRUE,
		SearchCallback,
		NULL ) );
	TEST( SearchCallbackCalls == 1 );

	//
	// dir w/o DLLs.
	//
	TEST_HR( StringCchPrintf( TestPath, _countof( TestPath ), L"%s\\..\\..\\..\\cfix", BasePath ) );
	TEST( PathCanonicalize( TestPath2, TestPath ) );
	SearchCallbackCalls = 0;
	TEST_HR( CfixrunSearchModules(
		TestPath2,
		FALSE,
		TRUE,
		SearchCallback,
		NULL ) );
	TEST( SearchCallbackCalls == 0 );

	SearchCallbackCalls = 0;
	TEST_HR( CfixrunSearchModules(
		TestPath2,
		TRUE,
		FALSE,
		SearchCallback,
		NULL ) );
	TEST( SearchCallbackCalls == 0 );

	//
	// dir w/ DLLs.
	//
	SearchCallbackCalls = 0;
	TEST_HR( CfixrunSearchModules(
		BasePath,
		FALSE,
		FALSE,
		SearchCallback,
		NULL ) );
	TEST( SearchCallbackCalls >= 7 );
}

void TestRunApi()
{
	TestSearchDll();
}

CFIX_BEGIN_FIXTURE(SearchDll)
	CFIX_FIXTURE_ENTRY(TestSearchDll)
CFIX_END_FIXTURE()