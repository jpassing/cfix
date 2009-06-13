/*----------------------------------------------------------------------
 * Purpose:
 *		Test case for the methods of the registry configuration store
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
#include "regvirt.h"

/*----------------------------------------------------------------------
 *
 *	Parameters
 *
 */

#define MAXALLOWEDNAME L"12345678901234567890123456789012"

static struct _PARAM_NAME
{
	LPCWSTR Name;
	HRESULT ExpectedHr;
} ParamName[] =
{
	{ L"",						E_INVALIDARG },
	{ L" ",						E_INVALIDARG },
	{ MAXALLOWEDNAME L".",		E_INVALIDARG },
	{ L"x",						S_OK },
	{ MAXALLOWEDNAME,			S_OK },
	{ 0, ( HRESULT ) -1 }
};

typedef union
{
	LPCWSTR String;
	DWORD Dword;
} SETTING_VALUE, *PSETTING_VALUE;

static SETTING_VALUE DwordValue1, DwordValue2;
static SETTING_VALUE StringValue1, StringValue2;
static SETTING_VALUE MultiValue1, MultiValue2;

static struct _PARAM_ACCESS
{
	DWORD AccessMode;
	struct 
	{
		DWORD Type;
		PSETTING_VALUE Value;
		HRESULT ExpectedHr;
	} UserWrite;
	struct 
	{
		DWORD Type;
		PSETTING_VALUE Value;
		HRESULT ExpectedHr;
	} GlobalWrite;
	struct 
	{
		DWORD Scope;
		DWORD Type;
		PSETTING_VALUE ExpectedValue;
		HRESULT ExpectedHr;
	} Read;
} ParamAccess[] =
{
	// Nonexistant reads
	{ CDIAG_CFGS_ACCESS_READ_USER,		{ 0, NULL, 0 },	{ 0, NULL, 0 },	{ CdiagUserScope,	 REG_DWORD, NULL,	CDIAG_E_SETTING_NOT_FOUND } },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE,	{ 0, NULL, 0 },	{ 0, NULL, 0 },	{ CdiagGlobalScope, REG_DWORD, NULL,	CDIAG_E_SETTING_NOT_FOUND } },
	{ CDIAG_CFGS_ACCESS_READ_USER |																						  
	  CDIAG_CFGS_ACCESS_READ_MACHINE,	{ 0, NULL, 0 },	{ 0, NULL, 0 },	{ CdiagUserScope,	 REG_DWORD, NULL,	CDIAG_E_SETTING_NOT_FOUND } },
	{ CDIAG_CFGS_ACCESS_READ_USER |																						  
	  CDIAG_CFGS_ACCESS_READ_MACHINE,	{ 0, NULL, 0 },	{ 0, NULL, 0 },	{ CdiagGlobalScope, REG_DWORD, NULL,	CDIAG_E_SETTING_NOT_FOUND } },

	// Insufficient access mode for writing
	{ CDIAG_CFGS_ACCESS_READ_USER,		{ REG_DWORD, &DwordValue1, E_ACCESSDENIED},	{ 0, NULL, 0 },	{ 0, 0, NULL,	S_OK  } },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE,	{ REG_DWORD, &DwordValue1, E_ACCESSDENIED},	{ 0, NULL, 0 },	{ 0, 0, NULL,	S_OK  } },
	{ CDIAG_CFGS_ACCESS_READ_USER |																				  
	  CDIAG_CFGS_ACCESS_WRITE_MACHINE,	{ REG_DWORD, &DwordValue1, E_ACCESSDENIED},	{ 0, NULL, 0 },	{ 0, 0, NULL,	S_OK  } },
																															  
	{ CDIAG_CFGS_ACCESS_READ_MACHINE,	{ 0, NULL, 0 },	{ REG_DWORD, &DwordValue1, E_ACCESSDENIED},	{ 0, 0, NULL,	S_OK  } },
	{ CDIAG_CFGS_ACCESS_READ_USER,		{ 0, NULL, 0 },	{ REG_DWORD, &DwordValue1, E_ACCESSDENIED},	{ 0, 0, NULL,	S_OK  } },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE |																				  
	  CDIAG_CFGS_ACCESS_WRITE_USER,	{ 0, NULL, 0 },	{ REG_DWORD, &DwordValue1, E_ACCESSDENIED},	{ 0, 0, NULL,	S_OK  } },
																															  
	// Insufficient access mode for reading, sufficient for writing															  
	{ CDIAG_CFGS_ACCESS_WRITE_USER,	{ REG_DWORD, &DwordValue1, S_OK }, { 0, NULL, 0 },				{ CdiagUserScope,	  REG_DWORD, &DwordValue1,	E_ACCESSDENIED  } },
	{ CDIAG_CFGS_ACCESS_WRITE_MACHINE,	{ 0, NULL, 0 }, { REG_DWORD, &DwordValue1, S_OK },				{ CdiagGlobalScope, REG_DWORD, &DwordValue1,	E_ACCESSDENIED  } },

	// User read/writes, must ignore global
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	 &DwordValue1,  S_OK },	{ REG_DWORD,	&DwordValue2,  S_OK }, { CdiagUserScope, REG_DWORD,	&DwordValue1,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		 &StringValue1, S_OK },	{ REG_SZ,		&StringValue2, S_OK }, { CdiagUserScope, REG_SZ,		&StringValue1, S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_MULTI_SZ,	 &MultiValue1,  S_OK },	{ REG_MULTI_SZ, &MultiValue2,  S_OK }, { CdiagUserScope, REG_MULTI_SZ,	&MultiValue1,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	 &DwordValue1,  S_OK },	{ REG_DWORD,	&DwordValue2,  S_OK }, { CdiagUserScope, REG_SZ,		&StringValue1, CDIAG_E_SETTING_MISMATCH } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		 &StringValue1, S_OK }, { REG_SZ,		&StringValue2, S_OK }, { CdiagUserScope, REG_DWORD,	&DwordValue1,  CDIAG_E_SETTING_MISMATCH } },

	// Global read/writes, must ignore user
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	&DwordValue1,  S_OK },	{ REG_DWORD,	&DwordValue2,  S_OK }, { CdiagGlobalScope, REG_DWORD,		&DwordValue2,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },	{ REG_SZ,		&StringValue2, S_OK }, { CdiagGlobalScope, REG_SZ,			&StringValue2, S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_MULTI_SZ,	&MultiValue1,  S_OK },	{ REG_MULTI_SZ, &MultiValue2,  S_OK }, { CdiagGlobalScope, REG_MULTI_SZ,	&MultiValue2,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	&DwordValue1,  S_OK },	{ REG_DWORD,	&DwordValue2,  S_OK }, { CdiagGlobalScope, REG_SZ,			&StringValue2, CDIAG_E_SETTING_MISMATCH } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },	{ REG_SZ,		&StringValue2, S_OK }, { CdiagGlobalScope, REG_DWORD,		&DwordValue2,  CDIAG_E_SETTING_MISMATCH } },

	// Effective reads
	{ CDIAG_CFGS_ACCESS_ALL,	{ 0, NULL, 0 },							{ REG_DWORD,	&DwordValue1,  S_OK }, { CdiagEffectiveScope, REG_DWORD,	&DwordValue1,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ 0, NULL, 0 },							{ REG_SZ,		&StringValue1, S_OK }, { CdiagEffectiveScope, REG_SZ,		&StringValue1, S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	&DwordValue1,  S_OK },	{ 0, NULL, 0 },						   { CdiagEffectiveScope, REG_DWORD,	&DwordValue1,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },	{ 0, NULL, 0 },						   { CdiagEffectiveScope, REG_SZ,		&StringValue1, S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_MULTI_SZ, &MultiValue1,  S_OK },	{ 0, NULL, 0 },						   { CdiagEffectiveScope, REG_MULTI_SZ,&MultiValue1,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	&DwordValue1,  S_OK },	{ REG_DWORD,	&DwordValue2,  S_OK }, { CdiagEffectiveScope, REG_DWORD,	&DwordValue1,  S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },	{ REG_SZ,		&StringValue2, S_OK }, { CdiagEffectiveScope, REG_SZ,		&StringValue1, S_OK } },
	{ CDIAG_CFGS_ACCESS_ALL,	{ REG_MULTI_SZ, &MultiValue1,	S_OK }, { REG_MULTI_SZ, &MultiValue2,  S_OK }, { CdiagEffectiveScope, REG_MULTI_SZ,&MultiValue1,  S_OK } },
	//{ CDIAG_CFGS_ACCESS_ALL,	{ REG_DWORD,	&DwordValue1,  S_OK },  { REG_SZ,		&StringValue2, S_OK }, { CdiagEffectiveScope, REG_DWORD,	&DwordValue1,  CDIAG_E_SETTING_MISMATCH } },
	//{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },  { REG_DWORD,	&DwordValue2,  S_OK }, { CdiagEffectiveScope, REG_SZ,		&StringValue1, CDIAG_E_SETTING_MISMATCH } },
	//{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },  { REG_MULTI_SZ, &MultiValue2,  S_OK }, { CdiagEffectiveScope, REG_SZ,		&StringValue1, CDIAG_E_SETTING_MISMATCH } },
	//{ CDIAG_CFGS_ACCESS_ALL,	{ REG_MULTI_SZ, &MultiValue1,  S_OK },  { REG_DWORD,	&StringValue2, S_OK }, { CdiagEffectiveScope, REG_SZ,		&StringValue1, CDIAG_E_SETTING_MISMATCH } },
	//{ CDIAG_CFGS_ACCESS_ALL,	{ REG_SZ,		&StringValue1, S_OK },  { REG_DWORD,	&StringValue2, S_OK }, { CdiagEffectiveScope, REG_MULTI_SZ,&MultiValue1,  CDIAG_E_SETTING_MISMATCH } },

	{ ( DWORD ) -1 }
};


static void TestCdiagCreateRegistryStoreMethod()
{
	PCDIAG_CONFIGURATION_STORE Store;
	PCWSTR BaseKeyName = L"Software\\JP\\__test";

	DwordValue1.Dword = 1;
	DwordValue2.Dword = 2;
	StringValue1.String = L"Foo";
	StringValue2.String = L"Bar";
	MultiValue1.String = L"Foo\0a\0Bar\0";
	MultiValue2.String = L"zzzzzzzzz\0";
	UINT MultiValueCb = 22;
	
	//
	// Virtualize registry access s.t. we do not need admin creds
	//
	SHDeleteKey( HKEY_CURRENT_USER, L"Software\\JP\\__test\\virtual" );
	TEST_HR( EnableRegistryRedirection( L"Software\\JP\\__test\\virtual" ) );

	TEST_HR( CdiagCreateRegistryStore( 
		BaseKeyName, CDIAG_CFGS_ACCESS_ALL, &Store ) );

	//
	// Test deletion
	//
	TEST( E_INVALIDARG == Store->DeleteSetting( Store, L"A", L"B", CdiagEffectiveScope ) );
	TEST( CDIAG_E_SETTING_NOT_FOUND == Store->DeleteSetting( Store, L"A", L"B", CdiagUserScope ) );
	TEST( CDIAG_E_SETTING_NOT_FOUND == Store->DeleteSetting( Store, L"A", L"B", CdiagGlobalScope) );

	TEST_HR( Store->WriteDwordSetting( Store, L"A", L"B", CdiagUserScope, 0xF00 ) );
	TEST_HR( Store->WriteDwordSetting( Store, L"A", L"B", CdiagGlobalScope, 0xF00 ) );
	TEST_HR( Store->DeleteSetting( Store, L"A", L"B", CdiagUserScope ) );
	TEST_HR( Store->DeleteSetting( Store, L"A", L"B", CdiagGlobalScope ) );

	TEST_HR( Store->WriteStringSetting( Store, L"A", L"B", CdiagUserScope, L"Foo" ) );
	TEST_HR( Store->WriteStringSetting( Store, L"A", L"B", CdiagGlobalScope, L"Foo" ) );
	TEST_HR( Store->DeleteSetting( Store, L"A", L"B", CdiagUserScope ) );
	TEST_HR( Store->DeleteSetting( Store, L"A", L"B", CdiagGlobalScope ) );
	

	//
	// Some invalid args
	//
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"", L"B", CdiagUserScope, L"Foo" ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L" ", L"B", CdiagUserScope, L"Foo" ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"A", L" ", CdiagUserScope, L"Foo" ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"A", L"", CdiagUserScope, L"Foo" ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"A\\B", L"B", CdiagUserScope, L"Foo" ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"A", L"B\\C", CdiagUserScope, L"Foo" ) );

	DWORD dw;
	WCHAR buffer[ 1 ] = { 0 };
	TEST( CDIAG_E_SETTING_NOT_FOUND == Store->DeleteSetting( Store, L"A", L"idonotexist", CdiagGlobalScope ) );
	TEST( CDIAG_E_SETTING_NOT_FOUND == Store->DeleteSetting( Store, L"idonotexist", L"idonotexist", CdiagGlobalScope ) );
	TEST( CDIAG_E_SETTING_NOT_FOUND == Store->ReadDwordSetting( Store, L"A", L"idonotexist", CdiagGlobalScope, &dw ) );
	TEST( CDIAG_E_SETTING_NOT_FOUND == Store->ReadStringSetting( Store, L"idonotexist", L"idonotexist", CdiagGlobalScope, _countof( buffer ), buffer, NULL ) );

	TEST( E_INVALIDARG == Store->WriteDwordSetting( Store, L"A", L"B", CdiagEffectiveScope, 0xF00 ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"A", L"B", CdiagEffectiveScope, L"Foo" ) );

	TEST( E_INVALIDARG == Store->WriteDwordSetting( Store, L"", L"B", CdiagUserScope, 0xF00 ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"", L"B", CdiagUserScope, L"Foo" ) );

	TEST( E_INVALIDARG == Store->WriteDwordSetting( Store, L"A", L" ", CdiagUserScope, 0xF00 ) );
	TEST( E_INVALIDARG == Store->WriteStringSetting( Store, L"A", L" ", CdiagUserScope, L"Foo" ) );

	Store->Delete( Store );


	for ( struct _PARAM_NAME *GroupName = ParamName;
		GroupName->ExpectedHr != ( HRESULT ) -1;
		GroupName++ )
	{
		for ( struct _PARAM_NAME *name = ParamName;
			name->ExpectedHr != ( HRESULT ) -1;
			name++ )
		{
			for ( struct _PARAM_ACCESS *access = ParamAccess;
				( DWORD ) access->AccessMode != -1;
				access++ )
			{
				HRESULT expectedHr;
				TEST_HR( CdiagCreateRegistryStore( 
					BaseKeyName, 
					access->AccessMode, 
					&Store ) );

				//
				// Write user
				//
				if ( access->UserWrite.Type != 0 )
				{
					expectedHr = 
						S_OK == GroupName->ExpectedHr &&
						S_OK == name->ExpectedHr 
							? access->UserWrite.ExpectedHr
							: E_INVALIDARG;
					if ( access->UserWrite.Type == REG_DWORD )
					{
						TEST( expectedHr ==
							Store->WriteDwordSetting(
								Store,
								GroupName->Name,
								name->Name,
								CdiagUserScope,
								access->UserWrite.Value->Dword ) );
					}
					else if ( access->UserWrite.Type == REG_MULTI_SZ )
					{
						TEST( expectedHr ==
							Store->WriteMultiStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								CdiagUserScope,
								access->UserWrite.Value->String ) );
					}
					else
					{
						TEST( expectedHr ==
							Store->WriteStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								CdiagUserScope,
								access->UserWrite.Value->String ) );
					}
				}

				//
				// Write global
				//
				if ( access->GlobalWrite.Type != 0 )
				{
					expectedHr = 
						S_OK == GroupName->ExpectedHr &&
						S_OK == name->ExpectedHr 
							? access->GlobalWrite.ExpectedHr
							: E_INVALIDARG;
					if ( access->GlobalWrite.Type == REG_DWORD )
					{
						TEST( expectedHr ==
							Store->WriteDwordSetting(
								Store,
								GroupName->Name,
								name->Name,
								CdiagGlobalScope,
								access->GlobalWrite.Value->Dword ) );
					}
					else if ( access->GlobalWrite.Type == REG_MULTI_SZ )
					{
						TEST( expectedHr ==
							Store->WriteMultiStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								CdiagGlobalScope,
								access->GlobalWrite.Value->String ) );
					}
					else
					{
						TEST( expectedHr ==
							Store->WriteStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								CdiagGlobalScope,
								access->GlobalWrite.Value->String ) );
					}
				}

				//
				// Read
				//
				if ( access->Read.Type != 0 )
				{
					if ( access->Read.Type == REG_DWORD )
					{
						expectedHr = 
							S_OK == GroupName->ExpectedHr &&
							S_OK == name->ExpectedHr 
								? access->Read.ExpectedHr
								: E_INVALIDARG;
					
						DWORD value;
						TEST( expectedHr ==
							Store->ReadDwordSetting(
								Store,
								GroupName->Name,
								name->Name,
								access->Read.Scope,
								&value ) );
						if ( SUCCEEDED( expectedHr ) )
						{
							TEST( value == access->Read.ExpectedValue->Dword );
						}
					}
					else if ( access->Read.Type == REG_MULTI_SZ )
					{
						WCHAR smallbuf[ 1 ];
						PWSTR buf = smallbuf;
						DWORD reqLen = 0, reqLen2;

						expectedHr = 
							S_OK == GroupName->ExpectedHr &&
							S_OK == name->ExpectedHr 
								? CDIAG_E_BUFFER_TOO_SMALL
								: E_INVALIDARG;

						TEST( expectedHr == 
							Store->ReadMultiStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								access->Read.Scope,
								1,
								buf,
								NULL ) );

						// ok, ask len
						TEST( expectedHr == 
							Store->ReadMultiStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								access->Read.Scope,
								1,
								buf,
								&reqLen ) );

						if ( reqLen > 0 )
						{
							// alloc buffer
							expectedHr = 
								S_OK == GroupName->ExpectedHr &&
								S_OK == name->ExpectedHr 
									? access->Read.ExpectedHr
									: E_INVALIDARG;

							LPWSTR newBuf = new WCHAR[ reqLen ];
							TEST( expectedHr ==
								Store->ReadMultiStringSetting(
									Store,
									GroupName->Name,
									name->Name,
									access->Read.Scope,
									reqLen,
									newBuf,
									&reqLen2 ) );
							if ( SUCCEEDED( expectedHr ) )
							{
								TEST( reqLen == reqLen2 );
								TEST( 0 == memcmp( newBuf, access->Read.ExpectedValue->String, MultiValueCb ) );
							}
							delete [] newBuf;
						}
					}
					else
					{
						WCHAR smallbuf[ 1 ];
						PWSTR buf = smallbuf;
						DWORD reqLen = 0, reqLen2;

						expectedHr = 
							S_OK == GroupName->ExpectedHr &&
							S_OK == name->ExpectedHr 
								? CDIAG_E_BUFFER_TOO_SMALL
								: E_INVALIDARG;

						TEST( expectedHr == 
							Store->ReadStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								access->Read.Scope,
								1,
								buf,
								NULL ) );

						// ok, ask len
						TEST( expectedHr == 
							Store->ReadStringSetting(
								Store,
								GroupName->Name,
								name->Name,
								access->Read.Scope,
								1,
								buf,
								&reqLen ) );

						if ( reqLen > 0 )
						{
							// alloc buffer
							expectedHr = 
								S_OK == GroupName->ExpectedHr &&
								S_OK == name->ExpectedHr 
									? access->Read.ExpectedHr
									: E_INVALIDARG;

							LPWSTR newBuf = new WCHAR[ reqLen ];
							TEST( expectedHr ==
								Store->ReadStringSetting(
									Store,
									GroupName->Name,
									name->Name,
									access->Read.Scope,
									reqLen,
									newBuf,
									&reqLen2 ) );
							if ( SUCCEEDED( expectedHr ) )
							{
								TEST( reqLen == reqLen2 );
								TEST( 0 == wcscmp( newBuf, access->Read.ExpectedValue->String ) );
							}
							delete [] newBuf;
						}
					}
				}

				Store->Delete( Store );
				TEST( NOERROR == SHDeleteKey( HKEY_CURRENT_USER, L"Software\\JP\\__test\\virtual" ) );
			}
		}
	}

	TEST_HR( DisableRegistryRedirection() );
}

CFIX_BEGIN_FIXTURE( RegistryStoreMethods )
	CFIX_FIXTURE_ENTRY( TestCdiagCreateRegistryStoreMethod )
CFIX_END_FIXTURE()
