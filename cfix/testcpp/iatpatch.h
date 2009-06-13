#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		IAT patching routines
 *
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

/*++
	Routine description:
		Replace an entry in a given module's IAT

	Returns:
		The old function pointer

--*/
EXTERN_C HRESULT PatchIat(
	__in HMODULE Module,
	__in PSTR ImportedModuleName,
	__in PSTR ImportedProcName,
	__in PVOID AlternateProc,
	__out_opt PVOID *OldProc
	);