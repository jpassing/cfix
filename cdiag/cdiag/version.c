/*----------------------------------------------------------------------
 * Purpose:
 *		Module version determination.
 *
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

#define CDIAGAPI

#include "cdiagp.h"

CDIAGAPI HRESULT CDIAGCALLTYPE CdiagGetModuleVersion(
	__in PCWSTR ModulePath,
	__out PCDIAG_MODULE_VERSION Version 
	)
{
	DWORD Handle;
	DWORD InfoLen;
	UINT BufLen;
	VS_FIXEDFILEINFO *FileInfo;
	PUCHAR RawData;
	HRESULT Hr = E_UNEXPECTED;
	
	if ( ! ModulePath || ! Version ) 
	{	
		return E_INVALIDARG;
	}

	if ( INVALID_FILE_ATTRIBUTES == GetFileAttributes( ModulePath ) )
	{
		return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
	}

	InfoLen = GetFileVersionInfoSize( ModulePath, &Handle );
	if ( ! InfoLen ) {
		DWORD Err = GetLastError();
		return ( ERROR_RESOURCE_TYPE_NOT_FOUND == Err )
			? CDIAG_E_NO_VERSION_INFO
			: HRESULT_FROM_WIN32( Err );
	}

	RawData = ( PUCHAR ) CdiagpMalloc( InfoLen, FALSE );
	if ( ! RawData )
	{
		return E_OUTOFMEMORY;
	}

	if ( ! GetFileVersionInfo( ModulePath, Handle, InfoLen, RawData ) ) {
		DWORD dwErr = GetLastError();
		Hr = HRESULT_FROM_WIN32( dwErr );
		goto Cleanup;
	}

	if ( ! VerQueryValue( 
		( PVOID ) RawData, 
		L"\\", 
		( PVOID* ) &FileInfo, 
		&BufLen ) ) 
	{
		Hr = CDIAG_E_NO_VERSION_INFO;
		goto Cleanup;
	}

	Version->Major		= HIWORD( FileInfo->dwFileVersionMS );
	Version->Minor		= LOWORD( FileInfo->dwFileVersionMS );
	Version->Revision	= HIWORD( FileInfo->dwFileVersionLS );
	Version->Build		= LOWORD( FileInfo->dwFileVersionLS );

	Hr = S_OK;

Cleanup:
	CdiagpFree( RawData );

	return Hr;
}

