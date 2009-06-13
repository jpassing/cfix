/*----------------------------------------------------------------------
 * Purpose:
 *		Utility routines. 
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

#include <wdm.h>

#include "cfixkrp.h"

NTSTATUS CfixkrpCompleteRequest(
	__in PIRP Irp,
	__in NTSTATUS Status,
	__in ULONG_PTR Information,
	__in CCHAR PriorityBoost
	)
{
	ASSERT( NULL == Irp->CancelRoutine );
	
	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;
	
	IoCompleteRequest( Irp, PriorityBoost );

	return Status;
}

PVOID CfixkrpAllocatePagedHashtableMemory(
	__in SIZE_T Size 
	)
{
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	return ExAllocatePoolWithTag( PagedPool, Size, CFIXKR_POOL_TAG );
}

PVOID CfixkrpAllocateNonpagedHashtableMemory(
	__in SIZE_T Size 
	)
{
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	return ExAllocatePoolWithTag( NonPagedPool, Size, CFIXKR_POOL_TAG );
}

VOID CfixkrpFreeHashtableMemory(
	__in PVOID Mem
	)
{
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ExFreePoolWithTag( Mem, CFIXKR_POOL_TAG );
}
