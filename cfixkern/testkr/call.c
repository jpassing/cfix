/*----------------------------------------------------------------------
 *	Purpose:
 *		Test of call-routine IOCTL.
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

ULONGLONG GetSomeLoadAddress(
	__in HANDLE Dev
	)
{
	DWORD Cb;
	CFIXKR_IOCTL_GET_MODULES Res;
	
	CFIX_ASSERT( DeviceIoControl(
		Dev,
		CFIXKR_IOCTL_GET_TEST_MODULES,
		NULL,
		0,
		&Res,
		sizeof( CFIXKR_IOCTL_GET_MODULES ),
		&Cb,
		NULL ) );
	CFIX_ASSERT_EQUALS_DWORD( sizeof( CFIXKR_IOCTL_GET_MODULES ), Cb );
	CFIX_ASSERT( Res.Count >= 1 );
	CFIX_ASSERT( Res.DriverLoadAddress[ 0 ] & 0x80000000 );
	CFIX_ASSERT( ( Res.DriverLoadAddress[ 0 ] & 0x00000FFF ) == 0 );

	return Res.DriverLoadAddress[ 0 ];
}

static CFIXKR_IOCTL_CALL_ROUTINE_REQUEST InvalidRequests[] =
{
	{ 0xDEADBEEF, 0, 0, { CfixBreak, CfixAbort }, 0 },
	{ 0, 0, 3, { CfixBreak, CfixAbort }, 0 },			// one off!
	{ 0, 1, 0, { CfixBreak, CfixAbort }, 0 },			// one off!
	{ 0x1000, 0, 0, { CfixBreak, CfixAbort + 1 }, 0 },
	{ 0x1000, 0, 0, { CfixAbort + 1, CfixAbort }, 0 },
	{ 0, 0, 0xffff, { CfixBreak, CfixAbort }, 0 },
	{ 0, 0xffff, 0, { CfixBreak, CfixAbort }, 0 },
	{ 0, 0xffff, 0xffff, { CfixBreak, CfixAbort }, 0 }
};

void CallRoutineWithInvalidParams()
{
	DWORD Cb;
	HANDLE Dev;
	ULONGLONG LoadAddress;
	ULONG Index;
	CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res;
	
	if ( ! IsDriverLoaded( L"cfixkr" ) )
	{
		CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	}
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

		LoadAddress = GetSomeLoadAddress( Dev );
		
		for ( Index = 0; Index < _countof( InvalidRequests ); Index++ )
		{
			DWORD Expected;

			if ( InvalidRequests[ Index ].DriverBaseAddress == 0 )
			{
				Expected = ERROR_PROC_NOT_FOUND;
				InvalidRequests[ Index ].DriverBaseAddress = LoadAddress;
			}
			else if ( InvalidRequests[ Index ].DriverBaseAddress == 0x1000 )
			{
				Expected = ERROR_INVALID_PARAMETER;
			}
			else
			{
				Expected = ERROR_NOT_FOUND;
			}

			CFIX_ASSERT( ! DeviceIoControl( 
				Dev,
				CFIXKR_IOCTL_CALL_ROUTINE,
				&InvalidRequests[ Index ],
				sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ),
				&Res,
				sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ),
				&Cb,
				NULL ) );
			CFIX_ASSERT_EQUALS_DWORD( Expected, GetLastError() );
		}

		CloseHandle( Dev );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
		UnloadDriver( L"testklib1" );
	}
	Sleep(2000);
}

void CallWhileDriverUnloaded()
{
	DWORD Cb;
	HANDLE Dev;
	CFIXKR_IOCTL_CALL_ROUTINE_REQUEST Req;
	CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res;
	
	if ( ! IsDriverLoaded( L"cfixkr" ) )
	{
		CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	}
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

		Req.DriverBaseAddress = GetSomeLoadAddress( Dev );
		Req.FixtureKey = 0;
		Req.RoutineKey = 0;
		Req.Dispositions.FailedAssertion	= CfixContinue;
		Req.Dispositions.UnhandledException	= CfixContinue;
		
		//
		// Unload driver to trigger error.
		//
		UnloadDriver( L"testklib1" );

		CFIX_ASSERT( ! DeviceIoControl( 
			Dev,
			CFIXKR_IOCTL_CALL_ROUTINE,
			&Req,
			sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ),
			&Res,
			sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ),
			&Cb,
			NULL ) );
		
		//
		// ERROR_NOT_FOUND is a lot more likely than getting
		// a ERROR_BAD_DRIVER
		//
		CFIX_ASSERT_EQUALS_DWORD( ERROR_NOT_FOUND, GetLastError() );

		CloseHandle( Dev );
	}
	__finally
	{
		UnloadDriver( L"cfixkr" );
	}
}

void CallValidRoutines()
{
	DWORD Cb;
	HANDLE Dev;
	CFIXKR_IOCTL_CALL_ROUTINE_REQUEST Req;
	CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res;
	
	if ( ! IsDriverLoaded( L"cfixkr" ) )
	{
		CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	}
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

		Req.DriverBaseAddress = GetSomeLoadAddress( Dev );
		Req.Dispositions.FailedAssertion	= CfixContinue;
		Req.Dispositions.UnhandledException	= CfixContinue;

		//
		// Call 'Fail'.
		//
		Req.FixtureKey = 0;
		Req.RoutineKey = 0;

		CFIX_ASSERT( DeviceIoControl( 
			Dev,
			CFIXKR_IOCTL_CALL_ROUTINE,
			&Req,
			sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ),
			&Res,
			sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ),
			&Cb,
			NULL ) );
		CFIX_ASSERT( Res.RoutineRanToCompletion );
		CFIX_ASSERT( ! Res.AbortRun );

		//
		// Call 'Succeed'.
		//
		Req.FixtureKey = 0;
		Req.RoutineKey = 1;

		CFIX_ASSERT( DeviceIoControl( 
			Dev,
			CFIXKR_IOCTL_CALL_ROUTINE,
			&Req,
			sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ),
			&Res,
			sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ),
			&Cb,
			NULL ) );
		CFIX_ASSERT( Res.RoutineRanToCompletion );
		CFIX_ASSERT( ! Res.AbortRun );

		CloseHandle( Dev );
	}
	__finally
	{
		UnloadDriver( L"testklib1" );
		UnloadDriver( L"cfixkr" );
	}
}

CFIX_BEGIN_FIXTURE( CallRoutineIoctl )
	CFIX_FIXTURE_SETUP( UnloadAllCfixDrivers )
	CFIX_FIXTURE_ENTRY( CallRoutineWithInvalidParams )
	CFIX_FIXTURE_ENTRY( CallWhileDriverUnloaded )
	CFIX_FIXTURE_ENTRY( CallValidRoutines )
CFIX_END_FIXTURE()
