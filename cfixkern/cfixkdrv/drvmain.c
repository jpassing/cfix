/*----------------------------------------------------------------------
 * Purpose:
 *		Code embedded into each test driver.
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

#include <wdm.h>
#include <cfixkr.h>
#include <stdarg.h>         // for va_start, etc.

#define CFIXKRDRV_POOL_TAG 'DifC'

//
// This (referenced) object is used to keep cfixkr loaded as long
// as we use its interface.
//
static PDEVICE_OBJECT CfixkDrvCfixkrDevice = NULL;
static CFIXKR_REPORT_SINK_INTERFACE_3 CfixkDrvReportSink;

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

static NTSTATUS CfixkDrvQueryReporter( 
	__in PDEVICE_OBJECT CfixkrDevObject 
	)
{
	ULONG_PTR AddressWithinCurrentModule; 
	KEVENT Event;
	IO_STATUS_BLOCK IoStatusBlock;
	PIRP Irp;
	PIO_STACK_LOCATION Stack;
	NTSTATUS Status;

	//
	// We need an arbitrary address that is guaranteed to lie
	// within the current module - use a function pointer.
	//
	#pragma warning( push )
	#pragma warning( disable : 4054 )
	AddressWithinCurrentModule = 
		( ULONG_PTR ) ( PVOID ) CfixkDrvQueryReporter;
	#pragma warning( pop )

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
	Stack->Parameters.QueryInterface.Size			= sizeof( CFIXKR_REPORT_SINK_INTERFACE_3 ) ;
	Stack->Parameters.QueryInterface.Version		= CFIXKR_REPORT_SINK_VERSION_3;
	Stack->Parameters.QueryInterface.Interface		= ( PINTERFACE ) &CfixkDrvReportSink;
	Stack->Parameters.QueryInterface.InterfaceSpecificData = ( PVOID ) AddressWithinCurrentModule;

	Status = IoCallDriver( CfixkrDevObject, Irp );
	if ( STATUS_PENDING == Status )
	{
		KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
		Status = IoStatusBlock.Status;
	}

	ASSERT( NT_SUCCESS( Status ) == ( CfixkDrvReportSink.Base.Context != NULL ) );
	ASSERT( NT_SUCCESS( Status ) == 
		( CfixkDrvReportSink.Base.Version == CFIXKR_REPORT_SINK_VERSION_3 ) );
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

VOID CFIXCALLTYPE CfixPeFail()
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		CfixkDrvReportSink.Methods.Fail(
			CfixkDrvReportSink.Base.Context );
	}
}

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

CFIX_REPORT_DISPOSITION __cdecl CfixPeReportFailedAssertionFormatW(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in ULONG Line,
	__in PCWSTR Format,
	...
	)
{
	CFIX_REPORT_DISPOSITION Disposition;
	va_list VarArgs;

	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		va_start( VarArgs, Format );
		Disposition = CfixkDrvReportSink.Methods.ReportFailedAssertionFormat(
			CfixkDrvReportSink.Base.Context,
			File,
			Routine,
			Line,
			Format,
			VarArgs );
		va_end( VarArgs );

		return Disposition;
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

PVOID CfixPeGetValue(
	__in ULONG Tag
	)
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		return CfixkDrvReportSink.Methods.GetValue(
			CfixkDrvReportSink.Base.Context,
			Tag );
	}
	else
	{
		return NULL;
	}
}

VOID CfixPeSetValue(
	__in ULONG Tag,
	__in PVOID Value
	)
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context != NULL )
	{
		CfixkDrvReportSink.Methods.SetValue(
			CfixkDrvReportSink.Base.Context,
			Tag,
			Value );
	}
}

NTSTATUS CFIXCALLTYPE CfixCreateSystemThread(
    __out PHANDLE ThreadHandle,
    __in ULONG DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt HANDLE ProcessHandle,
    __out_opt PCLIENT_ID ClientId,
    __in PKSTART_ROUTINE StartRoutine,
    __in_opt PVOID StartContext,
	__in ULONG Flags
    )
{
	ASSERT( CfixkDrvReportSink.Base.Context != NULL );
	if ( CfixkDrvReportSink.Base.Context == NULL )
	{
		return STATUS_UNSUCCESSFUL;
	}

	return CfixkDrvReportSink.Methods.CreateSystemThread(
		CfixkDrvReportSink.Base.Context,
		ThreadHandle,
		DesiredAccess,
		ObjectAttributes,
		ProcessHandle,
		ClientId,
		StartRoutine,
		StartContext,
		Flags );
}