/*----------------------------------------------------------------------
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * N.B. This file does not include any source code from WinUnit.
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

#include <WinUnit.h>
#include <Wincrypt.h>

BEGIN_TEST( TestIssue_345_AcceptPointerInAssertZero )
{
	PVOID PtrZero = NULL;
	
    WIN_ASSERT_ZERO( PtrZero );

	PVOID PtrNonZero = &PtrNonZero;
	WIN_ASSERT_NOT_ZERO( ULONG_MAX );
	WIN_ASSERT_NOT_ZERO( LONG_MIN );
    WIN_ASSERT_NOT_ZERO( PtrNonZero );
}
END_TEST

BEGIN_TEST( TestIssue_362_AssertNullOnConstTypedefStruct )
{
	PCCERT_CONTEXT pCert = NULL;
	WIN_ASSERT_NOT_NULL(pCert);
}
END_TEST