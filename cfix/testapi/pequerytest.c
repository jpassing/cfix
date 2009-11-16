/*----------------------------------------------------------------------
 * Purpose:
 *		PE query test.
 *
 * Copyright:
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
#include "test.h"

static WCHAR SameArchBinPath[ MAX_PATH ];
static WCHAR DifferentArchBinPath[ MAX_PATH ];

#ifdef _M_IX86
#define DIFFERENT_ARCH_BIN_DIR		L"..\\amd64"
#define DIFFERENT_ARCH_BIN_DIR_ALT	L"..\\..\\amd64\\fre"
#define SAME_MACHINE_TYPE			IMAGE_FILE_MACHINE_I386
#define DIFFERENT_MACHINE_TYPE		IMAGE_FILE_MACHINE_AMD64
#elif _M_X64
#define DIFFERENT_ARCH_BIN_DIR		L"..\\i386"
#define DIFFERENT_ARCH_BIN_DIR_ALT	L"..\\..\\i386\\fre"
#define SAME_MACHINE_TYPE			IMAGE_FILE_MACHINE_AMD64
#define DIFFERENT_MACHINE_TYPE		IMAGE_FILE_MACHINE_I386
#else
	#error Unsupported architecture
#endif

static void Setup()
{
	TEST( GetModuleFileName( 
		ModuleHandle, SameArchBinPath, _countof( SameArchBinPath ) ) );
	TEST( PathRemoveFileSpec( SameArchBinPath ) );

	TEST( PathCombine( 
		DifferentArchBinPath, SameArchBinPath, DIFFERENT_ARCH_BIN_DIR ) );
	if ( GetFileAttributes( DifferentArchBinPath ) == INVALID_FILE_ATTRIBUTES )
	{
		TEST( PathCombine( 
			DifferentArchBinPath, SameArchBinPath, DIFFERENT_ARCH_BIN_DIR_ALT ) );
	}
}

static void QueryWrongFileType()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

	TEST( PathCombine( Path, SameArchBinPath, L"cfix.pdb" ) );

	Info.SizeOfStruct = 0;
	TEST( E_INVALIDARG == CfixQueryPeImage( Path, &Info ) );

	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) == 
		CfixQueryPeImage( L"idonotexist", &Info ) );

	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT ) == CfixQueryPeImage(
		Path, &Info ) );
}

static void QueryNonCfixDllSameArch()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

	TEST( PathCombine( Path, SameArchBinPath, L"cfix.dll" ) );
	
	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( S_OK == CfixQueryPeImage( Path, &Info ) );

	TEST( Info.MachineType == SAME_MACHINE_TYPE );
	TEST( Info.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI );
	TEST( Info.FixtureExportsPresent == FALSE );
	TEST( Info.ModuleType == CfixModuleDll );
}

static void QueryCfixDllSameArch()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

	TEST( PathCombine( Path, SameArchBinPath, L"testapi.dll" ) );
	
	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( S_OK == CfixQueryPeImage( Path, &Info ) );

	TEST( Info.MachineType == SAME_MACHINE_TYPE );
	TEST( Info.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI );
	TEST( Info.FixtureExportsPresent == TRUE );
	TEST( Info.ModuleType == CfixModuleDll );
}

static void QueryNonCfixDllDifferentArch()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

	TEST( PathCombine( Path, DifferentArchBinPath, L"cfix.dll" ) );
	
	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( S_OK == CfixQueryPeImage( Path, &Info ) );

	TEST( Info.MachineType == DIFFERENT_MACHINE_TYPE );
	TEST( Info.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI );
	TEST( Info.FixtureExportsPresent == FALSE );
	TEST( Info.ModuleType == CfixModuleDll );
}

static void QueryCfixDllDifferentArch()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

	TEST( PathCombine( Path, DifferentArchBinPath, L"testapi.dll" ) );
	
	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( S_OK == CfixQueryPeImage( Path, &Info ) );

	TEST( Info.MachineType == DIFFERENT_MACHINE_TYPE );
	TEST( Info.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI );
	TEST( Info.FixtureExportsPresent == TRUE );
	TEST( Info.ModuleType == CfixModuleDll );
}

static void QueryCfixDriverDifferentArch()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

	TEST( PathCombine( Path, DifferentArchBinPath, L"testklib5.sys" ) );
	
	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( S_OK == CfixQueryPeImage( Path, &Info ) );

	TEST( Info.MachineType == DIFFERENT_MACHINE_TYPE );
	TEST( Info.Subsystem == IMAGE_SUBSYSTEM_NATIVE );
	TEST( Info.FixtureExportsPresent == TRUE );
	TEST( Info.ModuleType == CfixModuleDriver );
}

static void QueryCfixExe()
{
	CFIX_MODULE_INFO Info;
	WCHAR Path[ MAX_PATH ];

#ifdef _WIN64
	TEST( PathCombine( Path, DifferentArchBinPath, L"cfix32.exe" ) );
#else
	TEST( PathCombine( Path, SameArchBinPath, L"cfix32.exe" ) );
#endif

	Info.SizeOfStruct = sizeof( CFIX_MODULE_INFO );
	TEST( S_OK == CfixQueryPeImage( Path, &Info ) );

	TEST( Info.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI );
	TEST( Info.FixtureExportsPresent == FALSE );
	TEST( Info.ModuleType == CfixModuleExe );
}

CFIX_BEGIN_FIXTURE( PeQuery )
	CFIX_FIXTURE_SETUP( Setup )

	CFIX_FIXTURE_ENTRY( QueryWrongFileType )
	CFIX_FIXTURE_ENTRY( QueryNonCfixDllSameArch )
	CFIX_FIXTURE_ENTRY( QueryNonCfixDllDifferentArch )
	CFIX_FIXTURE_ENTRY( QueryCfixDllSameArch )
	CFIX_FIXTURE_ENTRY( QueryCfixDllDifferentArch )
	CFIX_FIXTURE_ENTRY( QueryCfixDriverDifferentArch )
	CFIX_FIXTURE_ENTRY( QueryCfixExe )
CFIX_END_FIXTURE()