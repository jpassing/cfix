/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
 *
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

static void FailingSetup()
{
	CFIX_LOG( L"In setup" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

static void InconcSetup()
{
	CFIX_LOG( L"In setup" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

static void SucSetup()
{
	CFIX_LOG( L"In setup" );
}




static void FailingTeardown()
{
	CFIX_LOG( L"In teardown" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

static void InconcTeardown()
{
	CFIX_LOG( L"In teardown" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

static void SucTeardown()
{
	CFIX_LOG( L"In teardown" );
}


static void FailingTest()
{
	CFIX_LOG( L"Test" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

static void InconcTest()
{
	CFIX_LOG( L"Test" );
	CFIX_ASSERT( !"Fail" );
	CFIX_LOG( L"?" );
}

static void SucTest()
{
	CFIX_LOG( L"Test" );
}



static void NeverCallMe()
{
	RaiseException( 0xDEADBEEF, 0, 0, NULL );
}


//
// N.B. These fixture should be run and evaluated manually.
//

CFIX_BEGIN_FIXTURE( FailingSetup_MustAbortImmediately )
	CFIX_FIXTURE_SETUP( FailingSetup )
	CFIX_FIXTURE_ENTRY( NeverCallMe )
	CFIX_FIXTURE_TEARDOWN( NeverCallMe )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( InconcSetup_MustAbortImmediately )
	CFIX_FIXTURE_SETUP( InconcSetup )
	CFIX_FIXTURE_ENTRY( NeverCallMe )
	CFIX_FIXTURE_TEARDOWN( NeverCallMe )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( FailingTest_MustProceedWithSucTestAndTeardown )
	CFIX_FIXTURE_SETUP( SucSetup )
	CFIX_FIXTURE_ENTRY( FailingTest )
	CFIX_FIXTURE_ENTRY( SucTest )
	CFIX_FIXTURE_TEARDOWN( SucTeardown )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( InconcTest_MustCallTeardown )
	CFIX_FIXTURE_SETUP( SucSetup )
	CFIX_FIXTURE_ENTRY( InconcTest )
	CFIX_FIXTURE_TEARDOWN( SucTeardown )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( FailingTeardown_ResumeAtNextFixture )
	CFIX_FIXTURE_SETUP( SucSetup )
	CFIX_FIXTURE_ENTRY( SucTest )
	CFIX_FIXTURE_TEARDOWN( FailingTeardown )
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE( InconcTeardown_ResumeAtNextFixture )
	CFIX_FIXTURE_SETUP( SucSetup )
	CFIX_FIXTURE_ENTRY( SucTest )
	CFIX_FIXTURE_TEARDOWN( InconcTeardown )
CFIX_END_FIXTURE()