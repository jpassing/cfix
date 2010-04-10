/*----------------------------------------------------------------------
 * Purpose:
 *		Directory traversal.
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
#include <shlwapi.h>
#include <cfixapi.h>
#include <cfixutil.h>
#include <crtdbg.h>

#define ASSERT _ASSERTE

static HRESULT CfixutilsTraverseDirectory(
	__in PCWSTR PathExpression,
	__in BOOL PathIsFilter,
	__in BOOL Recursive,
	__in CFIXUTIL_VISIT_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	WIN32_FIND_DATA FindData;
	HRESULT Hr = S_OK;
	HRESULT Hr2 = S_OK;
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

	Hr = ( Routine ) ( PathBuffer, CfixutilEnterDirectory, Context, TRUE );
	if ( SUCCEEDED( Hr ) )
	{
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
						Hr = CfixutilsTraverseDirectory(
							FilePath,
							FALSE,
							Recursive,
							Routine,
							Context );
						if ( E_ACCESSDENIED == Hr )
						{
							//
							// Ignore and continue search.
							//
							Hr = S_OK;
						}
					}
				}
				else
				{
					Hr = ( Routine ) ( FilePath, CfixutilFile, Context, TRUE );
				}
			}
		}
		while ( SUCCEEDED( Hr ) && FindNextFile( FindHandle, &FindData ) );

		Hr2 = ( Routine ) ( PathBuffer, CfixutilLeaveDirectory, Context, TRUE );
		if ( SUCCEEDED( Hr ) && FAILED( Hr2 ) )
		{
			Hr = Hr2;
		}
	}

	FindClose( FindHandle );

	return Hr;
}

HRESULT CfixutilSearch(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in CFIXUTIL_VISIT_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	DWORD Attr;

	if ( ! Path || ! Routine )
	{
		return E_INVALIDARG;
	}

	ASSERT( Path );
	ASSERT( Routine );

	Attr = GetFileAttributes( Path );

	if ( INVALID_FILE_ATTRIBUTES == Attr )
	{
		if ( wcschr( Path, L'*' ) != NULL ||
		 wcschr( Path, L'?' ) != NULL )
		{
			//
			// Path is probably a filter expression like c:\foo\foo*.dll.
			//
			return CfixutilsTraverseDirectory(
				Path,
				TRUE,
				Recursive,
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
		return CfixutilsTraverseDirectory(
			Path,
			FALSE,
			Recursive,
			Routine,
			Context );
	}
	else 
	{
		//
		// Found - no further search required.
		//
		return ( Routine ) ( Path, CfixutilFile, Context, FALSE );
	}
}