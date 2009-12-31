/*----------------------------------------------------------------------
 * Purpose:
 *		Test case for CdiagCreateRegistryStore
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

/*----------------------------------------------------------------------
 *
 *	Mocks
 *
 */

template< BOOL FailOnHkcu, BOOL FailOnHklm >
static LONG FailRegCreateKeyEx (
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
	BOOL failOnHklm = FailOnHklm;
	BOOL failOnHkcu = FailOnHkcu;
	if ( hKey == HKEY_LOCAL_MACHINE && failOnHklm ||
		 hKey == HKEY_CURRENT_USER && failOnHkcu )
	{
		return ERROR_ACCESS_DENIED;
	}
	else
	{
		if ( hKey == HKEY_LOCAL_MACHINE )
		{
			//
			// Use HKCU s.t. we do not need admin privs
			//
			hKey = HKEY_CURRENT_USER;
		}

		return RegCreateKeyEx(
			hKey,
			lpSubKey,
			Reserved,
			lpClass,
			dwOptions,
			samDesired,
			lpSecurityAttributes,
			phkResult,
			lpdwDisposition );
	}
}

/*----------------------------------------------------------------------
 *
 *	Parameters
 *
 */
static LPCWSTR String256 = L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234";
static LPCWSTR String257 = L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"01234567890123456789012345678901234567890123456789"
					L"012345";

static struct _PARAM_BASEKEYNAME
{
	LPCWSTR Name;
	HRESULT ExpectedHr;
} ParamBaseKeyName[] =
{
	{ L"",							E_INVALIDARG },
	{ L" \t",						E_INVALIDARG },
	//{ String257,					E_INVALIDARG },
	//{ String256,					S_OK},
	{ L"Software\\JP\\__test\\a\\b",S_OK},
	{ 0, ( HRESULT ) -1 } 
};

static struct _PARAM_ACCESSMODE
{
	DWORD Mode;
	LONG ( *RegCreateKeyExProc ) (
		__in HKEY hKey,
		__in LPCWSTR lpSubKey,
		__reserved DWORD Reserved,
		__in_opt LPWSTR lpClass,
		__in DWORD dwOptions,
		__in REGSAM samDesired,
		__in_opt LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		__out PHKEY phkResult,
		__out_opt LPDWORD lpdwDisposition
		);
	HRESULT ExpectedHr;
} ParamAccessMode[] =
{
	{ 0,								RegCreateKeyEx,						E_INVALIDARG },
	{ ( DWORD ) -1,						RegCreateKeyEx,						E_INVALIDARG },

	// User only
	{ CDIAG_CFGS_ACCESS_READ_USER,		FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_USER,		FailRegCreateKeyEx<FALSE, TRUE>,	S_OK },
	
	{ CDIAG_CFGS_ACCESS_WRITE_USER,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_WRITE_USER,	FailRegCreateKeyEx<FALSE, TRUE>,	S_OK },
	
	{ CDIAG_CFGS_ACCESS_READ_USER |
	  CDIAG_CFGS_ACCESS_WRITE_USER,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_USER |
	  CDIAG_CFGS_ACCESS_WRITE_USER,	FailRegCreateKeyEx<FALSE, TRUE>,	S_OK },

	// Machine only
	{ CDIAG_CFGS_ACCESS_READ_MACHINE,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE,	FailRegCreateKeyEx<TRUE, FALSE>,	S_OK },
	
	{ CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<TRUE, FALSE>,	S_OK },
	
	{ CDIAG_CFGS_ACCESS_READ_MACHINE |
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE |
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<TRUE, FALSE>,	S_OK },

	// Both
	{ CDIAG_CFGS_ACCESS_READ_USER |
	  CDIAG_CFGS_ACCESS_READ_MACHINE,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_USER |
	  CDIAG_CFGS_ACCESS_READ_MACHINE,	FailRegCreateKeyEx<TRUE, FALSE>,	E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_USER |
	  CDIAG_CFGS_ACCESS_READ_MACHINE,	FailRegCreateKeyEx<FALSE, TRUE>,	E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_READ_USER |
	  CDIAG_CFGS_ACCESS_READ_MACHINE,	FailRegCreateKeyEx<FALSE, FALSE>,	S_OK },

	{ CDIAG_CFGS_ACCESS_WRITE_USER |
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_WRITE_USER |
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<TRUE, FALSE>,	E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_WRITE_USER |
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<FALSE, TRUE>,	E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_WRITE_USER |
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	FailRegCreateKeyEx<FALSE, FALSE>,	S_OK },
	
	{ CDIAG_CFGS_ACCESS_ALL,			FailRegCreateKeyEx<TRUE, TRUE>,		E_ACCESSDENIED },
	{ CDIAG_CFGS_ACCESS_ALL,			FailRegCreateKeyEx<FALSE, FALSE>,	S_OK },
	{ 0, 0, ( HRESULT ) -1 } 
};

PCDIAG_CONFIGURATION_STORE Store = NULL;

static struct _PARAM_STORE
{
	PCDIAG_CONFIGURATION_STORE *Store;
	HRESULT ExpectedHr;
} ParamStore[] =
{
	{ NULL,							E_INVALIDARG },
	{ &Store,						S_OK },
	{ 0, ( HRESULT ) -1 } 
};

/*----------------------------------------------------------------------
 *
 *	Driver
 *
 */

static VOID TestCdiagCreateRegistryStore()
{
	HMODULE CdiagModule = GetModuleHandle( L"cdiag" );
	CFIX_ASSUME( CdiagModule != NULL );

	_ASSERTE( wcslen( String256 ) == 256 - 1 );
	_ASSERTE( wcslen( String257 ) == 257 - 1 );
	for ( struct _PARAM_BASEKEYNAME *keyName = ParamBaseKeyName;
		  keyName->ExpectedHr != ( HRESULT ) -1;
		  keyName++ )
	{
		for ( struct _PARAM_ACCESSMODE *mode = ParamAccessMode;
			  mode->ExpectedHr != ( HRESULT ) -1;
			  mode++ )
		{
			TEST_HR( PatchIat(
				CdiagModule,
				"advapi32.dll",
				"RegCreateKeyExW",
			 	mode->RegCreateKeyExProc,
				NULL ) );
			for ( struct _PARAM_STORE *store = ParamStore;
				  store->ExpectedHr != ( HRESULT ) -1;
				  store++ )
			{
				BOOL expectedSuccess = 
					SUCCEEDED( keyName->ExpectedHr ) &&
					SUCCEEDED( mode->ExpectedHr ) &&
					SUCCEEDED( store->ExpectedHr );
				
				HRESULT hr = CdiagCreateRegistryStore(
					keyName->Name,
					mode->Mode,
					store->Store );

				if ( expectedSuccess )
				{
					TEST_HR( hr );
					TEST( *store->Store );

					(*(store->Store))->Delete( *(store->Store) );
					*store->Store = NULL;
				}
				else
				{
					TEST( hr == keyName->ExpectedHr ||
						  hr == mode->ExpectedHr ||
						  hr == store->ExpectedHr );
				}
			}
		}
	}

	TEST_HR( PatchIat(
		CdiagModule,
		"advapi32.dll",
		"RegCreateKeyExW",
	 	RegCreateKeyExW,
		NULL ) );
}

CFIX_BEGIN_FIXTURE( CreateRegistryStore )
	CFIX_FIXTURE_ENTRY( TestCdiagCreateRegistryStore )
CFIX_END_FIXTURE()
