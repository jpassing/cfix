/*----------------------------------------------------------------------
 * Purpose:
 *		Code embedded into each test driver.
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
#include <cfixkr.h>
#include <stdarg.h>         // for va_start, etc.
#include <aux_klib.h>

#define CFIXKRDRV_POOL_TAG 'DifC'

//
// This (referenced) object is used to keep cfixkr loaded as long
// as we use its interface.
//
static PDEVICE_OBJECT CfixkDrvCfixkrDevice = NULL;
static CFIXKR_REPORT_SINK_INTERFACE CfixkDrvReportSink;

static NTSTATUS CfixkDrvNotImplemented(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp 
	)
{
	UNREFERENCED_PARAMETER( DeviceObject );
	UNREFERENCED_PARAMETER( Irp );
	
	return STATUS_NOT_IMPLEMENTED;
}

static VOID CfixkDrvUnload(
	__in PDRIVER_OBJECT DriverObject
	)
{
	UNREFERENCED_PARAMETER( DriverObject );

	if ( CfixkDrvCfixkrDevice != NULL )
	{
		CfixkDrvReportSink.Base.InterfaceDereference(
			CfixkDrvReportSink.Base.Context );
		
		RtlZeroMemory( &CfixkDrvReportSink, sizeof( CFIXKR_REPORT_SINK_INTERFACE ) );

		ObDereferenceObject( CfixkDrvCfixkrDevice );
	}
}

/*++
	Routine Description:
		There is no API for querying the current module's load address.
		This routine calculates the address by traversing the list
		of all loaded kernel modules.
--*/
static NTSTATUS CfixkDrvGetCurrentModuleBaseAddress(
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
		ExAllocatePoolWithTag( PagedPool, BufferSize, CFIXKRDRV_POOL_TAG );
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
		ULONG_PTR AddressWithinCurrentModule; 
		ULONG Index;
		ULONG NumberOfModules = BufferSize / sizeof( AUX_MODULE_EXTENDED_INFO );

		#pragma warning( push )
		#pragma warning( disable : 4054 )
		AddressWithinCurrentModule = 
			( ULONG_PTR ) ( PVOID ) CfixkDrvGetCurrentModuleBaseAddress;
		#pragma warning( pop )

		//
		// Now that we have the module list, see which one we are.
		//
		for ( Index = 0; Index < NumberOfModules; Index++ )
		{
			ULONG_PTR ImageBase = 
				( ULONG_PTR ) Modules[ Index ].BasicInfo.ImageBase;

			if ( AddressWithinCurrentModule >= ImageBase &&
				 AddressWithinCurrentModule <  ImageBase + Modules[ Index ].ImageSize )
			{
				*BaseAddress = ImageBase;
				break;
			}
		}
	}

	ExFreePoolWithTag( Modules, CFIXKRDRV_POOL_TAG );

	ASSERT( *BaseAddress );
	if ( *BaseAddress == 0 )
	{
		KdPrint( ( "CFIXDRV: Failed to obtain own module base address.\n" ) );
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint( ( "CFIXDRV: Own module base address is %p.\n", *BaseAddress ) );

	return Status;
}

static NTSTATUS CfixkDrvQueryReporter( 
	__in PDEVICE_OBJECT CfixkrDevObject 
	)
{
	KEVENT Event;
	IO_STATUS_BLOCK IoStatusBlock;
	PIRP Irp;
	ULONG_PTR LoadAddress;
	PIO_STACK_LOCATION Stack;
	NTSTATUS Status;

	Status = CfixkDrvGetCurrentModuleBaseAddress( &LoadAddress );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	KeInitializeEvent( &Event, NotificationEvent, FALSE );
	
	Irp = IoBuildSynchronousFsdRequest(
		IRP_MJ_PNP,
		CfixkrDevObject,
		NULL,
		0,
		NULL,
		&Event,
		&IoStatusBlock );

	if ( NULL == Irp )
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	Irp->RequestorMode = KernelMode;
	Stack = IoGetNextIrpStackLocation(Irp);

	//
	// Create an interface query out of the Irp.
	//
	Stack->MinorFunction							= IRP_MN_QUERY_INTERFACE;
	Stack->Parameters.QueryInterface.InterfaceType	= &GUID_CFIXKR_REPORT_SINK;
	Stack->Parameters.QueryInterface.Size			= sizeof( CFIXKR_REPORT_SINK_INTERFACE ) ;
	Stack->Parameters.QueryInterface.Version		= CFIXKR_REPORT_SINK_VERSION;
	Stack->Parameters.QueryInterface.Interface		= ( PINTERFACE ) &CfixkDrvReportSink;
	Stack->Parameters.QueryInterface.InterfaceSpecificData = ( PVOID ) LoadAddress;

	Status = IoCallDriver( CfixkrDevObject, Irp );
	if ( STATUS_PENDING == Status )
	{
		KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
		Status = IoStatusBlock.Status;
	}

	ASSERT( NT_SUCCESS( Status ) == ( CfixkDrvReportSink.Base.Context != NULL ) );

	return Status;
}

NTSTATUS DriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath
	)
{
	UNICODE_STRING CfixkrName = RTL_CONSTANT_STRING( CFIXKR_DEVICE_NT_NAME );
	PFILE_OBJECT File;
	PDEVICE_OBJECT CfixkrObject;
	NTSTATUS Status;

	UNREFERENCED_PARAMETER( RegistryPath );

	KdPrint( ( "CFIXDRV: DriverEntry of test driver\n" ) );

	//
	// We'll need AuxKlib.
	//
	Status = AuxKlibInitialize();
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}
	
	//
	// Install dispatch routines.
	//
	DriverObject->MajorFunction[ IRP_MJ_CREATE ]	= CfixkDrvNotImplemented;
	DriverObject->MajorFunction[ IRP_MJ_CLOSE ]		= CfixkDrvNotImplemented;
	DriverObject->DriverUnload						= CfixkDrvUnload;

	//
	// Lookup cfixkr and obtain interface.
	//
	Status = IoGetDeviceObjectPointer(
		&CfixkrName,
		FILE_WRITE_DATA,
		&File,
		&CfixkrObject );
	if ( ! NT_SUCCESS( Status ) )
	{
		KdPrint( ( "CFIX: Failed to obtain pointer to cfixkr device\n" ) );
		return Status;
	}

	Status = CfixkDrvQueryReporter( CfixkrObject );
	if ( NT_SUCCESS( Status ) )
	{
		//
		// Keep a reference to the device to lock the driver in
		// memory until we have released the interface.
		//
		ObReferenceObject( CfixkrObject );
		CfixkDrvCfixkrDevice = CfixkrObject;
	}

	//
	// We have obtained the interface, which is referenced. We can thus
	// release File while having locked the DO through the interface.
	//
	ObDereferenceObject( File );

	return Status; 
}

/*----------------------------------------------------------------------
 *
 * Report stubs.
 *
 */

CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeReportFailedAssertion(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression
	)
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		return CfixkDrvReportSink.Methods.ReportFailedAssertion(
			CfixkDrvReportSink.Base.Context,
			File,
			Routine,
			Line,
			Expression );
	}
	else
	{
		return CfixAbort;
	}
}

CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeAssertEqualsUlong(
	__in ULONG Expected,
	__in ULONG Actual,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Expression,
	__reserved ULONG Reserved
	)
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		return CfixkDrvReportSink.Methods.AssertEqualsUlong(
			CfixkDrvReportSink.Base.Context,
			Expected,
			Actual,
			File,
			Routine,
			Line,
			Expression,
			Reserved );
	}
	else
	{
		return CfixAbort;
	}
}

VOID CFIXCALLTYPE CfixPeReportInconclusiveness(
	__in PCWSTR Message
	)
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		CfixkDrvReportSink.Methods.ReportInconclusiveness(
			CfixkDrvReportSink.Base.Context,
			Message );
	}
}

VOID __cdecl CfixPeReportLog(
	__in PCWSTR Format,
	...
	)
{
	va_list VarArgs;

	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		va_start( VarArgs, Format );
		CfixkDrvReportSink.Methods.ReportLog(
			CfixkDrvReportSink.Base.Context,
			Format,
			VarArgs );
		va_end( VarArgs );
	}
}