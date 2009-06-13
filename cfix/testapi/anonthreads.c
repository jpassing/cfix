/*----------------------------------------------------------------------
 * Purpose:
 *		Test that unregistered child threads are aborted.
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
	CFIX_ASSERT( !"Dead statement" );

	return 0;
}

DWORD LogThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	CFIX_LOG( L"Log on another thread" );

	return 0;
}

DWORD InconclusiveThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	CFIX_INCONCLUSIVE( L"Inconclusive on another thread" );

	CFIX_ASSERT( !"Dead statement" );

	return 0;
}

DWORD ThrowThreadProc( PVOID Unused )
{
	UNREFERENCED_PARAMETER( Unused );

	RaiseException(
		EXCEPTION_FLT_DIVIDE_BY_ZERO,
		0,
		0,
		NULL );

	return 0;
}

void AssertOnAnonThread()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		AssertThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSERT( Thr );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( CFIX_EXIT_THREAD_ABORTED == ExCode );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void LogOnAnonThread()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		LogThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSERT( Thr );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( CFIX_EXIT_THREAD_ABORTED == ExCode );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

void InconclusiveOnAnonThread()
{
	DWORD ExCode;
	HANDLE Thr = CreateThread(
		NULL,
		0,
		InconclusiveThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSERT( Thr );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( CFIX_EXIT_THREAD_ABORTED == ExCode );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

CFIX_BEGIN_FIXTURE(AnonymousThreads)
	CFIX_FIXTURE_ENTRY(AssertOnAnonThread)
	CFIX_FIXTURE_ENTRY(LogOnAnonThread)
	CFIX_FIXTURE_ENTRY(InconclusiveOnAnonThread)
CFIX_END_FIXTURE()
