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
#include "iatpatch.h"
#include "regvirt.h"

//
// Disable ''type cast' : from function pointer...'
//
#pragma warning( push )
#pragma warning( disable: 4054 )

static WCHAR RedirectPathHklm[ 255 ];
static WCHAR RedirectPathHkcu[ 255 ];

static LONG VirtRegCreateKeyEx (
    __in HKEY hKey,
    __in LPCWSTR lpSubKey,
    __reserved DWORD Reserved,
    __in_opt LPWSTR lpClass,
    __in DWORD dwOptions,
    __in REGSAM samDesired,
    __in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    __out PHKEY phkResult,
    __out_opt LPDWORD lpdwDisposition
    )
{
	//
	// Create virtualized key if neccessary
	//
	HKEY virtKey = NULL;
	BOOL closeKey = FALSE;
	LONG res = 0;

	if ( hKey == HKEY_LOCAL_MACHINE || hKey == HKEY_CURRENT_USER )
	{
		res = RegCreateKeyEx(
			HKEY_CURRENT_USER,
			( hKey == HKEY_LOCAL_MACHINE 
				? RedirectPathHklm
				: RedirectPathHkcu ),
			0,
			NULL,
			0,
			KEY_ALL_ACCESS,
			NULL,
			&virtKey,
			NULL );
		if ( NOERROR != res ) 
			return res;

		closeKey = TRUE;
	}
	else
	{
		virtKey = hKey;
	}

	res = RegCreateKeyEx(
		virtKey,
		lpSubKey,
		Reserved,
		lpClass,
		dwOptions,
		samDesired,
		lpSecurityAttributes,
		phkResult,
		lpdwDisposition );

	if ( closeKey )
		RegCloseKey( virtKey );

	return res;
}

static LONG VirtRegOpenKeyEx(
	HKEY hKey,
	LPCTSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult
	)
{
	//
	// Create virtualized key if neccessary
	//
	HKEY virtKey = NULL;
	BOOL closeKey = FALSE;
	LONG res = 0;

	if ( hKey == HKEY_LOCAL_MACHINE || hKey == HKEY_CURRENT_USER )
	{
		res = RegCreateKeyEx(
			HKEY_CURRENT_USER,
			( hKey == HKEY_LOCAL_MACHINE 
				? RedirectPathHklm
				: RedirectPathHkcu ),
			0,
			NULL,
			0,
			KEY_ALL_ACCESS,
			NULL,
			&virtKey,
			NULL );
		if ( NOERROR != res ) 
			return res;

		closeKey = TRUE;
	}
	else
	{
		virtKey = hKey;
	}

	res = RegOpenKeyEx(
		hKey,
		lpSubKey,
		ulOptions,
		samDesired,
		phkResult );

	if ( closeKey )
		RegCloseKey( virtKey );

	return res;
}

HRESULT EnableRegistryRedirection(
	__in PCWSTR TempPath )
{
	HRESULT hr = StringCchPrintf(
		RedirectPathHklm, 
		_countof( RedirectPathHklm ),
		L"%s\\hklm",
		TempPath );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	hr = StringCchPrintf(
		RedirectPathHkcu, 
		_countof( RedirectPathHkcu ),
		L"%s\\hkcu",
		TempPath );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	hr = PatchIat(
		GetModuleHandle( L"cdiag" ),
		"advapi32.dll",
		"RegCreateKeyExW",
	 	( PVOID ) VirtRegCreateKeyEx,
		NULL );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	hr = PatchIat(
		GetModuleHandle( L"cdiag" ),
		"advapi32.dll",
		"RegOpenKeyExW",
	 	( PVOID ) VirtRegOpenKeyEx,
		NULL );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	return S_OK;
}

/*++
	Routine description:
		Restores normal registry access
--*/
HRESULT DisableRegistryRedirection()
{
	HRESULT hr = PatchIat(
		GetModuleHandle( L"cdiag" ),
		"advapi32.dll",
		"RegCreateKeyExW",
	 	( PVOID ) RegCreateKeyExW,
		NULL );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	hr = PatchIat(
		GetModuleHandle( L"cdiag" ),
		"advapi32.dll",
		"RegOpenKeyExW",
	 	( PVOID ) RegOpenKeyExW,
		NULL );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	return S_OK;
}
#pragma warning( pop )
