/*----------------------------------------------------------------------
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

#include "stdafx.h"
#include <stdlib.h>

static void TestGetVersion()
{
	CDIAG_MODULE_VERSION Ver;
	HMODULE Hmod;
	WCHAR Path[ MAX_PATH ];


	ZeroMemory( &Ver, sizeof( CDIAG_MODULE_VERSION ) );

	TEST( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) ==
		CdiagGetModuleVersion( L"idonotexist", &Ver ) );

	//
	// Module w/o version.
	//
	TEST( GetModuleFileName( GetModuleHandle( L"testdiag" ), Path, _countof( Path ) ) );

	TEST( CDIAG_E_NO_VERSION_INFO == 
		CdiagGetModuleVersion( Path, &Ver ) );

	//
	// Module w/ version.
	//
	Hmod = GetModuleHandle( L"cdiag" );
	TEST( ( ( PVOID ) Hmod ) != NULL );
	TEST( GetModuleFileName( Hmod, Path, _countof( Path ) ) );

	TEST( S_OK == CdiagGetModuleVersion( Path, &Ver ) );
	TEST( Ver.Major == 1 );
}

CFIX_BEGIN_FIXTURE( Version )
	CFIX_FIXTURE_ENTRY( TestGetVersion )
CFIX_END_FIXTURE()