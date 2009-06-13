/*----------------------------------------------------------------------
 * Purpose:
 *		Test case for the methods of the registry configuration store
 *
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
#include "regvirt.h"

/*----------------------------------------------------------------------
 *
 *	Parameters
 *
 */
static struct _PARAM_CB_ACCESSMODE
{
	DWORD Mode;
} ParamAccessMode[] =
{
	{ CDIAG_CFGS_ACCESS_READ_USER },
	{ CDIAG_CFGS_ACCESS_WRITE_USER },
	{ CDIAG_CFGS_ACCESS_WRITE_USER | CDIAG_CFGS_ACCESS_READ_USER },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE },
	{ CDIAG_CFGS_ACCESS_WRITE_MACHINE },
	{ CDIAG_CFGS_ACCESS_READ_MACHINE | CDIAG_CFGS_ACCESS_WRITE_MACHINE },
	{ CDIAG_CFGS_ACCESS_ALL },
	{ 0 }
};

typedef struct _PARAM_CALLBACK
{
	CDIAG_CONFIGSTORE_UPDATE_CALLBACK Proc;
	ULONG Calls;
} PARAM_CALLBACK, *PPARAM_CALLBACK;

static VOID CALLBACK UpdateCallbackA( PVOID Context )
{
	PPARAM_CALLBACK cb = ( PPARAM_CALLBACK ) Context;
	TEST( cb->Proc == UpdateCallbackA );
	cb->Calls++;
}

static VOID CALLBACK UpdateCallbackB( PVOID Context )
{
	PPARAM_CALLBACK cb = ( PPARAM_CALLBACK ) Context;
	TEST( cb->Proc == UpdateCallbackB );
	cb->Calls++;
}

static struct _PARAM_CALLBACK Callbacks[] = 
{
	{ UpdateCallbackA, 0 },
	{ UpdateCallbackB, 0 },
	{ NULL, 0 }
};

volatile LONG KeepGeneratingUpdates;

/*++
	Routine description:
		Keeps changing the callback
--*/
static DWORD CALLBACK AlterCallbackThreadProc( PVOID pv )
{
	PCDIAG_CONFIGURATION_STORE Store = ( PCDIAG_CONFIGURATION_STORE ) pv;
	UINT i, current = 0;
	
	for ( i = 0; i < 100; i++ )
	{
		Sleep( 2 );

		PPARAM_CALLBACK cb = &Callbacks[ current++ % _countof( Callbacks ) ];
		TEST_HR( Store->RegisterUpdateCallback(
			Store,
			cb->Proc,
			cb->Proc ? cb : NULL ) );
	}

	InterlockedDecrement( &KeepGeneratingUpdates );

	for ( i = 0; i < _countof( Callbacks ); i++ )
	{
		if ( Callbacks[ i ].Proc )
		{
			TEST( Callbacks[ i ].Calls > 0 );
		}
	}

	return 0;
}

static DWORD CALLBACK TriggerUpdatesThreadProc( PVOID Unused )
{
	PCDIAG_CONFIGURATION_STORE Store;
	DWORD Value = 0;

	UNREFERENCED_PARAMETER( Unused );

	TEST_HR( CdiagCreateRegistryStore(
			 L"Software\\JP\\__test",
			 CDIAG_CFGS_ACCESS_WRITE_USER,
			 &Store ) );

	while ( KeepGeneratingUpdates > 0 )
	{
		Sleep( 0 );

		TEST_HR( Store->WriteDwordSetting( 
			Store, L"X", L"X", CdiagUserScope, Value++ ) );
	}

	return 0;
}


static void TestCdiagCreateRegistryStoreCallback()
{
	//
	// Virtualize registry access s.t. we do not need admin creds
	//
	SHDeleteKey( HKEY_CURRENT_USER, L"Software\\JP\\__test\\virtual" );
	TEST_HR( EnableRegistryRedirection( L"Software\\JP\\__test\\virtual" ) );

	
	for ( struct _PARAM_CB_ACCESSMODE *mode = ParamAccessMode;
		  mode->Mode != 0;
		  mode++ )
	{
		PCDIAG_CONFIGURATION_STORE Store;
		TEST_HR( CdiagCreateRegistryStore(
			 L"Software\\JP\\__test",
			 mode->Mode,
			 &Store ) );

		KeepGeneratingUpdates = 1;

		HANDLE AlterThread = CreateThread( 
			NULL, 0, AlterCallbackThreadProc, Store, 0, NULL );
		TEST( AlterThread );

		HANDLE TriggerThread = CreateThread( 
			NULL, 0, TriggerUpdatesThreadProc, Store, 0, NULL );
		TEST( TriggerThread );

		HANDLE handles[] = { AlterThread, TriggerThread };
		WaitForMultipleObjects( _countof( handles ), handles, TRUE, INFINITE );

		Store->Delete( Store );
		TEST( CloseHandle( AlterThread ) );
		TEST( CloseHandle( TriggerThread ) );
	}

	TEST_HR( DisableRegistryRedirection() );
}

CFIX_BEGIN_FIXTURE( RegistryStoreCallback )
	CFIX_FIXTURE_ENTRY( TestCdiagCreateRegistryStoreCallback )
CFIX_END_FIXTURE()
