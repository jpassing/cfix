/*----------------------------------------------------------------------
 *	Purpose:
 *		Get Modules IOCTL.
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

void ListModulesInvalidRequestSize()
{
	HANDLE Dev;
	DWORD Cb;
	ULONG Req;
	CFIXKR_IOCTL_GET_MODULES Res;
	
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
			CFIXKR_IOCTL_GET_TEST_MODULES,
			&Req,
			sizeof( ULONG ),	// should be 0.
			&Res,
			sizeof( CFIXKR_IOCTL_GET_MODULES ),
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

void ListModulesInvalidResponseSize()
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
			CFIXKR_IOCTL_GET_TEST_MODULES,
			NULL,
			0,
			&Res,
			sizeof( ULONG ),	// too small
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

void ListModulesExpectEmptyResponse()
{
	HANDLE Dev;
	DWORD Cb;
	CFIXKR_IOCTL_GET_MODULES Res;
	
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

		CFIX_ASSERT( DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_GET_TEST_MODULES,
			NULL,
			0,
			&Res,
			sizeof( CFIXKR_IOCTL_GET_MODULES ),
			&Cb,
			NULL ) );
		CFIX_ASSERT( CloseHandle( Dev ) );
		CFIX_ASSERT( Cb == sizeof( ULONG ) );
		CFIX_ASSERT( Res.Count == 0 );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
	}
}

void ListModulesExpectOneResponse()
{
	HANDLE Dev;
	DWORD Cb;
	CFIXKR_IOCTL_GET_MODULES Res;
	
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

		CFIX_ASSERT( DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_GET_TEST_MODULES,
			NULL,
			0,
			&Res,
			sizeof( CFIXKR_IOCTL_GET_MODULES ),
			&Cb,
			NULL ) );
		CFIX_ASSERT( CloseHandle( Dev ) );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( CFIXKR_IOCTL_GET_MODULES ), Cb );
		CFIX_ASSERT( Res.Count == 1 );
		CFIX_ASSERT( Res.DriverLoadAddress[ 0 ] & 0x80000000 );
		CFIX_ASSERT( ( Res.DriverLoadAddress[ 0 ] & 0x00000FFF ) == 0 );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
		UnloadDriver( L"testklib1" );
	}
}

void ListModulesExpectOverflow()
{
	HANDLE Dev;
	DWORD Cb;
	CFIXKR_IOCTL_GET_MODULES Res;
	
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadDriver( L"testklib1", L"testklib1.sys" ) );
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

		CFIX_ASSERT( ! DeviceIoControl(
			Dev,
			CFIXKR_IOCTL_GET_TEST_MODULES,
			NULL,
			0,
			&Res,
			sizeof( Res ),
			&Cb,
			NULL ) );
		CFIX_ASSERT_EQUALS_DWORD( ERROR_MORE_DATA, GetLastError() );
		CFIX_ASSERT( CloseHandle( Dev ) );
		CFIX_ASSERT_EQUALS_DWORD( sizeof( Res ), Cb );
		CFIX_ASSERT_EQUALS_DWORD( 2, Res.Count );
		CFIX_ASSERT( Res.DriverLoadAddress[ 0 ] & 0x80000000 );
		CFIX_ASSERT( ( Res.DriverLoadAddress[ 0 ] & 0x00000FFF ) == 0 );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
		UnloadDriver( L"testklib1" );
		UnloadDriver( L"testklib2" );
	}
}

CFIX_BEGIN_FIXTURE( GetModulesIoctl )
	CFIX_FIXTURE_ENTRY( ListModulesInvalidRequestSize )
	CFIX_FIXTURE_ENTRY( ListModulesInvalidResponseSize )
	CFIX_FIXTURE_ENTRY( ListModulesExpectEmptyResponse )
	CFIX_FIXTURE_ENTRY( ListModulesExpectOneResponse )
	CFIX_FIXTURE_ENTRY( ListModulesExpectOverflow )
CFIX_END_FIXTURE()
