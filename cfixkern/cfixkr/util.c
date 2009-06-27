/*----------------------------------------------------------------------
 * Purpose:
 *		Utility routines. 
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
#include <ntddk.h>
#include <aux_klib.h>

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

NTSTATUS CfixkrpGetModuleBaseAddress(
	__in ULONG_PTR AddressWithinModule,
	__out ULONG_PTR *BaseAddress
	)
{
	ULONG BufferSize = 0;
	PAUX_MODULE_EXTENDED_INFO Modules;
	NTSTATUS Status;

	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );
	ASSERT( BaseAddress );

	*BaseAddress = 0;

	//
	// There is no API for querying the current module's load address.
	// This routine calculates the address by traversing the list
	// of all loaded kernel modules.
	//

	//
	// Query required size.
	//
	Status = AuxKlibQueryModuleInformation (
		&BufferSize,
		sizeof( AUX_MODULE_EXTENDED_INFO ),
		NULL );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	ASSERT( ( BufferSize % sizeof( AUX_MODULE_EXTENDED_INFO ) ) == 0 );

	Modules = ( PAUX_MODULE_EXTENDED_INFO )
		ExAllocatePoolWithTag( PagedPool, BufferSize, CFIXKR_POOL_TAG );
	if ( ! Modules )
	{
		return STATUS_NO_MEMORY;
	}

	RtlZeroMemory( Modules, BufferSize );

	//
	// Query loaded modules list.
	//
	Status = AuxKlibQueryModuleInformation(
		&BufferSize,
		sizeof( AUX_MODULE_EXTENDED_INFO ),
		Modules );
	if ( NT_SUCCESS( Status ) )
	{
		ULONG Index;
		ULONG NumberOfModules = BufferSize / sizeof( AUX_MODULE_EXTENDED_INFO );

		//
		// Now that we have the module list, see which one we are.
		//
		for ( Index = 0; Index < NumberOfModules; Index++ )
		{
			ULONG_PTR ImageBase = 
				( ULONG_PTR ) Modules[ Index ].BasicInfo.ImageBase;

			if ( AddressWithinModule >= ImageBase &&
				 AddressWithinModule <  ImageBase + Modules[ Index ].ImageSize )
			{
				*BaseAddress = ImageBase;
				break;
			}
		}
	}

	ExFreePoolWithTag( Modules, CFIXKR_POOL_TAG );

	ASSERT( *BaseAddress );
	if ( *BaseAddress == 0 )
	{
		KdPrint( ( "CFIXDRV: Failed to obtain own module base address.\n" ) );
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint( ( "CFIXDRV: Own module base address is %p.\n", *BaseAddress ) );

	return Status;
}

ULONG CfixkrGetCurrentThreadId()
{
	return ( ULONG ) ( ULONG_PTR ) PsGetCurrentThreadId();
}