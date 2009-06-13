#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Cfix main header file.
 *
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

#include <cfixapi.h>
#include <cfixpe.h>
#include <crtdbg.h>

#define ASSERT _ASSERTE

#ifdef DBG
#define VERIFY ASSERT
#else
#define VERIFY( x ) ( ( VOID ) ( x ) )
#endif

#define NOP ( ( VOID ) 0 )

/*++
	Routines to be called from DllMain.
--*/
BOOL CfixpSetupTls();
BOOL CfixpTeardownTls();