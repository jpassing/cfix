/*----------------------------------------------------------------------
 * Purpose:
 *		String formatting.
 *
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

#include <stdio.h>
#include "cdiagp.h"

/*++
	Routine Description:
		Like _wcsicmp, but only compares up to a maximum number 
		of characters only.
--*/
static INT CdiagsCompareString(
    __in PCWSTR Dst,
    __in PCWSTR Src,
	__in SIZE_T MaxCch
    )
{
    WCHAR DstChar;
	WCHAR SrcChar;

    do 
	{
        DstChar = towlower( *Dst );
        SrcChar = towlower( *Src );
        Dst++;
        Src++;
    } 
	while ( --MaxCch > 0 &&
			DstChar && 
			( DstChar == SrcChar ) );
    return ( INT ) ( DstChar - SrcChar );
}

/*++
	Routine Description:
		Get ordinal for given keyword. As the keyword table is assumed
		to be very small (<20 entries), seuqential serach is used.
--*/
static BOOL CdiagsGetVariableBindingIndex(
	__in ULONG VarCount,
	__in PFORMAT_VARIABLE Variables,
	__in PCWSTR VariableName,	// not null terminated!
	__in SIZE_T VariableNameCch,
	__out PINT BindingIndex
	)
{
	UINT Index;

	for ( Index = 0; Index < VarCount; Index++ )
	{
		if ( 0 == CdiagsCompareString( 
			Variables[ Index ].Name, 
			VariableName,
			VariableNameCch ) )
		{
			*BindingIndex = Index;
			return TRUE;
		}
	}

	return FALSE;
}

/*++
	Routine Description:
		Translate the word between VariableNameBegin and VariableNameEndExcl
		and write it to buffer with % prepended.

		No null termination is performed.
--*/
static HRESULT CdiagsHandleVariable(
	__in ULONG VarCount,
	__in PFORMAT_VARIABLE Variables,
	__in DWORD_PTR *Bindings,
	__in CONST WCHAR* VariableNameBegin,
	__in CONST WCHAR* VariableNameEndExcl,
	__in WCHAR** BufferPtr,
	__in SIZE_T BufferSizeInChars
	)
{
	INT BindingIndex;
	INT CharsWritten;
	BOOL Translated;

	Translated = CdiagsGetVariableBindingIndex( 
		VarCount,
		Variables,
		VariableNameBegin, 
		VariableNameEndExcl - VariableNameBegin,
		&BindingIndex );

	if ( ! Translated )
	{
		return S_OK;
	}

	//
	// We need _snwprintf, _snwprintf_s is not suitable here.
	//
#pragma warning( push )
#pragma warning( disable : 4995 4996 )
	CharsWritten = _snwprintf(
		*BufferPtr,
		BufferSizeInChars,
		Variables[ BindingIndex ].Format,
		Bindings[ BindingIndex ] );
#pragma warning( push )

	if ( CharsWritten >= 0 )
	{
		*BufferPtr += CharsWritten;
		return S_OK;
	}
	else
	{
		return CDIAG_E_BUFFER_TOO_SMALL;
	}
}

#ifdef _DEBUG
__declspec(dllexport)
#endif 
HRESULT CdiagpFormatString(
	__in ULONG VarCount,
	__in CONST PFORMAT_VARIABLE Variables,
	__in DWORD_PTR *Bindings,
	__in PCWSTR FormatStringTemplate,
	__in SIZE_T BufferSizeInChars,
	__out_ecount(BufferSizeInChars) PWSTR Buffer 
	)
{
	PCWSTR VariableName = NULL;
	CONST WCHAR *Cur = 0;
	WCHAR *BufferPtr;
	WCHAR *BufferEnd;
	HRESULT Hr;

	BOOL InVariable = FALSE;

	_ASSERTE( VarCount > 0 );
	_ASSERTE( Variables );
	_ASSERTE( Bindings );
	_ASSERTE( CdiagpIsStringValid( FormatStringTemplate, 1, MAXWORD, FALSE ) );
	_ASSERTE( BufferSizeInChars > 0 );
	_ASSERTE( Buffer );

	if ( VarCount == 0 ||
		 ! Variables ||
		 ! Bindings ||
		 ! CdiagpIsStringValid( FormatStringTemplate, 1, MAXWORD, FALSE ) ||
		 BufferSizeInChars == 0 ||
		 ! Buffer )
	{
		return E_INVALIDARG;
	}

	BufferPtr = &Buffer[ 0 ];
	BufferEnd = &Buffer[ BufferSizeInChars - 1 ];

	for ( Cur = FormatStringTemplate; *Cur != L'\0'; Cur++ )
	{
		if ( BufferPtr >= BufferEnd )
		{
			return CDIAG_E_BUFFER_TOO_SMALL;
		}

		if ( *Cur == L'%' )
		{
			if ( VariableName == Cur )
			{
				//
				// That was a %%.
				//
				InVariable = FALSE;
				*BufferPtr++ = *Cur;
			}
			else
			{
				InVariable = TRUE;

				if ( VariableName )
				{
					Hr = CdiagsHandleVariable(
						VarCount,
						Variables,
						Bindings,
						VariableName,
						Cur,
						&BufferPtr, 
						BufferEnd - BufferPtr );
					if ( FAILED( Hr ) )
					{
						return Hr;
					}
				}

				//
				// Save beginning of keyword.
				//
				VariableName = Cur + 1;
			}
		}
		else if ( InVariable && ! iswalnum( *Cur ) )
		{
			Hr = CdiagsHandleVariable(
				VarCount,
				Variables,
				Bindings,
				VariableName,
				Cur,
				&BufferPtr, 
				BufferEnd - BufferPtr );
			if ( FAILED( Hr ) )
			{
				return Hr;
			}
			
			InVariable = FALSE;
			VariableName = NULL;

			*BufferPtr++ = *Cur;
		}
		else if ( ! InVariable )
		{
			*BufferPtr++ = *Cur;
		}
	}

	if ( BufferPtr <= BufferEnd )
	{
		if ( InVariable )
		{
			Hr = CdiagsHandleVariable(
				VarCount,
				Variables,
				Bindings,
				VariableName,
				Cur,
				&BufferPtr, 
				BufferEnd - BufferPtr );
			if ( FAILED( Hr ) )
			{
				return Hr;
			}
		}
	
		*BufferPtr = L'\0';
	}
	else
	{
		return CDIAG_E_BUFFER_TOO_SMALL;
	}

	return S_OK;
}