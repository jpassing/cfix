/*----------------------------------------------------------------------
 * Copyright:
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

static void TestFormattedAssertionAtPassiveLevel()
{
	CFIX_ASSERT_MESSAGE( TRUE, L"%d%s", 1, L"123" );
	CFIX_ASSERT_MESSAGE( TRUE, L"test" );
}

static void TestFormattedAssertionAtElevatedIrql()
{
	KIRQL OldIrql;
	KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );
	CFIX_ASSERT_MESSAGE( TRUE, L"%d%s", 1, L"123" );
	CFIX_ASSERT_MESSAGE( TRUE, L"test" );
	KeLowerIrql( OldIrql );
}

static void Nulls()
{
	CFIX_ASSERT_MESSAGE( TRUE, NULL );
	CFIX_LOG( NULL );
	//CFIX_INCONCLUSIVE( NULL );
}

void SucSetup()
{
	CFIX_LOG( L"In setup" );
}

void SucTeardown()
{
	CFIX_LOG( L"In teardown" );
}

void FailingTest()
{
	CFIX_LOG( L"Test" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

void FailingTestAtElevatedIrql()
{
	KIRQL OldIrql;
	KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );
	CFIX_LOG( L"Test" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
	KeLowerIrql( OldIrql );
}

CFIX_BEGIN_FIXTURE( FormattedAssertion )
	CFIX_FIXTURE_ENTRY( TestFormattedAssertionAtPassiveLevel )
	CFIX_FIXTURE_ENTRY( TestFormattedAssertionAtElevatedIrql )
	CFIX_FIXTURE_ENTRY( Nulls )
CFIX_END_FIXTURE()

//CFIX_BEGIN_FIXTURE( FailingTest )
//	CFIX_FIXTURE_SETUP( SucSetup )
//	CFIX_FIXTURE_ENTRY( FailingTestAtElevatedIrql )
//	CFIX_FIXTURE_TEARDOWN( SucTeardown )
//CFIX_END_FIXTURE()