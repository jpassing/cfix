/*----------------------------------------------------------------------
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ntddk.h>
#include <cfix.h>

static VOID LogThreeMessagesAtPassiveLevel()
{
	CFIX_ASSERT( "Truth" );

	CFIX_LOG( L"Log at %s", L"PASSIVE_LEVEL" );
	CFIX_LOG( L"Log at %s", L"PASSIVE_LEVEL" );
	CFIX_LOG( L"Log at %s", L"PASSIVE_LEVEL" );
}

static VOID LogThreeMessagesAtDirql()
{
	KIRQL Irql;
	KeRaiseIrql( DISPATCH_LEVEL + 1, &Irql );
	
	CFIX_ASSERT( "Truth" );

	CFIX_LOG( L"Log at %s", L"elevated irql" );
	CFIX_LOG( L"Log at %s", L"elevated irql" );
	CFIX_LOG( L"Log at %s", L"elevated irql" );

	KeLowerIrql( Irql );
}

static VOID LogAndInconclusiveAtPassiveLevel()
{
	CFIX_LOG( L"Log at %s", L"PASSIVE_LEVEL" );
	CFIX_LOG( L"Log at %s", L"PASSIVE_LEVEL" );
	CFIX_INCONCLUSIVE( L"This blah blah blah blah blah blah blah is inconclusive" );
	ASSERT( !"Should not make it here!" );
}

static VOID LogAndInconclusiveAtDirql()
{
	KIRQL Irql;
	KeRaiseIrql( DISPATCH_LEVEL + 1, &Irql );
	
	CFIX_ASSERT( "Truth" );

	CFIX_LOG( L"Log at %s", L"PASSIVE_LEVEL" );

	//
	// Note: message is of same length as IRQL-warning. Important
	// as both compete for the same buffer space.
	//
	CFIX_INCONCLUSIVE( L"Inconclusive   "
		L"01234567890123456789012345678901234567890123456789"
		L"01234567890123456789012345678901234567890123456789" );
	ASSERT( !"Should not make it here!" );
}

static VOID ThrowAtPassiveLevel()
{
	CFIX_ASSERT_EQUALS_ULONG( 1, 1 );
	CFIX_LOG( L"" );
	CFIX_LOG( L"" );

	//
	// N.B. ExRaiseStatus requires PASSIVE_LEVEL.
	//
	ExRaiseStatus( 'Excp' );
}

static VOID LogAndFailAtPassiveLevelAndAbort()
{
	CFIX_ASSERT( "Truth" );
	CFIX_LOG( L"" );
	CFIX_LOG( L"" );
	CFIX_ASSERT( !"Untruth" );
	ASSERT( !"Should not make it here!" );
}

static VOID LogAndFailAtDirqlAndAbort()
{
	KIRQL Irql;
	KeRaiseIrql( DISPATCH_LEVEL + 1, &Irql );
	CFIX_ASSERT( "Truth" );
	CFIX_LOG( L"" );
	CFIX_ASSERT_EQUALS_ULONG( 1, 2 );
	ASSERT( !"Should not make it here!" );
}

CFIX_BEGIN_FIXTURE( ReportTests )
	CFIX_FIXTURE_ENTRY(LogThreeMessagesAtPassiveLevel)
	CFIX_FIXTURE_ENTRY(LogThreeMessagesAtDirql)
	CFIX_FIXTURE_ENTRY(LogAndInconclusiveAtPassiveLevel)
	CFIX_FIXTURE_ENTRY(LogAndInconclusiveAtDirql)
	CFIX_FIXTURE_ENTRY(ThrowAtPassiveLevel)
	CFIX_FIXTURE_ENTRY(LogAndFailAtPassiveLevelAndAbort)
	CFIX_FIXTURE_ENTRY(LogAndFailAtDirqlAndAbort)
CFIX_END_FIXTURE()
