/*----------------------------------------------------------------------
 * Purpose:
 *		Search for Test-DLLs.
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
#include <shlwapi.h>
#include "cfixrunp.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

static BOOL CfixrunsIsDll(
	__in PCWSTR Path
	)
{
	size_t Len = wcslen( Path );
	return ( Len > 4 && 0 == _wcsicmp( Path + Len - 4, L".dll" ) );
}

static BOOL CfixrunsIsSys(
	__in PCWSTR Path
	)
{
	size_t Len = wcslen( Path );
	return ( Len > 4 && 0 == _wcsicmp( Path + Len - 4, L".sys" ) );
}

static HRESULT CfixrunSearchModulesInDirectory(
	__in PCWSTR PathExpression,
	__in BOOL PathIsFilter,
	__in BOOL Recursive,
	__in BOOL IncludeDrivers,
	__in CFIXRUN_VISIT_DLL_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	WIN32_FIND_DATA FindData;
	HRESULT Hr = S_OK;
	HANDLE FindHandle;
	PCWSTR Filter;
	WCHAR FilterBuffer[ MAX_PATH ];
	WCHAR PathBuffer[ MAX_PATH ];

	if ( PathIsFilter )
	{
		//
		// Path is a filter expression.
		//
		Filter = PathExpression;

		if ( wcschr( PathExpression, L'\\' ) != NULL )
		{
			PWSTR FilePart;

			//
			// Directory is included:
			//   some\dir\*.dll.
			//
			if ( 0 == GetFullPathName( 
				PathExpression, 
				_countof( PathBuffer ), 
				PathBuffer,
				&FilePart ) )
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}

			//
			// Strip file part.
			//
			ASSERT( FilePart != NULL );
			FilePart[ 0 ] = L'\0';
		}
		else
		{
			//
			// File name only:
			//   *.dll
			//
			if ( 0 == GetCurrentDirectory( _countof( PathBuffer ), PathBuffer ) )
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}
		}
	}
	else
	{
		PWSTR FilePartUnused;

		//
		// Path is a directory - make it absolute and derive a filter expression.
		//
		if ( 0 == GetFullPathName( 
			PathExpression, 
			_countof( PathBuffer ), 
			PathBuffer,
			&FilePartUnused ) )
		{
			return HRESULT_FROM_WIN32( GetLastError() );
		}

		if ( ! PathCombine( FilterBuffer, PathBuffer, L"*.*" ) )
		{
			return E_UNEXPECTED;
		}

		Filter = FilterBuffer;
	}

	ASSERT( ! PathIsRelative( PathBuffer ) );

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
			if ( ! PathCombine( FilePath, PathBuffer, FindData.cFileName ) )
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
					CfixrunSearchModulesInDirectory(
						FilePath,
						FALSE,
						Recursive,
						IncludeDrivers,
						Routine,
						Context );
				}
			}
			else if ( CfixrunsIsDll( FilePath )  )
			{
				Hr = ( Routine ) ( FilePath, CfixrunDll, Context, TRUE );
				if ( FAILED( Hr ) )
				{
					break;
				}
			}
			else if ( IncludeDrivers && CfixrunsIsSys( FilePath ) )
			{
				Hr = ( Routine ) ( FilePath, CfixrunSys, Context, TRUE );
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

HRESULT CfixrunSearchModules(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in BOOL IncludeDrivers,
	__in CFIXRUN_VISIT_DLL_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	DWORD Attr = GetFileAttributes( Path );
	ASSERT( Path );
	ASSERT( Routine );

	if ( INVALID_FILE_ATTRIBUTES == Attr )
	{
		if ( wcschr( Path, L'*' ) != NULL ||
		 wcschr( Path, L'?' ) != NULL )
		{
			//
			// Path is probably a filter expression like c:\foo\foo*.dll.
			//
			return CfixrunSearchModulesInDirectory(
				Path,
				TRUE,
				Recursive,
				IncludeDrivers,
				Routine,
				Context );
		}
		else 
		{
			return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
		}
	}
	else if ( FILE_ATTRIBUTE_DIRECTORY & Attr )
	{
		return CfixrunSearchModulesInDirectory(
			Path,
			FALSE,
			Recursive,
			IncludeDrivers,
			Routine,
			Context );
	}
	else if ( CfixrunsIsDll( Path ) )
	{
		//
		// It is a DLL - no further search required.
		//
		return ( Routine ) ( Path, CfixrunDll, Context, FALSE );
	}
	else if ( CfixrunsIsDll( Path ) ||
			  IncludeDrivers && CfixrunsIsSys( Path ) )
	{
		//
		// It is a SYS - no further search required.
		//
		return ( Routine ) ( Path, CfixrunSys, Context, FALSE );
	}
	else
	{
		return E_INVALIDARG;
	}
}