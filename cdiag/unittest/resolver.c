/*----------------------------------------------------------------------
  * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

PCWSTR RegistratioEmpty[] = { 0 };
PCWSTR RegistratioInetOle[] = { L"wininet.dll", L"cdiag.dll", 0 };

PCWSTR Insertions[] = { L"one", L"two", L"three", L"four", L"five", L"six", 0 };

typedef struct _MESSAGE_SAMPLE
{
	PCWSTR *Registrations;
	DWORD MsgCode;
	DWORD Flags;
	PCWSTR *InsertionStrings;
	WCHAR ExpectedPrefix[ 100 ];
	HRESULT HrExpected;
} MESSAGE_SAMPLE, *PMESSAGE_SAMPLE;

//
// Note: This test assumes english windows!
//
static MESSAGE_SAMPLE MessageSamples[] = 
{
	// Normal
	{ RegistratioEmpty,		2,			0,															NULL,			L"The system cannot",	S_OK },
	{ RegistratioEmpty,		2,			CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						NULL,			L"The system cannot",	S_OK },
	{ RegistratioEmpty,		2,			CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS|
										CDIAG_MSGRES_FALLBACK_TO_DEFAULT,							NULL,			L"The system cannot",	S_OK },

	// Unknown msg
	{ RegistratioEmpty,		2,			CDIAG_MSGRES_NO_SYSTEM,										NULL,			L"",					CDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	0,															NULL,			L"",					CDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	CDIAG_MSGRES_NO_SYSTEM|CDIAG_MSGRES_FALLBACK_TO_DEFAULT,	NULL,			L"0x10000000",			S_OK },
	{ RegistratioEmpty,		0x10000000,	CDIAG_MSGRES_NO_SYSTEM,										NULL,			L"",					CDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						NULL,			L"",					CDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS|
										CDIAG_MSGRES_NO_SYSTEM|CDIAG_MSGRES_FALLBACK_TO_DEFAULT,	Insertions,		L"0x10000000",			S_OK },
	{ RegistratioEmpty,		0x10000000,	CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS|CDIAG_MSGRES_NO_SYSTEM,Insertions,		L"",					CDIAG_E_UNKNOWN_MESSAGE },

	// registrations
	{ RegistratioEmpty,		2,			0,															NULL,			L"The system cannot",	S_OK },	// system
	{ RegistratioInetOle,	0x90000001, 0,															NULL,			L"Microsoft-Windows-",	S_OK },	// oleres.dll
	{ RegistratioInetOle,	0x00002EE2, 0,															Insertions,		L"The operation timed",	S_OK },	// wininet.dll
	{ RegistratioInetOle,	0x10000000, 0,															Insertions,		L"",					CDIAG_E_UNKNOWN_MESSAGE},	

	// insertions
	{ RegistratioInetOle,	0x800481ff,	CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						NULL,			L"Test=%1,%2",		S_OK },
	{ RegistratioInetOle,	0x800481ff,	CDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						Insertions,		L"Test=%1,%2",		S_OK },
	{ RegistratioInetOle,	0x800481ff,	0,															Insertions,		L"Test=one,two",	S_OK },
	{ 0 }
};

static VOID TestDelete()
{
	PCDIAG_MESSAGE_RESOLVER Res = NULL;
	
	TEST_HR( CdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	if ( ! Res ) return;

	Res->Reference( Res );
	Res->Dereference( Res );
	Res->Dereference( Res );
	
	TEST_HR( CdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	if ( ! Res ) return;

	TEST_HR( Res->RegisterMessageDll( Res, L"ntdll.dll", 0, 0 ) );
	Res->Dereference( Res );

	TEST_HR( CdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	if ( ! Res ) return;
	
	TEST_HR( Res->RegisterMessageDll( Res, L"ntdll.dll", 0, 0 ) );
	TEST_HR( Res->RegisterMessageDll( Res, L"cdiag.dll", 0, 0 ) );
	Res->Dereference( Res );
}

static VOID TestRegisterUnregisterMessageDll()
{
	PCDIAG_MESSAGE_RESOLVER Res = NULL;
	WCHAR path[ MAX_PATH ];
	
	TEST_HR( CdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	if ( ! Res ) return;

	// Invalid
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, NULL, 0, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L"", 0, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L" ", 0, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L"cdiag.dll", 0xF00, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L"cdiag.dll", 0, 0xF00 ) );

	// Not found
	TEST( HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND ) == 
		Res->RegisterMessageDll( Res, L"___crap.dll", 0, 0 ) );
	TEST( HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND ) == 
		Res->RegisterMessageDll( Res, L"c:\\___crap.dll", CDIAG_MSGRES_REGISTER_EXPLICIT_PATH, 0 ) );

	// Normal
	TEST_HR( Res->RegisterMessageDll( Res, L"cdiag.dll", 0, 0 ) );
	TEST_HR( Res->UnregisterMessageDll( Res, L"cdiag.dll" ) );
	
	// Double-register
	TEST_HR( Res->RegisterMessageDll( Res, L"cdiag.dll", 0, 0 ) );
	TEST( CDIAG_E_ALREADY_REGISTERED == 
		Res->RegisterMessageDll( Res, L"cdiag.dll", 0, 0 ) );
	
	// Explicit path
	( VOID ) GetWindowsDirectory( path, _countof( path ) );
	TEST_HR( StringCchCat( path, _countof( path ), L"\\system32\\ntdll.dll" ) );
	TEST_HR( Res->RegisterMessageDll( 
		Res, path, CDIAG_MSGRES_REGISTER_EXPLICIT_PATH, 0 ) );
	TEST( CDIAG_E_ALREADY_REGISTERED == 
		Res->RegisterMessageDll( Res, L"ntdll.dll", 0, 0 ) );
	TEST_HR( Res->UnregisterMessageDll( Res, L"ntdll.dll" ) );
	

	TEST_HR( Res->UnregisterMessageDll( Res, L"cdiag.dll" ) );

	// Invalid unregister
	TEST( CDIAG_E_DLL_NOT_REGISTERED ==
		Res->UnregisterMessageDll( Res, L"cdiag.dll" ) );
	TEST( CDIAG_E_DLL_NOT_REGISTERED ==
		Res->UnregisterMessageDll( Res, L"__crap.dll" ) );

	Res->Dereference( Res );
}

static VOID TestResolveMessageInvalids()
{
	PCDIAG_MESSAGE_RESOLVER Res = NULL;
	WCHAR buf[ 1 ];
	
	TEST_HR( CdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	if ( ! Res ) return;

	// Invalids
	TEST( E_INVALIDARG == Res->ResolveMessage(
		NULL, 0, 0, NULL, _countof( buf ), buf ) );
	TEST( E_INVALIDARG == Res->ResolveMessage(
		Res, 0, 0xF00, NULL, _countof( buf ), buf ) );
	TEST( E_INVALIDARG == Res->ResolveMessage(
		Res, 0, 0, NULL, 0, buf ) );
	TEST( E_INVALIDARG == Res->ResolveMessage(
		Res, 0, 0, NULL, 0x10000, buf ) );

	Res->Dereference( Res );
}

static VOID TestResolveMessages()
{
	PCDIAG_MESSAGE_RESOLVER Res = NULL;
	PMESSAGE_SAMPLE Sample;

	for ( Sample = MessageSamples; Sample->Registrations != 0; Sample++ )
	{
		PCWSTR *Dll;
		WCHAR SmallBuf[ 1 ];
		WCHAR LargeBuf[ 512 ];
		HRESULT Hr;
		SIZE_T MsgLen[ 2 ];
		UINT FlagIndex;

		TEST_HR( CdiagCreateMessageResolver( &Res ) );
		TEST( Res );
		if ( ! Res ) return;

		for ( Dll = Sample->Registrations; *Dll != 0; Dll++ )
		{
			TEST_HR( Res->RegisterMessageDll( Res, *Dll, 0, 0 ) );
		}

		//
		// Try too small buffer.
		//
		Hr = Res->ResolveMessage(
			Res,
			Sample->MsgCode,
			Sample->Flags,
			Sample->InsertionStrings,
			_countof( SmallBuf ),
			SmallBuf );
		TEST( Hr == CDIAG_E_BUFFER_TOO_SMALL || Hr == Sample->HrExpected );

		//
		// Try real buffer with/without CDIAG_MSGRES_STRIP_NEWLINES.
		//
		for ( FlagIndex = 0; FlagIndex <= 1; FlagIndex++ )
		{
			DWORD AdditionalFlag = FlagIndex == 0
				? 0
				: CDIAG_MSGRES_STRIP_NEWLINES;

			Hr = Res->ResolveMessage(
				Res,
				Sample->MsgCode,
				Sample->Flags | AdditionalFlag,
				Sample->InsertionStrings,
				_countof( LargeBuf ),
				LargeBuf );
			TEST( Hr == Sample->HrExpected );

			//
			// Prefix-compare strings.
			//
			MsgLen[ FlagIndex ] = wcslen( Sample->ExpectedPrefix );
			LargeBuf[ MsgLen[ FlagIndex ] ] = L'\0';
			TEST( 0 == wcscmp( LargeBuf, Sample->ExpectedPrefix ) );
		}

		//
		// Removing CRLFs should not cut the size by more than 2.
		//
		TEST( abs( ( int ) MsgLen[ 0 ] - ( int ) MsgLen[ 1 ] ) <= 2 );

		for ( Dll = Sample->Registrations; *Dll != 0; Dll++ )
		{
			TEST_HR( Res->UnregisterMessageDll( Res, *Dll ) );
		}
		Res->Dereference( Res );
	}

}

CFIX_BEGIN_FIXTURE( Resolver )
	CFIX_FIXTURE_ENTRY( TestDelete )
	CFIX_FIXTURE_ENTRY( TestRegisterUnregisterMessageDll )
	CFIX_FIXTURE_ENTRY( TestResolveMessageInvalids )
	CFIX_FIXTURE_ENTRY( TestResolveMessages )
CFIX_END_FIXTURE()

