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

static ULONG SetupDummy1 = 1;
static ULONG SetupDummy2 = 2;
static ULONG BeforeDummy1 = 3;
static ULONG BeforeDummy2 = 4;

static void Setup()
{
	CFIX_ASSERT( CfixPeGetValue( 0 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) == NULL );

	CfixPeSetValue( 0, &SetupDummy1 );
	CfixPeSetValue( 1, &SetupDummy1 );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, &SetupDummy2 );
}

static void Before()
{
	CFIX_ASSERT( CfixPeGetValue( 0 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) == NULL );

	CfixPeSetValue( 0, &BeforeDummy1 );
	CfixPeSetValue( 1, &BeforeDummy1 );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, &BeforeDummy2 );
}

static void After()
{
	CFIX_ASSERT( CfixPeGetValue( 0 ) == &BeforeDummy1 );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) == &BeforeDummy2 );
}


static void TestTls()
{
	ULONG Dummy1 = 1;
	ULONG Dummy2 = 1;

	CFIX_ASSERT( CfixPeGetValue( 0 ) == &BeforeDummy1  );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) == &BeforeDummy2  );

	CfixPeSetValue( 0, &Dummy1 );
	CfixPeSetValue( 1, &Dummy1 );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, &Dummy2 );

	CFIX_ASSERT( CfixPeGetValue( 0 ) == &Dummy1 );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) == &Dummy2 );
	
	CfixPeSetValue( 0, NULL );
	CfixPeSetValue( 1, NULL );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, NULL );

	CFIX_ASSERT( CfixPeGetValue( 0 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( CFIX_TAG_RESERVED_FOR_CC ) == NULL );

	CfixPeSetValue( 0, &BeforeDummy1 );
	CfixPeSetValue( 1, &BeforeDummy1 );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, &BeforeDummy2 );
}

static DWORD CALLBACK ThreadProc( PVOID Pv )
{
	UNREFERENCED_PARAMETER( Pv );

	//
	// BABEFACE visible.
	//
	CFIX_ASSERT_EQUALS_DWORD( 
		0xBABEFACE, 
		( ULONG ) ( ULONG_PTR ) CfixPeGetValue( 0 ) );

	return 0;
}

static void TestTlsVisibleOnChildThread()
{
	HANDLE Thread;

	CfixPeSetValue( 0, ( PVOID ) ( ULONG_PTR ) 0xBABEFACE );

	Thread = CfixCreateThread2(
		NULL,
		0,
		ThreadProc,
		NULL,
		0,
		NULL,
		CFIX_THREAD_FLAG_CRT );

	CFIX_ASSERT( Thread );
	CFIX_ASSERT( WAIT_OBJECT_0 == WaitForSingleObject( Thread, INFINITE ) );
	CFIX_ASSERT( CloseHandle( Thread ) );

	CfixPeSetValue( 0, &BeforeDummy1 );
}

CFIX_BEGIN_FIXTURE( Tls )
	CFIX_FIXTURE_SETUP( Setup )
	CFIX_FIXTURE_BEFORE( Before )
	CFIX_FIXTURE_AFTER( After )
	CFIX_FIXTURE_TEARDOWN( Setup )
	CFIX_FIXTURE_ENTRY( TestTls )
	CFIX_FIXTURE_ENTRY( TestTlsVisibleOnChildThread )
CFIX_END_FIXTURE()