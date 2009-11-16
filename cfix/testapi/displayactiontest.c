/*----------------------------------------------------------------------
 * Purpose:
 *		Test DisplayAction.
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
#include "test.h"
#include <cfixrunp.h>
#include <Shellapi.h>

void TestDisplayAction()
{
	PCFIX_ACTION Action;
	PCFIX_TEST_MODULE Mod;
	WCHAR Path[ MAX_PATH ];

	CFIXRUN_OPTIONS Options;
	CFIXRUN_OUTPUT_TARGET DefOutputProgress;

	ZeroMemory( &Options, sizeof( CFIXRUN_OPTIONS ) );
	Options.PrintConsole = wprintf;

	if ( IsDebuggerPresent() )
	{
		DefOutputProgress = CfixrunTargetDebug;
	}
	else
	{
		DefOutputProgress = CfixrunTargetConsole;
	}

	TEST( GetModuleFileName( ModuleHandle, Path, _countof( Path ) ) );
	TEST( PathRemoveFileSpec( Path ) );
	TEST( PathAppend( Path, L"testlib6.dll" ) );

	TEST_HR( CfixCreateTestModuleFromPeImage( Path, &Mod ) );

	TEST_HR( CfixrunpCreateDisplayAction(
		Mod->Fixtures[ 0 ],
		&Options,
		&Action ) );

	Action->Run( Action, NULL );
	Action->Dereference( Action );
	Mod->Routines.Dereference( Mod );
}

CFIX_BEGIN_FIXTURE( DisplayAction )
	CFIX_FIXTURE_ENTRY( TestDisplayAction )
CFIX_END_FIXTURE()