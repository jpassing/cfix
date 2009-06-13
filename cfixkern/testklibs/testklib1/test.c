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

static void Succeed()
{
	KIRQL OldIrql;
	KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );
	CFIX_ASSERT( "Succeed" );
	KeLowerIrql( OldIrql );
}

static void Fail()
{
	KIRQL OldIrql;
	KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );
	CFIX_ASSERT( !"Fail" );
	KeLowerIrql( OldIrql );
}

static void Throw()
{
	ExRaiseAccessViolation();
}

CFIX_BEGIN_FIXTURE( KernelTest )
	CFIX_FIXTURE_ENTRY( Fail )
	CFIX_FIXTURE_ENTRY( Succeed )
//	CFIX_FIXTURE_ENTRY( Throw )
CFIX_END_FIXTURE()