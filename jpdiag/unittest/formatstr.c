/*----------------------------------------------------------------------
 * Purpose:
 *		Test case for JpdiagpFormatString
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
#include "stdafx.h"

typedef struct _FORMAT_VARIABLE
{
	PWSTR Name;
	PWSTR Format;
} FORMAT_VARIABLE, *PFORMAT_VARIABLE;

extern HRESULT JPDIAGCALLTYPE JpdiagpFormatString(
	__in ULONG VarCount,
	__in PFORMAT_VARIABLE Variables,
	__in DWORD_PTR *Bindings,
	__in PCWSTR Format,
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer 
	);

static FORMAT_VARIABLE VarDef[] =
{
	{ L"x",		L"%x" },
	{ L"Str1",	L"%s" },
	{ L"Int",	L"%d" }
};

static DWORD_PTR VarBindings[][ 3 ] =
{
	{ 0,		  ( DWORD_PTR ) ( PVOID ) L"",			0 },
	{ 0xDEADBEEF, ( DWORD_PTR ) ( PVOID ) L" Foo",		( DWORD_PTR ) -1 }
};

typedef struct _FORMAT
{
	PCWSTR Format;
	DWORD_PTR* Bindings;
	PCWSTR Expected;
} FORMAT, *PFORMAT;

static FORMAT Formats[] =
{
	{ L"x",								VarBindings[ 0 ], L"x" },
	{ L"%x%%%x %iNt %str1",				VarBindings[ 0 ], L"0%0 0 " },
	{ L" %x%%%x %iNt %str1 ",			VarBindings[ 0 ], L" 0%0 0  " },
	{ L"% %% %crap%",					VarBindings[ 0 ], L" % " },
	{ L" %%%% %str1%str1 %str1%int",	VarBindings[ 1 ], L" %%  Foo Foo  Foo-1" },
	{ L"-%x-",							VarBindings[ 1 ], L"-deadbeef-" },
	{ L"-%int-",						VarBindings[ 1 ], L"--1-" },
	{ 0 }
};

static VOID TestFormats()
{
#ifdef _DEBUG
	PFORMAT Format;
	SIZE_T BufferLen;
	WCHAR Buffer[ 100 ];
	HRESULT Hr;

	for ( Format = Formats; Format->Format != 0; Format++ )
	{
		for ( BufferLen = _countof( Buffer ) - 5; BufferLen > 0; BufferLen-- )
		{
			// 
			// Mark end of buffer
			//
			memset( Buffer, 0, sizeof( Buffer ) );
			Buffer[ BufferLen     ] = 0xBAD1;
			Buffer[ BufferLen + 1 ] = 0xBAD2;
			Buffer[ BufferLen + 2 ] = 0xBAD3;
			Buffer[ BufferLen + 3 ] = 0xBAD4;
			Buffer[ BufferLen + 4 ] = 0xBAD5;

			Hr = JpdiagpFormatString(
				3,
				VarDef,
				Format->Bindings,
				Format->Format,
				BufferLen,
				Buffer );
			TEST( JPDIAG_E_BUFFER_TOO_SMALL == Hr ||
				  ( S_OK == Hr &&
					0 == wcscmp( Buffer, Format->Expected ) ) );

			TEST( Buffer[ BufferLen     ] == 0xBAD1 );
			TEST( Buffer[ BufferLen + 1 ] == 0xBAD2 );
			TEST( Buffer[ BufferLen + 2 ] == 0xBAD3 );
			TEST( Buffer[ BufferLen + 3 ] == 0xBAD4 );
			TEST( Buffer[ BufferLen + 4 ] == 0xBAD5 );
		}
	}

#endif
}

static VOID TestIntArgs()
{
#ifdef _DEBUG
	FORMAT_VARIABLE Var = { L"Foo", L"%d" };
	WCHAR Buffer[ 11 ];
	
	DWORD Dw = 1234567890;
	DWORD_PTR Bindings[ 1 ];
	Bindings[ 0 ] = Dw;

	TEST_HR( JpdiagpFormatString(
		1,
		&Var,
		Bindings,
		L"%Foo",
		_countof( Buffer ),
		Buffer ) );
	TEST( 0 == wcscmp( Buffer, L"1234567890" ) );
#endif
}

VOID TestInternals()
{
	TestFormats();
	TestIntArgs();
}