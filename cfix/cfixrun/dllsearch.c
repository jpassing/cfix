/*----------------------------------------------------------------------
 * Purpose:
 *		Search for Test-DLLs.
 *
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
#include <shlwapi.h>
#include "internal.h"

static BOOL CfixrunsIsDll(
	__in PCWSTR Path
	)
{
	size_t Len = wcslen( Path );
	return ( Len > 4 && 0 == _wcsicmp( Path + Len - 4, L".dll" ) );
}

static HRESULT CfixrunsSearchDllsInDirectory(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in CFIXRUN_VISIT_DLL_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	WIN32_FIND_DATA FindData;
	HRESULT Hr = S_OK;
	HANDLE FindHandle;
	WCHAR Filter[ MAX_PATH ];

	if ( ! PathCombine( Filter, Path, L"*.*" ) )
	{
		return E_UNEXPECTED;
	}

	FindHandle = FindFirstFile( Filter, &FindData );
	if ( INVALID_HANDLE_VALUE == FindHandle )
	{
		DWORD Err = GetLastError();
		if ( ERROR_NO_MORE_FILES == Err )
		{
			return S_OK;
		}
		else
		{
			return HRESULT_FROM_WIN32( Err );
		}
	}

	do
	{
		if ( 0 == wcscmp( FindData.cFileName, L"." ) ||
			 0 == wcscmp( FindData.cFileName, L".." ) )
		{
		}
		else 
		{
			//
			// Derive path from filename.
			//
			WCHAR FilePath[ MAX_PATH ];
			if ( ! PathCombine( FilePath, Path, FindData.cFileName ) )
			{
				Hr = E_UNEXPECTED;
				break;
			}
			if ( FILE_ATTRIBUTE_REPARSE_POINT & FindData.dwFileAttributes )
			{
				//
				// Junction/SymLink - skip.
				//
			}
			else if ( FILE_ATTRIBUTE_DIRECTORY & FindData.dwFileAttributes )
			{
				if ( Recursive )
				{
					CfixrunsSearchDllsInDirectory(
						FilePath,
						Recursive,
						Routine,
						Context );
				}
			}
			else if ( CfixrunsIsDll( FilePath ) )
			{
				Hr = ( Routine ) ( FilePath, Context, TRUE );
				if ( FAILED( Hr ) )
				{
					break;
				}
			}
		}
	}
	while ( FindNextFile( FindHandle, &FindData ) );

	FindClose( FindHandle );

	return Hr;
}

HRESULT CfixrunSearchDlls(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in CFIXRUN_VISIT_DLL_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	DWORD Attr = GetFileAttributes( Path );
	ASSERT( Path );
	ASSERT( Routine );

	if ( INVALID_FILE_ATTRIBUTES == Attr )
	{
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}
	else if ( FILE_ATTRIBUTE_DIRECTORY & Attr )
	{
		return CfixrunsSearchDllsInDirectory(
			Path,
			Recursive,
			Routine,
			Context );
	}
	else if ( CfixrunsIsDll( Path ) )
	{
		//
		// It is a DLL - no further search required.
		//
		return ( Routine ) ( Path, Context, FALSE );
	}
	else
	{
		return E_INVALIDARG;
	}
}