/*----------------------------------------------------------------------
 * Purpose:
 *		IAT patching routines
 *
 * Copyright:
 *		2007-2009 Johannes Passing (passing at users.sourceforge.net)
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

#define PtrFromRva( base, rva ) ( ( ( PBYTE ) base ) + rva )

/*++
	Routine Description:
		Replace the function pointer in a module's IAT.

	Parameters:
		Module				- Module to use IAT from.
		ImportedModuleName	- Name of imported DLL from which 
							  function is imported.
		ImportedProcName	- Name of imported function.
		AlternateProc		- Function to be written to IAT.
		OldProc				- Original function.

	Return Value:
		S_OK on success.
		(any HRESULT) on failure.
--*/
HRESULT PatchIat(
	__in HMODULE Module,
	__in PSTR ImportedModuleName,
	__in PSTR ImportedProcName,
	__in PVOID AlternateProc,
	__out_opt PVOID *OldProc
	)
{
	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER ) Module;
	PIMAGE_NT_HEADERS NtHeader; 
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	UINT Index;

	_ASSERTE( Module );
	_ASSERTE( ImportedModuleName );
	_ASSERTE( ImportedProcName );
	_ASSERTE( AlternateProc );

	NtHeader = ( PIMAGE_NT_HEADERS ) 
		PtrFromRva( DosHeader, DosHeader->e_lfanew );
	if( IMAGE_NT_SIGNATURE != NtHeader->Signature )
	{
		return HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
	}

	ImportDescriptor = ( PIMAGE_IMPORT_DESCRIPTOR ) PtrFromRva( DosHeader, 
		NtHeader->OptionalHeader.DataDirectory
			[ IMAGE_DIRECTORY_ENTRY_IMPORT ].VirtualAddress );

	//
	// Iterate over import descriptors/DLLs.
	//
	for ( Index = 0; ImportDescriptor[ Index ].Characteristics != 0; Index++ )
	{
		PSTR dllName = ( PSTR ) 
			PtrFromRva( DosHeader, ImportDescriptor[ Index ].Name );

		if ( 0 == _strcmpi( dllName, ImportedModuleName ) )
		{
			//
			// This the DLL we are after.
			//
			PIMAGE_THUNK_DATA Thunk;
			PIMAGE_THUNK_DATA OrigThunk;

			if ( ! ImportDescriptor[ Index ].FirstThunk ||
				 ! ImportDescriptor[ Index ].OriginalFirstThunk )
			{
				return E_INVALIDARG;
			}

			Thunk = ( PIMAGE_THUNK_DATA )
				PtrFromRva( DosHeader, ImportDescriptor[ Index ].FirstThunk );
			OrigThunk = ( PIMAGE_THUNK_DATA )
				PtrFromRva( DosHeader, ImportDescriptor[ Index ].OriginalFirstThunk );

			for ( ; OrigThunk->u1.Function != 0; OrigThunk++, Thunk++ )
			{
				PIMAGE_IMPORT_BY_NAME import;
				if ( OrigThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG )
				{
					//
					// Ordinal import - we can handle named imports
					// ony, so skip it.
					//
					continue;
				}

				import = ( PIMAGE_IMPORT_BY_NAME )
					PtrFromRva( DosHeader, OrigThunk->u1.AddressOfData );

				if ( 0 == strcmp( ImportedProcName, ( char* ) import->Name ) )
				{
					//
					// Proc found, patch it.
					//
					DWORD junk;
					MEMORY_BASIC_INFORMATION thunkMemInfo;

					//
					// Make page writable.
					//
					VirtualQuery(
						Thunk,
						&thunkMemInfo,
						sizeof( MEMORY_BASIC_INFORMATION ) );
					if ( ! VirtualProtect(
						thunkMemInfo.BaseAddress,
						thunkMemInfo.RegionSize,
						PAGE_EXECUTE_READWRITE,
						&thunkMemInfo.Protect ) )
					{
						return HRESULT_FROM_WIN32( GetLastError() );
					}

					//
					// Replace function pointers (non-atomically).
					//
					if ( OldProc )
					{
						*OldProc = ( PVOID ) ( DWORD_PTR ) Thunk->u1.Function;
					}
#ifdef _WIN64
					Thunk->u1.Function = ( ULONGLONG ) ( DWORD_PTR ) AlternateProc;
#else
					Thunk->u1.Function = ( DWORD ) ( DWORD_PTR ) AlternateProc;
#endif
					//
					// Restore page protection.
					//
					if ( ! VirtualProtect(
						thunkMemInfo.BaseAddress,
						thunkMemInfo.RegionSize,
						thunkMemInfo.Protect,
						&junk ) )
					{
						return HRESULT_FROM_WIN32( GetLastError() );
					}

					return S_OK;
				}
			}
			
			//
			// Import not found.
			//
			return HRESULT_FROM_WIN32( ERROR_PROC_NOT_FOUND );		
		}
	}

	//
	// DLL not found.
	//
	return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
}