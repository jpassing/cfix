/*----------------------------------------------------------------------
 *	Purpose:
 *		Test of report functionality.
 * 
 *	Copyright:
 *		2008-2009, Johannes Passing (passing at users.sourceforge.net)
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
#include <stdio.h>

#include "util.h"

static BOOL IsWow64()
{
	BOOL Wow64;
	CFIX_ASSERT( CfixklIsWow64Process( GetCurrentProcess(), &Wow64 ) );
	return Wow64;
}

static VOID CheckEventsForLog(
	__in USHORT Routine,
	__in PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res
	)
{
	ULONG Coverage = 0;
	PCFIXKR_EXECUTION_EVENT Event;
	ULONG Index;

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Res + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Res->Events.Count; Index++ )
	{
		CFIX_ASSERT( Event->Type == CfixEventLog );
		CFIX_ASSERT( Event->Size >= sizeof( CFIXKR_EXECUTION_EVENT ) );

		CFIX_ASSERT( Event->Info.Log.MessageOffset + 
					 Event->Info.Log.MessageLength <= Event->Size );

		if ( Routine == 0 )
		{
			CFIX_ASSERT( Event->Info.Log.MessageLength == 40 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Log at PASSIVE_LEVEL",
				Event->Info.Log.MessageLength ) );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}
		else
		{
			CFIX_ASSERT( Event->Info.Log.MessageLength == 18 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Log at %s",
				Event->Info.Log.MessageLength ) );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}
	
		Event = ( PCFIXKR_EXECUTION_EVENT ) 
			( ( PUCHAR ) Event + Event->Size );
	}

	if ( Res->Events.Flags != CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_ASSERT_EQUALS_DWORD( 1, Coverage );
	}
}

static VOID CheckEventsForInconclusivenessPassiveLevel(
	__in PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res
	)
{
	ULONG Coverage = 0;
	PCFIXKR_EXECUTION_EVENT Event;
	ULONG Index;

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Res + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Res->Events.Count; Index++ )
	{
		CFIX_ASSERT( Event->Size >= sizeof( CFIXKR_EXECUTION_EVENT ) );

		if ( Index == 0 )
		{
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Log.MessageLength == 40 );

			#pragma warning( push )
			#pragma warning( disable: 6385 )
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Log at PASSIVE_LEVEL",
				Event->Info.Log.MessageLength ) );
			#pragma warning( pop )

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}
		else if ( Index == 1 )
		{
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Log.MessageLength == 40 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Log at PASSIVE_LEVEL",
				Event->Info.Log.MessageLength ) );
		
			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 2;
		}
		else
		{
			CFIX_ASSERT( Event->Type == CfixEventInconclusiveness );
			CFIX_ASSERT( Event->Info.Inconclusiveness.MessageOffset + 
						 Event->Info.Inconclusiveness.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Inconclusiveness.MessageLength == 110 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Inconclusiveness.MessageOffset,
				L"This blah blah blah blah blah blah blah is inconclusive",
				Event->Info.Inconclusiveness.MessageLength ) );
	
			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 4;
		}

		Event = ( PCFIXKR_EXECUTION_EVENT ) 
			( ( PUCHAR ) Event + Event->Size );
	}

	if ( Res->Events.Flags != CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_ASSERT_EQUALS_DWORD( 7, Coverage );
	}
}

static VOID CheckEventsForInconclusivenessDirql(
	__in PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res
	)
{
	ULONG Coverage = 0;
	PCFIXKR_EXECUTION_EVENT Event;
	ULONG Index;

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Res + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Res->Events.Count; Index++ )
	{
		CFIX_ASSERT( Event->Size >= sizeof( CFIXKR_EXECUTION_EVENT ) );

		if ( Index == 0 )
		{
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Log.MessageLength == 18 );
			
			#pragma warning( push )
			#pragma warning( disable: 6385 )
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Log at %s",
				Event->Info.Log.MessageLength ) );
			#pragma warning( pop )
			
			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}
		else if ( Index == 1 )
		{
			CFIX_ASSERT( Event->Type == CfixEventInconclusiveness );
			CFIX_ASSERT( Event->Info.Inconclusiveness.MessageOffset + 
						 Event->Info.Inconclusiveness.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Inconclusiveness.MessageLength == 230 );
			
			#pragma warning( push )
			#pragma warning( disable: 6385 )
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Inconclusiveness.MessageOffset,
				L"Inconclusive   "
				L"01234567890123456789012345678901234567890123456789"
				L"01234567890123456789012345678901234567890123456789",
				Event->Info.Inconclusiveness.MessageLength ) );
			#pragma warning( pop )

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );
			
			Coverage |= 2;
		}
		else
		{
			//
			// IRQL-warning.
			//
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );
			CFIX_ASSERT( Event->Info.Log.MessageLength == 230 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Testcase aborted at elevated I",
				60 ) );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 4;
		}

		Event = ( PCFIXKR_EXECUTION_EVENT ) 
			( ( PUCHAR ) Event + Event->Size );
	}

	if ( Res->Events.Flags != CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_ASSERT_EQUALS_DWORD( 7, Coverage );
	}
}

static void CheckEventsForException(
	__in PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res
	)
{
	ULONG Coverage = 0;
	PCFIXKR_EXECUTION_EVENT Event;
	ULONG Index;

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Res + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Res->Events.Count; Index++ )
	{
		CFIX_ASSERT( Event->Size >= sizeof( CFIXKR_EXECUTION_EVENT ) );

		if ( Index < 2 )
		{
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Log.MessageLength == 0 );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}
		else
		{
			CFIX_ASSERT( Event->Type == CfixEventUncaughtException );

#ifdef _WIN64
			CFIX_ASSERT( Event->Info.UncaughtException.Type == CfixkrExceptionRecord64 );
			CFIX_ASSERT( Event->Info.UncaughtException.u.ExceptionRecord64
					.ExceptionCode == 'Excp' );
#else
			if ( IsWow64() )
			{
				CFIX_ASSERT( Event->Info.UncaughtException.Type == CfixkrExceptionRecord64 );
				CFIX_ASSERT( Event->Info.UncaughtException.u.ExceptionRecord64
					.ExceptionCode == 'Excp' );
			}
			else
			{
				CFIX_ASSERT( Event->Info.UncaughtException.Type == CfixkrExceptionRecord32 );
				CFIX_ASSERT( Event->Info.UncaughtException.u.ExceptionRecord32
					.ExceptionCode == 'Excp' );
			}
#endif

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}

		Event = ( PCFIXKR_EXECUTION_EVENT ) 
			( ( PUCHAR ) Event + Event->Size );
	}

	if ( Res->Events.Flags != CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_ASSERT_EQUALS_DWORD( 1, Coverage );
	}
}

static VOID CheckEventsForLogAndFailPassiveLevel(
	__in PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res
	)
{
	ULONG Coverage = 0;
	PCFIXKR_EXECUTION_EVENT Event;
	ULONG Index;

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Res + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Res->Events.Count; Index++ )
	{
		CFIX_ASSERT( Event->Size >= sizeof( CFIXKR_EXECUTION_EVENT ) );

		if ( Index <= 1 )
		{
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Log.MessageLength == 0 );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );
			
			Coverage |= 1;
		}
		else
		{
			CFIX_ASSERT( Event->Type == CfixEventFailedAssertion );
			CFIX_ASSERT( Event->Info.FailedAssertion.FileOffset + 
						 Event->Info.FailedAssertion.FileLength <= Event->Size );
			CFIX_ASSERT( Event->Info.FailedAssertion.RoutineOffset + 
						 Event->Info.FailedAssertion.RoutineLength <= Event->Size );
			CFIX_ASSERT( Event->Info.FailedAssertion.ExpressionOffset + 
						 Event->Info.FailedAssertion.ExpressionLength <= Event->Size );

			CFIX_ASSERT( Event->Info.FailedAssertion.FileLength >= 14 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event 
					+ Event->Info.FailedAssertion.FileOffset
					+ Event->Info.FailedAssertion.FileLength
					- 14,
				L"suite.c",
				14 ) );
	
			CFIX_ASSERT( Event->Info.FailedAssertion.RoutineLength == 64 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.FailedAssertion.RoutineOffset,
				L"LogAndFailAtPassiveLevelAndAbort",
				Event->Info.FailedAssertion.RoutineLength ) );

			CFIX_ASSERT( Event->Info.FailedAssertion.Line > 0 );

			CFIX_ASSERT( Event->Info.FailedAssertion.ExpressionLength == 20 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.FailedAssertion.ExpressionOffset,
				L"!\"Untruth\"",
				Event->Info.FailedAssertion.ExpressionLength ) );

			Coverage |= 2;
		}

		Event = ( PCFIXKR_EXECUTION_EVENT ) 
			( ( PUCHAR ) Event + Event->Size );
	}

	if ( Res->Events.Flags != CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_ASSERT_EQUALS_DWORD( 3, Coverage );
	}
}

static VOID CheckEventsForLogAndFailAtDirql(
	__in PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res
	)
{
	ULONG Coverage = 0;
	PCFIXKR_EXECUTION_EVENT Event;
	ULONG Index;

	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Res + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Res->Events.Count; Index++ )
	{
		CFIX_ASSERT( Event->Size >= sizeof( CFIXKR_EXECUTION_EVENT ) );

		if ( Index == 0 )
		{
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );

			CFIX_ASSERT( Event->Info.Log.MessageLength == 0 );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 1;
		}
		else if ( Index == 1 )
		{
			CFIX_ASSERT( Event->Type == CfixEventFailedAssertion );
			CFIX_ASSERT( Event->Info.FailedAssertion.FileOffset + 
						 Event->Info.FailedAssertion.FileLength <= Event->Size );
			CFIX_ASSERT( Event->Info.FailedAssertion.RoutineOffset + 
						 Event->Info.FailedAssertion.RoutineLength <= Event->Size );
			CFIX_ASSERT( Event->Info.FailedAssertion.ExpressionOffset + 
						 Event->Info.FailedAssertion.ExpressionLength <= Event->Size );

			CFIX_ASSERT( Event->Info.FailedAssertion.FileLength >= 14 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event 
					+ Event->Info.FailedAssertion.FileOffset
					+ Event->Info.FailedAssertion.FileLength
					- 14,
				L"suite.c",
				14 ) );
	
			CFIX_ASSERT( Event->Info.FailedAssertion.RoutineLength == 50 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.FailedAssertion.RoutineOffset,
				L"LogAndFailAtDirqlAndAbort",
				Event->Info.FailedAssertion.RoutineLength ) );

			CFIX_ASSERT( Event->Info.FailedAssertion.Line > 0 );

			CFIX_ASSERT( Event->Info.FailedAssertion.ExpressionLength == 2 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.FailedAssertion.ExpressionOffset,
				L"2",
				Event->Info.FailedAssertion.ExpressionLength ) );

			Coverage |= 2;
		}
		else
		{
			//
			// IRQL-warning.
			//
			CFIX_ASSERT( Event->Type == CfixEventLog );
			CFIX_ASSERT( Event->Info.Log.MessageOffset + 
						 Event->Info.Log.MessageLength <= Event->Size );
			CFIX_ASSERT( Event->Info.Log.MessageLength == 230 );
			CFIX_ASSERT( 0 == memcmp( 
				( PUCHAR ) Event + Event->Info.Log.MessageOffset,
				L"Testcase aborted at elevated I",
				60 ) );

			CFIX_ASSERT( Event->StackTrace.FrameCount < 32 );

			Coverage |= 4;
		}

		Event = ( PCFIXKR_EXECUTION_EVENT ) 
			( ( PUCHAR ) Event + Event->Size );
	}

	if ( Res->Events.Flags != CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_ASSERT_EQUALS_DWORD( 7, Coverage );
	}
}

static void TestEventReporting()
{
	DWORD Cb;
	HANDLE Dev;
	CFIXKR_IOCTL_CALL_ROUTINE_REQUEST Req;
	PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Res;
	
	if ( ! IsDriverLoaded( L"cfixkr" ) )
	{
		CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadReflector() );
	}
	CFIX_ASSERT_EQUALS_DWORD( ERROR_SUCCESS, LoadDriver( L"testklib7", L"testklib7.sys" ) );
	__try
	{
		ULONG ResponseSize = sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE );
		ULONG SuccessfulAttemptsWithTrunc = 0;
		ULONG SuccessfulAttempts = 0;

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
		Req.Dispositions.FailedAssertion	= CfixAbort;
		Req.Dispositions.UnhandledException	= CfixContinue;

		for ( ; ResponseSize < 1536; ResponseSize++ )
		{
			USHORT Routine;
			BOOL Success;

			Res = malloc( ResponseSize );
			CFIX_ASSERT( Res != NULL );
			if ( Res == NULL ) __leave;

			//
			// Call 'ReportTests' and try both 
			//  LogThreeMessagesAtPassiveLevel and 
			//  LogThreeMessagesAtDirql
			//
			Req.FixtureKey = 0;
			for ( Routine = 0; Routine <= 6; Routine++ )
			{
				Req.RoutineKey = Routine;
				Success = DeviceIoControl( 
					Dev,
					CFIXKR_IOCTL_CALL_ROUTINE,
					&Req,
					sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ),
					Res,
					ResponseSize,
					&Cb,
					NULL );
				CFIX_ASSERT( Success || GetLastError() == ERROR_MORE_DATA );
				
				if ( ! Success )
				{
					continue;
				}

				//
				// N.B. All test cases attempt to create 3 reports.
				//
				CFIX_ASSERT( Res->Events.Flags == CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED ||
					Res->Events.Count == 3 );

				if ( Res->Events.Flags == CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
				{
					SuccessfulAttemptsWithTrunc++;
					//wprintf( L"0x%8x: Trunc\n", ResponseSize );
				}
				else
				{
					SuccessfulAttempts++;
					//wprintf( L"0x%8x: Ok\n", ResponseSize );
				}
				
				//
				// Check events.
				//
				switch ( Routine )
				{
				case 0:
				case 1:
					CFIX_ASSERT( Res->RoutineRanToCompletion );
					CFIX_ASSERT( ! Res->AbortRun );

					CheckEventsForLog( Routine, Res );
					break;

				case 2:
					CFIX_ASSERT( ! Res->RoutineRanToCompletion );
					CFIX_ASSERT( ! Res->AbortRun );

					CheckEventsForInconclusivenessPassiveLevel(  Res );
					break;

				case 3:
					CFIX_ASSERT( ! Res->RoutineRanToCompletion );
					CFIX_ASSERT( ! Res->AbortRun );

					CheckEventsForInconclusivenessDirql( Res );
					break;

				case 4:
					CFIX_ASSERT( ! Res->RoutineRanToCompletion );
					CFIX_ASSERT( ! Res->AbortRun );

					CheckEventsForException( Res );
					break;

				case 5:
					CFIX_ASSERT( ! Res->RoutineRanToCompletion );
					CFIX_ASSERT( Res->AbortRun );

					CheckEventsForLogAndFailPassiveLevel( Res );
					break;

				case 6:
					CFIX_ASSERT( ! Res->RoutineRanToCompletion );
					CFIX_ASSERT( Res->AbortRun );

					CheckEventsForLogAndFailAtDirql( Res );
					break;
					
				default:
					CFIX_ASSERT( !"Invalid routine" );
				}
			}
			free( Res );
		}

		CFIX_ASSERT( SuccessfulAttemptsWithTrunc > 0 );
		CFIX_ASSERT( SuccessfulAttempts > 0 );

		//CFIX_LOG( L"SuccessfulAttemptsWithTrunc=%d", SuccessfulAttemptsWithTrunc );
		//CFIX_LOG( L"SuccessfulAttempts=%d", SuccessfulAttempts );
		
		CloseHandle( Dev );
	}
	__finally
	{
		UnloadDriver( L"testklib7" );
		UnloadDriver( L"cfixkr" );
	}
}


CFIX_BEGIN_FIXTURE( ReportTests )
	CFIX_FIXTURE_ENTRY( TestEventReporting )
CFIX_END_FIXTURE()
