/*----------------------------------------------------------------------
 *	Purpose:
 *		Query Module IOCTL.
 *
 *	Copyright:
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

#include <cfix.h>
#include <cfixkrio.h>
#include <stdlib.h>

#include "util.h"

#pragma warning( push )
#pragma warning( disable: 4201 )
#include <winioctl.h>
#pragma warning( pop )

void CreateCloseDevice()
{
	HANDLE Dev[ 3 ];
	ULONG Index;

	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	__try
	{
		for ( Index = 0; Index < _countof( Dev ); Index++ )
		{
			Dev[ Index ] = CreateFile(
				L"\\\\.\\Cfixkr",
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL  );
			CFIX_ASSERT( Dev[ Index ] != INVALID_HANDLE_VALUE );
		}

		for ( Index = 0; Index < _countof( Dev ); Index++ )
		{
			CFIX_ASSERT( CloseHandle( Dev[ Index ] ) );
		}
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
	}
}

void SendInvalidIoctl()
{
	HANDLE Dev;
	DWORD Cb;
	UCHAR Buffer[ 1 ];

	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	__try
	{
		Dev = CreateFile(
			L"\\\\.\\Cfixkr",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		CFIX_ASSERT( Dev != INVALID_HANDLE_VALUE );

		CFIX_ASSERT( ! DeviceIoControl(
			Dev,
			IOCTL_CHANGER_GET_PARAMETERS,
			Buffer,
			sizeof( Buffer ),
			Buffer,
			sizeof( Buffer ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( ERROR_INVALID_FUNCTION, GetLastError() );
		CFIX_ASSERT( CloseHandle( Dev ) );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
	}
}

void QueryModulesInvalidRequestSize()
{
	HANDLE Dev;
	DWORD Cb;
	ULONG Res;
	
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	__try
	{
		Dev = CreateFile(
			L"\\\\.\\Cfixkr",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		CFIX_ASSERT( Dev != INVALID_HANDLE_VALUE );

		CFIX_ASSERT( ! DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_QUERY_TEST_MODULE,
			NULL,
			0,
			&Res,
			sizeof( ULONG ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( ERROR_INVALID_PARAMETER, GetLastError() );
		CFIX_ASSERT( CloseHandle( Dev ) );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
	}
}

void QueryNonexistingModule()
{
	HANDLE Dev;
	DWORD Cb;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST Req;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE Res;

	Req.DriverBaseAddress = 0xDEADBEEF;

	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	__try
	{
		Dev = CreateFile(
			L"\\\\.\\Cfixkr",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		CFIX_ASSERT( Dev != INVALID_HANDLE_VALUE );

		CFIX_ASSERT( ! DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_QUERY_TEST_MODULE,
			&Req,
			sizeof( CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST ),
			&Res,
			sizeof( CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( ERROR_NOT_FOUND, GetLastError() );
		CFIX_ASSERT( CloseHandle( Dev ) );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
	}
}

void QueryTestlib1()
{
	HANDLE Dev;
	DWORD Cb;
	CFIXKR_IOCTL_GET_MODULES ListRes;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST QryReq;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE QryRes;
	PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE QryResDyn = NULL;
	
	PCFIXKRIO_FIXTURE Fixture;
	PWSTR Name;
	
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadDriver( L"testklib1", L"testklib1.sys" ) );
	__try
	{
		Dev = CreateFile(
			L"\\\\.\\Cfixkr",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		CFIX_ASSERT( Dev != INVALID_HANDLE_VALUE );

		//
		// Get load address.
		//
		CFIX_ASSERT( DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_GET_TEST_MODULES,
			NULL,
			0,
			&ListRes,
			sizeof( ListRes ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( CFIXKR_IOCTL_GET_MODULES ), Cb );
		CFIX_ASSERT( ListRes.Count == 1 );
		CFIX_ASSERT( ListRes.DriverLoadAddress[ 0 ] & 0x80000000 );
		CFIX_ASSERT( ( ListRes.DriverLoadAddress[ 0 ] & 0x00000FFF ) == 0 );

		//
		// Query size.
		//
		QryReq.DriverBaseAddress = ListRes.DriverLoadAddress[ 0 ];
		CFIX_ASSERT( ! DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_QUERY_TEST_MODULE,
			&QryReq,
			sizeof( QryReq ),
			&QryRes,
			sizeof( QryRes ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( ERROR_MORE_DATA, GetLastError() );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( ULONG ), Cb );
		CFIX_ASSERT( QryRes.u.SizeRequired > 4 );
		CFIX_ASSERT( QryReq.DriverBaseAddress == ListRes.DriverLoadAddress[ 0 ] );

		//CFIX_LOG( L"QryRes.u.SizeRequired = %d", QryRes.u.SizeRequired  );
		
		//
		// Query with approproiate buffer.
		//
		QryResDyn = ( PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE )
			malloc( QryRes.u.SizeRequired + sizeof( ULONG ) );
		CFIX_ASSERT( QryResDyn );
		if ( ! QryResDyn ) __leave;

		//
		// Write guard.
		//
		* ( PULONG ) ( ( ( PUCHAR ) QryResDyn ) + QryRes.u.SizeRequired ) = 0xDEADBEEF;
		CFIX_ASSERT( DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_QUERY_TEST_MODULE,
			&QryReq,
			sizeof( QryReq ),
			QryResDyn,
			QryRes.u.SizeRequired,
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( QryRes.u.SizeRequired, Cb );
		CFIX_ASSERT_EQUALS_DWORD( 0xDEADBEEF, *( PDWORD ) ( ( ( PUCHAR ) QryResDyn ) + QryRes.u.SizeRequired ) );
		CFIX_ASSERT( CloseHandle( Dev ) );

		CFIX_ASSERT_EQUALS_DWORD( 1, QryResDyn->u.TestModule.FixtureCount );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( CFIXKRIO_TEST_MODULE ), QryResDyn->u.TestModule.FixtureOffsets[ 0 ] );

		Fixture = ( PCFIXKRIO_FIXTURE ) 
			( ( ( PUCHAR ) QryResDyn ) + QryResDyn->u.TestModule.FixtureOffsets[ 0 ] );
		CFIX_ASSERT_EQUALS_DWORD( 0, Fixture->Key );
		CFIX_ASSERT_EQUALS_DWORD( 2, Fixture->EntryCount );
		CFIX_ASSERT_EQUALS_DWORD( 20, Fixture->NameLength ); // L"KernelTest"

		// N.B. Name is not zero-terminated.
		Name = ( PWSTR ) ( ( ( PUCHAR ) QryResDyn ) + Fixture->NameOffset );
		CFIX_ASSERT( 0 == memcmp( Name, L"KernelTest", 20 ) );

		CFIX_ASSERT( Fixture->Entries[ 0 ].Type == CfixEntryTypeTestcase );
		CFIX_ASSERT( Fixture->Entries[ 0 ].Key == 0 );
		CFIX_ASSERT_EQUALS_DWORD( 8, Fixture->Entries[ 0 ].NameLength );	// "Fail"
		
		// N.B. Name is not zero-terminated.
		Name = ( PWSTR ) ( ( ( PUCHAR ) QryResDyn ) + Fixture->Entries[ 0 ].NameOffset );
		CFIX_ASSERT( 0 == memcmp( Name, L"Fail", 8 ) );
	}
	__finally
	{
		UnloadDriver( L"testklib1" );
		UnloadDriver( L"cfixkr" );

		if ( QryResDyn ) free( QryResDyn );
	}
}

void QueryTestlib2()
{
	HANDLE Dev;
	DWORD Cb;
	CFIXKR_IOCTL_GET_MODULES ListRes;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST QryReq;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE QryRes;
	PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE QryResDyn = NULL;
	
	PCFIXKRIO_FIXTURE Fixture;
	PWSTR Name;
	
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadDriver( L"testklib2", L"testklib2.sys" ) );
	__try
	{
		Dev = CreateFile(
			L"\\\\.\\Cfixkr",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		CFIX_ASSERT( Dev != INVALID_HANDLE_VALUE );

		//
		// Get load address.
		//
		CFIX_ASSERT( DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_GET_TEST_MODULES,
			NULL,
			0,
			&ListRes,
			sizeof( ListRes ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( CFIXKR_IOCTL_GET_MODULES ), Cb );
		CFIX_ASSERT( ListRes.Count == 1 );
		CFIX_ASSERT( ListRes.DriverLoadAddress[ 0 ] & 0x80000000 );
		CFIX_ASSERT( ( ListRes.DriverLoadAddress[ 0 ] & 0x00000FFF ) == 0 );

		//
		// Query size.
		//
		QryReq.DriverBaseAddress = ListRes.DriverLoadAddress[ 0 ];
		CFIX_ASSERT( ! DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_QUERY_TEST_MODULE,
			&QryReq,
			sizeof( QryReq ),
			&QryRes,
			sizeof( QryRes ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( ERROR_MORE_DATA, GetLastError() );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( ULONG ), Cb );
		CFIX_ASSERT( QryRes.u.SizeRequired > 4 );
		CFIX_ASSERT( QryReq.DriverBaseAddress == ListRes.DriverLoadAddress[ 0 ] );

		//CFIX_LOG( L"QryRes.u.SizeRequired = %d", QryRes.u.SizeRequired  );
		
		//
		// Query with approproiate buffer.
		//
		QryResDyn = ( PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE )
			malloc( QryRes.u.SizeRequired + sizeof( ULONG ) );
		CFIX_ASSERT( QryResDyn );
		if ( ! QryResDyn ) __leave;

		//
		// Write guard.
		//
		* ( PULONG ) ( ( ( PUCHAR ) QryResDyn ) + QryRes.u.SizeRequired ) = 0xDEADBEEF;
		CFIX_ASSERT( DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_QUERY_TEST_MODULE,
			&QryReq,
			sizeof( QryReq ),
			QryResDyn,
			QryRes.u.SizeRequired,
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( QryRes.u.SizeRequired, Cb );
		CFIX_ASSERT_EQUALS_DWORD( 0xDEADBEEF, *( PDWORD ) ( ( ( PUCHAR ) QryResDyn ) + QryRes.u.SizeRequired ) );
		CFIX_ASSERT( CloseHandle( Dev ) );

		CFIX_ASSERT_EQUALS_DWORD( 1, QryResDyn->u.TestModule.FixtureCount );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( CFIXKRIO_TEST_MODULE ), QryResDyn->u.TestModule.FixtureOffsets[ 0 ] );

		Fixture = ( PCFIXKRIO_FIXTURE ) 
			( ( ( PUCHAR ) QryResDyn ) + QryResDyn->u.TestModule.FixtureOffsets[ 0 ] );
		CFIX_ASSERT_EQUALS_DWORD( 0, Fixture->Key );
		CFIX_ASSERT_EQUALS_DWORD( 2, Fixture->NameLength ); // L"X"
		CFIX_ASSERT_EQUALS_DWORD( 0, Fixture->EntryCount );

		// N.B. Name is not zero-terminated.
		Name = ( PWSTR ) ( ( ( PUCHAR ) QryResDyn ) + Fixture->NameOffset );
		CFIX_ASSERT( 0 == memcmp( Name, L"X", 2 ) );
	}
	__finally
	{
		UnloadDriver( L"testklib2" );
		UnloadDriver( L"cfixkr" );

		if ( QryResDyn ) free( QryResDyn );
	}
}


CFIX_BEGIN_FIXTURE( QueryModuleIoctl )
	CFIX_FIXTURE_ENTRY( CreateCloseDevice )
	CFIX_FIXTURE_ENTRY( SendInvalidIoctl )
	CFIX_FIXTURE_ENTRY( QueryModulesInvalidRequestSize )
	CFIX_FIXTURE_ENTRY( QueryNonexistingModule )
	CFIX_FIXTURE_ENTRY( QueryTestlib1 )
	CFIX_FIXTURE_ENTRY( QueryTestlib2 )
CFIX_END_FIXTURE()
