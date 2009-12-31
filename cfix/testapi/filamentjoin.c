/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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
#include <cfixmsg.h>

static DWORD CALLBACK ThreadProc( PVOID Pv )
{
	UNREFERENCED_PARAMETER( Pv );

	Sleep( 1000 );

	return 0;
}

static void SpawnAndJoinPolitely()
{
	HANDLE Thread = CfixCreateThread2(
		NULL,
		0,
		ThreadProc,
		NULL,
		0,
		NULL,
		CFIX_THREAD_FLAG_CRT );
	CFIX_ASSUME( Thread != NULL );

	CFIX_ASSERT( WAIT_OBJECT_0 == WaitForSingleObject( Thread, INFINITE ) );

	CFIX_ASSERT( CloseHandle( Thread ) );
}

static void SpawnAndAutoJoin()
{
	CFIX_ASSERT( CloseHandle( CfixCreateThread2(
		NULL,
		0,
		ThreadProc,
		NULL,
		0,
		NULL,
		CFIX_THREAD_FLAG_CRT ) ) );
}

static void SpawnAndAutoJoinLotsOfThreads()
{
	ULONG Index;

	for ( Index = 0; Index < CFIX_MAX_THREADS; Index++ )
	{
		SpawnAndAutoJoin();
	}

	CFIX_ASSERT( ! CfixCreateThread2(
			NULL,
			0,
			ThreadProc,
			NULL,
			0,
			NULL,
			CFIX_THREAD_FLAG_CRT ) );
	CFIX_ASSERT( GetLastError() == ( DWORD ) CFIX_E_TOO_MANY_CHILD_THREADS );
}

static void CfixRegisterThreadFailsWhenAutoRegisteringDisabled()
{
	CFIX_ASSERT_HRESULT( E_UNEXPECTED, CfixRegisterThread( NULL ) );
}

CFIX_BEGIN_FIXTURE( FilamentJoin )
	CFIX_FIXTURE_ENTRY( SpawnAndJoinPolitely )
	CFIX_FIXTURE_ENTRY( SpawnAndAutoJoin )
	CFIX_FIXTURE_ENTRY( SpawnAndAutoJoinLotsOfThreads )

	//
	// Threads have to work in all these situations, too.
	//
	CFIX_FIXTURE_SETUP( SpawnAndAutoJoin )
	CFIX_FIXTURE_TEARDOWN( SpawnAndAutoJoin )
	CFIX_FIXTURE_BEFORE( SpawnAndAutoJoin )
	CFIX_FIXTURE_AFTER( SpawnAndAutoJoin )

	CFIX_FIXTURE_ENTRY( CfixRegisterThreadFailsWhenAutoRegisteringDisabled )
CFIX_END_FIXTURE()