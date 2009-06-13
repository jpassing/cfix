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

PCWSTR RegistratioEmpty[] = { 0 };
PCWSTR RegistratioInetOle[] = { L"wininet.dll", L"oleres.dll", 0 };

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
	{ RegistratioEmpty,		2,			JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						NULL,			L"The system cannot",	S_OK },
	{ RegistratioEmpty,		2,			JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS|
										JPDIAG_MSGRES_FALLBACK_TO_DEFAULT,							NULL,			L"The system cannot",	S_OK },

	// Unknown msg
	{ RegistratioEmpty,		2,			JPDIAG_MSGRES_NO_SYSTEM,									NULL,			L"",					JPDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	0,															NULL,			L"",					JPDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	JPDIAG_MSGRES_NO_SYSTEM|JPDIAG_MSGRES_FALLBACK_TO_DEFAULT,	NULL,			L"0x10000000",			S_OK },
	{ RegistratioEmpty,		0x10000000,	JPDIAG_MSGRES_NO_SYSTEM,									NULL,			L"",					JPDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						NULL,			L"",					JPDIAG_E_UNKNOWN_MESSAGE },
	{ RegistratioEmpty,		0x10000000,	JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS|
										JPDIAG_MSGRES_NO_SYSTEM|JPDIAG_MSGRES_FALLBACK_TO_DEFAULT,	Insertions,		L"0x10000000",			S_OK },
	{ RegistratioEmpty,		0x10000000,	JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS|JPDIAG_MSGRES_NO_SYSTEM,Insertions,	L"",					JPDIAG_E_UNKNOWN_MESSAGE },

	// registrations
	{ RegistratioEmpty,		2,			0,															NULL,			L"The system cannot",	S_OK },	// system
	{ RegistratioInetOle,	0x90000001, 0,															NULL,			L"Microsoft-Windows-",	S_OK },	// oleres.dll
	{ RegistratioInetOle,	0x00002EE2, 0,															Insertions,		L"The operation timed",	S_OK },	// wininet.dll
	{ RegistratioInetOle,	0x10000000, 0,															Insertions,		L"",					JPDIAG_E_UNKNOWN_MESSAGE},	

	// inserions
	{ RegistratioInetOle,	0xC0002714,	JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						NULL,			L"DCOM got error \"%1\"",		S_OK },
	{ RegistratioInetOle,	0xC0002714,	JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS,						Insertions,		L"DCOM got error \"%1\"",		S_OK },
	{ RegistratioInetOle,	0xC0002714,	0,															Insertions,		L"DCOM got error \"one\"",		S_OK },
	{ RegistratioInetOle,	0xC0004724,	0,															Insertions,		L"The machine wide one two",	S_OK },
	{ 0 }
};

static VOID TestDelete()
{
	PJPDIAG_MESSAGE_RESOLVER Res = NULL;
	
	TEST_HR( JpdiagCreateMessageResolver( &Res ) );
	TEST( Res );

	Res->Reference( Res );
	Res->Dereference( Res );
	Res->Dereference( Res );
	
	TEST_HR( JpdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	TEST_HR( Res->RegisterMessageDll( Res, L"winlogon.exe", 0, 0 ) );
	Res->Dereference( Res );

	TEST_HR( JpdiagCreateMessageResolver( &Res ) );
	TEST( Res );
	TEST_HR( Res->RegisterMessageDll( Res, L"winlogon.exe", 0, 0 ) );
	TEST_HR( Res->RegisterMessageDll( Res, L"jpdiag.dll", 0, 0 ) );
	Res->Dereference( Res );
}

static VOID TestRegisterUnregisterMessageDll()
{
	PJPDIAG_MESSAGE_RESOLVER Res = NULL;
	WCHAR path[ MAX_PATH ];
	
	TEST_HR( JpdiagCreateMessageResolver( &Res ) );
	TEST( Res );

	// Invalid
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, NULL, 0, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L"", 0, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L" ", 0, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L"jpdiag.dll", 0xF00, 0 ) );
	TEST( E_INVALIDARG == Res->RegisterMessageDll( Res, L"jpdiag.dll", 0, 0xF00 ) );

	// Not found
	TEST( HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND ) == 
		Res->RegisterMessageDll( Res, L"___crap.dll", 0, 0 ) );
	TEST( HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND ) == 
		Res->RegisterMessageDll( Res, L"c:\\___crap.dll", JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH, 0 ) );

	// Normal
	TEST_HR( Res->RegisterMessageDll( Res, L"jpdiag.dll", 0, 0 ) );
	TEST_HR( Res->UnregisterMessageDll( Res, L"jpdiag.dll" ) );
	
	// Double-register
	TEST_HR( Res->RegisterMessageDll( Res, L"jpdiag.dll", 0, 0 ) );
	TEST( JPDIAG_E_ALREADY_REGISTERED == 
		Res->RegisterMessageDll( Res, L"jpdiag.dll", 0, 0 ) );
	
	// Explicit path
	( VOID ) GetWindowsDirectory( path, _countof( path ) );
	TEST_HR( StringCchCat( path, _countof( path ), L"\\system32\\winlogon.exe" ) );
	TEST_HR( Res->RegisterMessageDll( 
		Res, path, JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH, 0 ) );
	TEST( JPDIAG_E_ALREADY_REGISTERED == 
		Res->RegisterMessageDll( Res, L"winlogon.exe", 0, 0 ) );
	TEST_HR( Res->UnregisterMessageDll( Res, L"winlogon.exe" ) );
	

	TEST_HR( Res->UnregisterMessageDll( Res, L"jpdiag.dll" ) );

	// Invalid unregister
	TEST( JPDIAG_E_DLL_NOT_REGISTERED ==
		Res->UnregisterMessageDll( Res, L"jpdiag.dll" ) );
	TEST( JPDIAG_E_DLL_NOT_REGISTERED ==
		Res->UnregisterMessageDll( Res, L"__crap.dll" ) );

	Res->Dereference( Res );
}

static VOID TestResolveMessageInvalids()
{
	PJPDIAG_MESSAGE_RESOLVER Res = NULL;
	WCHAR buf[ 1 ];
	
	TEST_HR( JpdiagCreateMessageResolver( &Res ) );
	TEST( Res );

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
	PJPDIAG_MESSAGE_RESOLVER Res = NULL;
	PMESSAGE_SAMPLE Sample;

	for ( Sample = MessageSamples; Sample->Registrations != 0; Sample++ )
	{
		PCWSTR *Dll;
		WCHAR SmallBuf[ 1 ];
		WCHAR LargeBuf[ 512 ];
		HRESULT Hr;
		SIZE_T MsgLen;

		TEST_HR( JpdiagCreateMessageResolver( &Res ) );
		TEST( Res );
		
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
		TEST( Hr == JPDIAG_E_BUFFER_TOO_SMALL || Hr == Sample->HrExpected );

		//
		// Try real buffer
		//
		Hr = Res->ResolveMessage(
			Res,
			Sample->MsgCode,
			Sample->Flags,
			Sample->InsertionStrings,
			_countof( LargeBuf ),
			LargeBuf );
		TEST( Hr == Sample->HrExpected );

		//
		// Prefix-compare strings.
		//
		MsgLen = wcslen( Sample->ExpectedPrefix );
		LargeBuf[ MsgLen ] = L'\0';
		TEST( 0 == wcscmp( LargeBuf, Sample->ExpectedPrefix ) );

		for ( Dll = Sample->Registrations; *Dll != 0; Dll++ )
		{
			TEST_HR( Res->UnregisterMessageDll( Res, *Dll ) );
		}
		Res->Dereference( Res );
	}

}

VOID TestResolver()
{
	TestDelete();
	TestRegisterUnregisterMessageDll();
	TestResolveMessageInvalids();
	TestResolveMessages();
}