/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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

#include <cfix.h>
#include <stdio.h>

#define TEST CFIX_ASSERT

static VOID Setup()
{
}

static VOID Teardown()
{
}

static VOID FailTestWithLog()
{
	CFIX_LOG( L"log" );
	TEST( !"I fail" );

	if ( ! IsDebuggerPresent() )
	{
		// Should not make it here.
		DebugBreak();
	}
	else
	{
		CFIX_LOG( L"log2" );
	}
}

static VOID FailTestWithLogMayContinue()
{
	CFIX_LOG( L"log" );
	TEST( !"I fail" );
	CFIX_LOG( L"log2" );
}

static VOID InconclusiveTest()
{
	CFIX_INCONCLUSIVE( L"Dunno" );

	// Should not make it here.
	DebugBreak();
}

static VOID SuccTestWithLog()
{
	CFIX_LOG( L"%s with %x", L"log", 0xbabe );
}

static VOID Throw()
{
	RaiseException(
		'excp',
		0,
		0,
		NULL );
}

static VOID ValidEquals()
{
	CFIX_ASSERT_EQUALS_DWORD( 0, 0 );
	CFIX_ASSERT_EQUALS_DWORD( 0xF00D, 0xF00D );
}

static VOID InvalidEquals()
{
	SetLastError( ERROR_FILE_NOT_ENCRYPTED );

	CFIX_ASSERT_EQUALS_DWORD( ERROR_FILE_INVALID, GetLastError() );
}

static VOID FailSetup()
{
	CFIX_ASSERT( !"Fail" );
}

static VOID DoNotCallMeTeardown()
{
	CFIX_ASSERT( !"This routine must not be called as setup failed" );
}


CFIX_BEGIN_FIXTURE(JustSetupAndTearDown)
	CFIX_FIXTURE_TEARDOWN(Teardown)
	CFIX_FIXTURE_SETUP(Setup)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(TestValidEquals)
	CFIX_FIXTURE_ENTRY(ValidEquals)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(TestInvalidEquals)
	CFIX_FIXTURE_ENTRY(InvalidEquals)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(FailSetupDoNotCallTeardown)
	CFIX_FIXTURE_SETUP(FailSetup)
	CFIX_FIXTURE_TEARDOWN(DoNotCallMeTeardown)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(FailTeardown)
	CFIX_FIXTURE_TEARDOWN(Throw)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(SetupTwoSuccTestsAndTearDown)
	CFIX_FIXTURE_TEARDOWN(Teardown)
	CFIX_FIXTURE_ENTRY(SuccTestWithLog)
	CFIX_FIXTURE_ENTRY(SuccTestWithLog)
	CFIX_FIXTURE_SETUP(Setup)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(SetupSucFailInconThrowTearDown)
	CFIX_FIXTURE_TEARDOWN(Teardown)
	CFIX_FIXTURE_ENTRY(SuccTestWithLog)
	CFIX_FIXTURE_ENTRY(InconclusiveTest)
	CFIX_FIXTURE_ENTRY(FailTestWithLogMayContinue)
	CFIX_FIXTURE_ENTRY(Throw)
	CFIX_FIXTURE_SETUP(Setup)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(AbortMeBecauseOfUnhandledExcp)
	CFIX_FIXTURE_ENTRY(Throw)
CFIX_END_FIXTURE()

CFIX_BEGIN_FIXTURE(AbortMeBecauseOfFailure)
	CFIX_FIXTURE_ENTRY(FailTestWithLog)
CFIX_END_FIXTURE()