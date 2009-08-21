/*----------------------------------------------------------------------
 * Purpose:
 *		Search for Test-DLLs.
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
#include <shlwapi.h>
#include "cfixrunp.h"

typedef struct _CFIXRUNP_SEARCH_CONTEXT
{
	CFIXRUN_VISIT_DLL_ROUTINE Callback;
	PVOID CallbackContext;
	BOOL IncludeDrivers;
} CFIXRUNP_SEARCH_CONTEXT, *PCFIXRUNP_SEARCH_CONTEXT;

static HRESULT CfixrunsVisit(
	__in PCWSTR Path,
	__in CFIXUTIL_VISIT_TYPE Type,
	__in_opt PVOID Context,
	__in BOOL SearchPerformed
	)
{
	PCFIXRUNP_SEARCH_CONTEXT SearchContext = 
		( PCFIXRUNP_SEARCH_CONTEXT ) Context;

	ASSERT( SearchContext != NULL );
	if ( SearchContext == NULL )
	{
		return E_INVALIDARG;
	}

	if ( Type != CfixutilFile )
	{
		return S_OK;
	}

	if ( CfixrunpIsDll( Path ) )
	{
		return ( SearchContext->Callback ) ( 
			Path, 
			CfixrunDll, 
			SearchContext->CallbackContext, 
			SearchPerformed );
	}
	else if ( SearchContext->IncludeDrivers && CfixrunpIsSys( Path ) )
	{
		return ( SearchContext->Callback ) ( 
			Path, 
			CfixrunSys, 
			SearchContext->CallbackContext, 
			SearchPerformed );
	}
	else
	{
		return S_OK;
	}
}

BOOL CfixrunpIsDll(
	__in PCWSTR Path
	)
{
	size_t Len = wcslen( Path );
	return ( Len > 4 && 0 == _wcsicmp( Path + Len - 4, L".dll" ) );
}

BOOL CfixrunpIsSys(
	__in PCWSTR Path
	)
{
	size_t Len = wcslen( Path );
	return ( Len > 4 && 0 == _wcsicmp( Path + Len - 4, L".sys" ) );
}

BOOL CfixrunpIsExe(
	__in PCWSTR Path
	)
{
	size_t Len = wcslen( Path );
	return ( Len > 4 && 0 == _wcsicmp( Path + Len - 4, L".exe" ) );
}

HRESULT CfixrunSearchModules(
	__in PCWSTR Path,
	__in BOOL Recursive,
	__in BOOL IncludeDrivers,
	__in CFIXRUN_VISIT_DLL_ROUTINE Routine,
	__in_opt PVOID Context
	)
{
	CFIXRUNP_SEARCH_CONTEXT SearchContext;

	if ( ! Path || ! Routine )
	{
		return E_INVALIDARG;
	}

	SearchContext.Callback			= Routine;
	SearchContext.CallbackContext	= Context;
	SearchContext.IncludeDrivers	= IncludeDrivers;

	return CfixutilSearch(
		Path,
		Recursive,
		CfixrunsVisit,
		&SearchContext );
}