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
		'excp',
		0,
		0,
		NULL );

	return 0;
}



//
// This testcase must fail.
//
void AssertOnRegisteredThread()
{
	DWORD ExCode;
	HANDLE Thr = CfixCreateThread(
		NULL,
		0,
		AssertThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSERT( Thr );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ( ( HRESULT ) ExCode ) == CFIX_EXIT_THREAD_ABORTED );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

//
// This testcase must fail.
//
void LogOnRegisteredThread()
{
	DWORD ExCode;
	HANDLE Thr = CfixCreateThread(
		NULL,
		0,
		LogThreadProc,
		NULL,
		0,
		NULL );
	CFIX_ASSERT( Thr );

	WaitForSingleObject( Thr, INFINITE );
	
	CFIX_ASSERT( GetExitCodeThread( Thr, &ExCode ) );
	CFIX_ASSERT( ExCode == 0 );
	CFIX_ASSERT( ExCode == 0 );
	CFIX_ASSERT( CloseHandle( Thr ) );
}

//
// This testcase must fail.
//
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
	CFIX_ASSERT( Thr );

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
	CFIX_ASSERT( Thr );

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
