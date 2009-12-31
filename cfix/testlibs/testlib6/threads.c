/*----------------------------------------------------------------------
 * Purpose:
 *		Tests that threads which have been registered are aborted
 *		as soon as a failure occurs AND the testcase is considered
 *		to have failed.
 *
 * Copyright:
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
#include <stdio.h>
#include <crtdbg.h>

DWORD AssertThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	//
	// Should break here!
	//
	CFIX_ASSERT( !"Assert on another thread" );

	//
	// Should break here!
	//
	_ASSERTE( !"Dead statement" );

	return 0;
}

DWORD RegisterAndAssertThreadProc( __in PVOID EventHandle )
{
	CFIX_ASSERT_OK( CfixRegisterThread( NULL ) );

	SetEvent( ( HANDLE ) EventHandle );

	//
	// Should break here!
	//
	CFIX_ASSERT( !"Assert on another thread" );

	//
	// Should break here!
	//
	_ASSERTE( !"Dead statement" );

	return 0;
}

DWORD LogThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	CFIX_LOG( L"Log 1 on another thread" );
	CFIX_LOG( L"Log 2 on another thread" );

	return 0;
}

DWORD RegisterAndInconclusiveThreadProc(  __in PVOID EventHandle )
{
	CFIX_ASSERT_OK( CfixRegisterThread( NULL ) );

	SetEvent( ( HANDLE ) EventHandle );

	CFIX_INCONCLUSIVE( L"Inconclusive on another thread" );

	_ASSERTE( !"Dead statement" );

	return 0;
}

DWORD InconclusiveThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	CFIX_INCONCLUSIVE( L"Inconclusive on another thread" );

	_ASSERTE( !"Dead statement" );

	return 0;
}

DWORD ThrowThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	RaiseException(
		'excp',
		0,
		0,
		NULL );

	_ASSERTE( !"Dead statement" );

	return 0;
}

/*----------------------------------------------------------------------
 * Using registered threads.
 */

void AssertOnRegisteredThread()
{
	DWORD ExCode;
	HANDLE Thr = CfixCreateThread2(
		NULL,
		0,
		AssertThreadProc,
		NULL,
		0,
		NULL,
		CFIX_THREAD_FLAG_CRT + 1 );
	CFIX_ASSERT( Thr == NULL );

	Thr = CfixCreateThread2(
		NULL,
		0,
		AssertThreadProc,
		NULL,
		0,
		NULL,
		CFIX_THREAD_FLAG_CRT );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ( ( HRESULT ) ExCode ) == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void LogOnRegisteredThread()
{
	DWORD ExCode;
	HANDLE Thr = CfixCreateThread2(
		NULL,
		0,
		LogThreadProc,
		NULL,
		0,
		NULL,
		CFIX_THREAD_FLAG_CRT );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == 0 );
	CFIX_ASSERT( ExCode == 0 );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void InconclusiveOnRegisteredThread()
{
	DWORD ExCode;
	HANDLE Thr = CfixCreateThread(
		NULL,
		0,
		InconclusiveThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void ThrowOnRegisteredThread()
{
	DWORD ExCode;
	HANDLE Thr = CfixCreateThread(
		NULL,
		0,
		ThrowThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

CFIX_BEGIN_FIXTURE(AssertOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(AssertOnRegisteredThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(LogOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(LogOnRegisteredThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(InconclusiveOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(InconclusiveOnRegisteredThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(ThrowOnRegisteredThread)
	CFIX_FIXTURE_ENTRY(ThrowOnRegisteredThread)
CFIX_END_FIXTURE()

/*----------------------------------------------------------------------
 * Using unregistered threads.
 */

void AssertOnAnonymousThread()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		AssertThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ( ( HRESULT ) ExCode ) == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void AssertOnAnonymousThreadWithoutJoin()
{
	HANDLE RegisteredEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	HANDLE Thr = CreateThread(
		NULL,
		0,
		RegisterAndAssertThreadProc,
		RegisteredEvent,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	//
	// Wait for event to ensure that CfixRegisterThread has been
	// called.
	//
	WaitForSingleObject( RegisteredEvent, INFINITE );

	CFIX_ASSERT( CloseHandle( Thr ) );
	CFIX_ASSERT( CloseHandle( RegisteredEvent ) );
}

void LogOnAnonymousThread()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		LogThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == 0 );
	CFIX_ASSERT( ExCode == 0 );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void InconclusiveOnAnonymousThread()
{
	HANDLE RegisteredEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	HANDLE Thr = CreateThread(
		NULL,
		0,
		RegisterAndInconclusiveThreadProc,
		RegisteredEvent,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	//
	// Wait for event to ensure that CfixRegisterThread has been
	// called.
	//
	WaitForSingleObject( RegisteredEvent, INFINITE );

	CFIX_ASSERT( CloseHandle( Thr ) );
	CFIX_ASSERT( CloseHandle( RegisteredEvent ) );
}

void InconclusiveOnAnonymousThreadWithoutJoin()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		InconclusiveThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void ThrowOnAnonymousThread()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		ThrowThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSUME( Thr != NULL );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

CFIX_BEGIN_FIXTURE(AssertOnAnonymousThread)
	CFIX_FIXTURE_ENTRY(AssertOnAnonymousThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE_EX(AssertOnAnonymousThreadWithAnonThreadSupport, CFIX_FIXTURE_USES_ANONYMOUS_THREADS)
	CFIX_FIXTURE_ENTRY(AssertOnAnonymousThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE_EX(AssertOnAnonymousThreadWithoutJoin, CFIX_FIXTURE_USES_ANONYMOUS_THREADS)
	CFIX_FIXTURE_ENTRY(AssertOnAnonymousThreadWithoutJoin)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(LogOnAnonymousThread)
	CFIX_FIXTURE_ENTRY(LogOnAnonymousThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE_EX(LogOnAnonymousThreadWithAnonThreadSupport, CFIX_FIXTURE_USES_ANONYMOUS_THREADS)
	CFIX_FIXTURE_ENTRY(LogOnAnonymousThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(InconclusiveOnAnonymousThread)
	CFIX_FIXTURE_ENTRY(InconclusiveOnAnonymousThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE_EX(InconclusiveOnAnonymousThreadWithAnonThreadSupport, CFIX_FIXTURE_USES_ANONYMOUS_THREADS)
	CFIX_FIXTURE_ENTRY(InconclusiveOnAnonymousThread)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE_EX(InconclusiveOnAnonymousThreadWithoutJoin, CFIX_FIXTURE_USES_ANONYMOUS_THREADS)
	CFIX_FIXTURE_ENTRY(InconclusiveOnAnonymousThreadWithoutJoin)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(ThrowOnAnonymousThread)
	CFIX_FIXTURE_ENTRY(ThrowOnAnonymousThread)
CFIX_END_FIXTURE()
