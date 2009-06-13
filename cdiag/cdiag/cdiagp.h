#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Internal utility routines
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

#include <cdiag.h>
#include <crtdbg.h>

#ifdef _DEBUG
#define _VERIFY _ASSERTE
#else
#define _VERIFY( x ) ( ( VOID ) ( x ) )
#endif

extern HMODULE CdiagpModule;

/*----------------------------------------------------------------------
 *
 * Memory allocation
 *
 */

/*++
	Routine Description:
		Allocate memory.

	Parameters:
		Size	Size of memory to allocate
		Zero	Zero out memory?

	Returns:
		Pointer to memory on success
		NULL on failure
--*/
PVOID CdiagpMalloc( 
	__in SIZE_T Size ,
	__in BOOL Zero
	);

/*++
	Routine Description:
		Free memory.

	Parameters:
		Ptr		Memory to free
--*/
VOID CdiagpFree( 
	__in PVOID Ptr
	);

/*----------------------------------------------------------------------
 *
 * String handling
 *
 */

/*++
	Routine description:
		Tests whether a string consists of whitespace exclusively.
--*/
BOOL CdiagpIsWhitespaceOnly(
	__in PCWSTR String
	);

/*++
	Routine description:
		Validates a string. Lengths are excluding null termination.

	Returns:
		TRUE if valid, FALSE otherwise.
--*/
BOOL CdiagpIsStringValid(
	__in PCWSTR String,
	__in SIZE_T MinLengthInclusive,
	__in SIZE_T MaxLengthInclusive,
	__in BOOL AllowWhitespaceOnly
	);

/*----------------------------------------------------------------------
 *
 * String formatting
 *
 */

typedef struct _FORMAT_VARIABLE
{
	//
	// Name of variable. Variable names are case-insensitive.
	//
	PWSTR Name;

	//
	// printf-style formatter. The formatter **MUST** be suitable
	// for the data passed to CdiagpFormatString.
	//
	PWSTR Format;
} FORMAT_VARIABLE, *PFORMAT_VARIABLE;

/*++
	Routine Description:
		Formats a string according to a template. The advantage of this
		function over *printf is that mnemonics can be used and 
		the template need not be trusted, i.e. it may be user (UI)-defined. 
		An invalid template cannot lead to a crash.
		
		The template may look as follows:
			'The %Var1-brown %Fox%% %Foo (%Bar)'
		The template may contain variables, which are prefixed by a
		'%' sign and may contain any alphanumeric characters.
		'%%' is the escape sequence for '%'.

		Given the variable definition and bindings, the buffer is 
		formatted according to the Template. Provided the variables
			Name: Var1	Format: %s
			Name: Var2	Format: %s
			Name: Foo	Format: %x
		and the bindings
			"quick"
			"brown"
			42
		the function yields the string
			'The quick-brown fox% 42 ()'
		Unknown variables (%Bar) evaluate to ''.

	Parameters:
		VarCount	- Number of variábles (=array size) referred to by 
					  both Variables and Bindings
		Variables	- Array of Variable definitions. Array must be of length
					  VarCount.
		Bindings	- Values. Array must be of length VarCount.
		Format		- The format template, see discussion above.
		BufferSizeInChars
		Buffer

	Return Value:
		S_OK on success.
		CDIAG_E_BUFFER_TOO_SMALL if the buffer is too small.
		(any HRESULT) on failure
--*/
#ifdef _DEBUG
__declspec(dllexport)
#endif 
HRESULT CdiagpFormatString(
	__in ULONG VarCount,
	__in CONST PFORMAT_VARIABLE Variables,
	__in DWORD_PTR *Bindings,
	__in PCWSTR Format,
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer 
	);


/*----------------------------------------------------------------------
 *
 * Initialization Routines.
 *
 */
VOID CdiagpInitializeFormatter();


/*----------------------------------------------------------------------
 *
 * Validation Routines.
 *
 */
#define CdiagpIsValidHandler( h ) \
	( ( ( h ) != NULL && ( ( PCDIAG_HANDLER ) ( h ) )->Size == sizeof( CDIAG_HANDLER ) ) )
