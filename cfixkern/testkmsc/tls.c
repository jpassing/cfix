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

static ULONG SetupDummy1 = 1;
static ULONG SetupDummy2 = 2;
static ULONG BeforeDummy1 = 3;
static ULONG BeforeDummy2 = 4;

static void Setup()
{
	CFIX_ASSERT( CfixPeGetValue( 0 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );

	CfixPeSetValue( 0, &SetupDummy1 );
	CfixPeSetValue( 1, &SetupDummy1 );
}

static void Before()
{
	CFIX_ASSERT( CfixPeGetValue( 0 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );

	CfixPeSetValue( 0, &BeforeDummy1 );
	CfixPeSetValue( 1, &BeforeDummy1 );
}

static void After()
{
	CFIX_ASSERT( CfixPeGetValue( 0 ) == &BeforeDummy1 );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
}


static void TestTls()
{
	ULONG Dummy1 = 1;

	CFIX_ASSERT( CfixPeGetValue( 0 ) == &BeforeDummy1  );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );

	CfixPeSetValue( 0, &Dummy1 );
	CfixPeSetValue( 1, &Dummy1 );

	CFIX_ASSERT( CfixPeGetValue( 0 ) == &Dummy1 );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );
	
	CfixPeSetValue( 0, NULL );
	CfixPeSetValue( 1, NULL );

	CFIX_ASSERT( CfixPeGetValue( 0 ) == NULL );
	CFIX_ASSERT( CfixPeGetValue( 1 ) == NULL );

	CfixPeSetValue( 0, &BeforeDummy1 );
	CfixPeSetValue( 1, &BeforeDummy1 );
}

CFIX_BEGIN_FIXTURE( Tls )
	CFIX_FIXTURE_SETUP( Setup )
	CFIX_FIXTURE_BEFORE( Before )
	CFIX_FIXTURE_AFTER( After )
	CFIX_FIXTURE_TEARDOWN( Setup )
	CFIX_FIXTURE_ENTRY( TestTls )
CFIX_END_FIXTURE()