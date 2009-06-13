/*----------------------------------------------------------------------
 * Purpose:
 *		DriverEntry and Dispatch routines. 
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
#include <aux_klib.h>
#include "cfixkrp.h"

//#define CFIXKRP_TRACE KdPrint
#define CFIXKRP_TRACE( x )

DRIVER_DISPATCH CfixkrpDispatchCreate;
DRIVER_DISPATCH CfixkrpDispatchCleanup;
DRIVER_DISPATCH CfixkrpDispatchClose;
DRIVER_DISPATCH CfixkrpDispatchDeviceControl;
DRIVER_DISPATCH CfixkrpDispatchPnp;
DRIVER_UNLOAD CfixkrpUnload;

typedef struct _CFIXKRP_DEVICE_EXTENSION
{
	ULONG Unused;
} CFIXKRP_DEVICE_EXTENSION, *PCFIXKRP_DEVICE_EXTENSION;

/*----------------------------------------------------------------------
 *
 * IOCTL routines. All routines use buffered I/O.
 *
 */
static NTSTATUS CfixkrsQueryModuleIoctl(
	__in PVOID Buffer,
	__in ULONG InputBufferLength,
	__in ULONG OutputBufferLength,
	__out PULONG BytesWritten
	)
{
	ULONG BufferSize;
	PCFIXKRP_DRIVER_CONNECTION DriverConnection;
	PCFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST Request;
	PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE Response;
	NTSTATUS Status;

	ASSERT( BytesWritten );

	*BytesWritten = 0;

	//
	// Validate parameters.
	//
	if ( ! Buffer ||
		 InputBufferLength != sizeof( CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST ) ||
		 OutputBufferLength < RTL_SIZEOF_THROUGH_FIELD( 
			CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE, u.SizeRequired ) )
	{
		return STATUS_INVALID_PARAMETER;
	}

	Request = ( PCFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST ) Buffer;
	Response = ( PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE ) Buffer;

	//
	// Try to lookup connection to this driver.
	//
	Status = CfixkrpLookupDriverConnection(
		Request->DriverBaseAddress,
		&DriverConnection );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	//
	// Fill buffer - if it is large enough.
	//
	Status = CfixkrpQueryModuleDriverConnection(
		DriverConnection,
		OutputBufferLength,
		Buffer,
		&BufferSize );

	if ( Status == STATUS_BUFFER_OVERFLOW )
	{
		//
		// BufferSize contains necessary size.
		//
		Response->u.SizeRequired = BufferSize;
		*BytesWritten = RTL_SIZEOF_THROUGH_FIELD( 
			CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE, u.SizeRequired );
	}
	else if ( NT_SUCCESS( Status ) )
	{
		//
		// Buffer has been filled.
		//
		*BytesWritten = BufferSize;
	}
	else
	{
		//
		// Some error occurred.
		//
	}

	CfixkrpDereferenceConnection( DriverConnection );

	ASSERT( *BytesWritten <= OutputBufferLength );

	return Status;
}

static NTSTATUS CfixkrsGetModulesIoctl(
	__in PVOID Buffer,
	__in ULONG InputBufferLength,
	__in ULONG OutputBufferLength,
	__out PULONG BytesWritten
	)
{
	ULONG Capacity;
	ULONG ElementsWritten;
	ULONG ElementsAvailable;
	PCFIXKR_IOCTL_GET_MODULES Response;
	NTSTATUS Status;

	ASSERT( BytesWritten );

	*BytesWritten = 0;

	//
	// Validate parameters.
	//
	if ( ! Buffer ||
		 InputBufferLength != 0 ||
		 OutputBufferLength < sizeof( CFIXKR_IOCTL_GET_MODULES ) )
	{
		return STATUS_INVALID_PARAMETER;
	}

	//
	// Calculate how many elements the output buffer can hold.
	//
	Capacity = ( OutputBufferLength - 
		FIELD_OFFSET( CFIXKR_IOCTL_GET_MODULES, DriverLoadAddress ) ) /
		sizeof( ULONGLONG );
	Response = ( PCFIXKR_IOCTL_GET_MODULES ) Buffer;
	
	ASSERT( Capacity >= 1 );

	Status = CfixkrpGetDriverConnections(
		Capacity,
		Response->DriverLoadAddress,
		&ElementsWritten,
		&ElementsAvailable );
	if ( Status == STATUS_BUFFER_OVERFLOW )
	{
		Response->Count = ElementsAvailable;
		*BytesWritten = RTL_SIZEOF_THROUGH_FIELD( 
			CFIXKR_IOCTL_GET_MODULES, 
			DriverLoadAddress[ ElementsWritten - 1 ] );
	}
	else if ( NT_SUCCESS( Status ) )
	{
		Response->Count = ElementsWritten;
		if ( ElementsWritten == 0 )
		{
			*BytesWritten = RTL_SIZEOF_THROUGH_FIELD( 
				CFIXKR_IOCTL_GET_MODULES, 
				Count );
		}
		else
		{
			*BytesWritten = RTL_SIZEOF_THROUGH_FIELD( 
				CFIXKR_IOCTL_GET_MODULES, 
				DriverLoadAddress[ ElementsWritten - 1 ] );
		}
	}
	else
	{
		//
		// Some error occurred.
		//
	}

	ASSERT( *BytesWritten <= OutputBufferLength );

	return Status;
}

static NTSTATUS CfixkrsCallRoutineIoctl(
	__in PVOID Buffer,
	__in ULONG InputBufferLength,
	__in ULONG OutputBufferLength,
	__out PULONG BytesWritten
	)
{
	CFIXKRP_REPORT_CHANNEL Channel;
	PCFIXKRP_DRIVER_CONNECTION DriverConnection;
	PCFIXKR_IOCTL_CALL_ROUTINE_REQUEST Request;
	PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Response;
	NTSTATUS Status;

	ASSERT( BytesWritten );

	*BytesWritten = 0;

	//
	// Validate parameters.
	//
	if ( ! Buffer ||
		 InputBufferLength  < sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ) ||
		 OutputBufferLength < sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) )
	{
		return STATUS_INVALID_PARAMETER;
	}

	Request = ( PCFIXKR_IOCTL_CALL_ROUTINE_REQUEST ) Buffer;
	Response = ( PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) Buffer;

	if ( Request->DriverBaseAddress == 0 )
	{
		return STATUS_INVALID_PARAMETER;
	}

	if ( Request->Dispositions.FailedAssertion > CfixAbort ||
		 Request->Dispositions.UnhandledException > CfixAbort )
	{
		return STATUS_INVALID_PARAMETER;
	}

	Channel.Signature						= CFIXKRP_REPORT_CHANNEL_SIGNATURE;
	Channel.Dispositions.FailedAssertion	= Request->Dispositions.FailedAssertion;
	Channel.Dispositions.UnhandledException = Request->Dispositions.UnhandledException;

	//
	// Event structures will be appended to the output buffer
	// after the end of the CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE struct.
	//
	// We can thus use the buffer to directly write events to.
	//
	Channel.EventBuffer.BufferSize			= 
		OutputBufferLength - sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE );
	Channel.EventBuffer.BufferLength		= 0;
	Channel.EventBuffer.BufferTruncated		= FALSE;
	Channel.EventBuffer.Buffer				= 
		( PUCHAR ) Buffer + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE );
	Channel.EventBuffer.EventCount			= 0;

	Channel.TlsValue = ( PVOID ) ( ULONG_PTR ) Request->TlsValue;

	//
	// Try to lookup connection to this driver.
	//
	Status = CfixkrpLookupDriverConnection(
		Request->DriverBaseAddress,
		&DriverConnection );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	Status = CfixkrpCallRoutineDriverConnection(
		DriverConnection,
		Request->FixtureKey,
		Request->RoutineKey,
		&Channel,
		&Response->RoutineRanToCompletion,
		&Response->AbortRun );
	if ( NT_SUCCESS( Status ) )
	{
		//
		// Initialize the remaining parts of the response. The events
		// have already been written to the buffer.
		//
		ASSERT( Channel.EventBuffer.BufferLength <= Channel.EventBuffer.BufferSize );
		ASSERT( ( Channel.EventBuffer.BufferLength % 8 ) == 0 );

		Response->Events.Count = Channel.EventBuffer.EventCount;
		Response->Events.Flags = Channel.EventBuffer.BufferTruncated
			? CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED
			: 0;

		Response->TlsValue = ( ULONG_PTR ) Channel.TlsValue;

		*BytesWritten = sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) +
			Channel.EventBuffer.BufferLength;
	}

	CfixkrpDereferenceConnection( DriverConnection );

	ASSERT( *BytesWritten <= OutputBufferLength );

	return Status;
}


/*----------------------------------------------------------------------
 *
 * Dispatch routines.
 *
 */

/*++
	Routine Description:
		Handles IRP_MJ_CREATE. Called either by client code (user mode)
		to later initiate a session or by a test driver.
--*/
NTSTATUS CfixkrpDispatchCreate(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp 
	)
{
	CFIXKRP_TRACE(( "CFIXKR: CfixkrpDispatchCreate.\n" ));

	UNREFERENCED_PARAMETER( DeviceObject );
	UNREFERENCED_PARAMETER( Irp );

	return CfixkrpCompleteRequest( Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT );
}

NTSTATUS CfixkrpDispatchCleanup(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp 
	)
{
	CFIXKRP_TRACE(( "CFIXKR: CfixkrpDispatchCleanup.\n" ));
	UNREFERENCED_PARAMETER( DeviceObject );
	UNREFERENCED_PARAMETER( Irp );
	
	return CfixkrpCompleteRequest( Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT );
}

NTSTATUS CfixkrpDispatchClose(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp 
	)
{
	PIO_STACK_LOCATION StackLocation;

	CFIXKRP_TRACE(( "CFIXKR: CfixkrpDispatchClose.\n" ));
	UNREFERENCED_PARAMETER( DeviceObject );

	StackLocation = IoGetCurrentIrpStackLocation( Irp );

	ASSERT( StackLocation->FileObject &&
			StackLocation->FileObject->FsContext == NULL );

	return CfixkrpCompleteRequest( Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT );
}

NTSTATUS CfixkrpDispatchDeviceControl(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp 
	)
{
	PIO_STACK_LOCATION StackLocation;
	ULONG ResultSize;
	NTSTATUS Status;

	CFIXKRP_TRACE(( "CFIXKR: CfixkrpDispatchDeviceControl.\n" ));
	UNREFERENCED_PARAMETER( DeviceObject );

	StackLocation = IoGetCurrentIrpStackLocation( Irp );

	switch ( StackLocation->Parameters.DeviceIoControl.IoControlCode )
	{
	case CFIXKR_IOCTL_QUERY_TEST_MODULE:
		Status = CfixkrsQueryModuleIoctl(
			Irp->AssociatedIrp.SystemBuffer,
			StackLocation->Parameters.DeviceIoControl.InputBufferLength,
			StackLocation->Parameters.DeviceIoControl.OutputBufferLength,
			&ResultSize );
		break;

	case CFIXKR_IOCTL_GET_TEST_MODULES:
		Status = CfixkrsGetModulesIoctl(
			Irp->AssociatedIrp.SystemBuffer,
			StackLocation->Parameters.DeviceIoControl.InputBufferLength,
			StackLocation->Parameters.DeviceIoControl.OutputBufferLength,
			&ResultSize );
		break;

	case CFIXKR_IOCTL_CALL_ROUTINE:
		Status = CfixkrsCallRoutineIoctl(
			Irp->AssociatedIrp.SystemBuffer,
			StackLocation->Parameters.DeviceIoControl.InputBufferLength,
			StackLocation->Parameters.DeviceIoControl.OutputBufferLength,
			&ResultSize );
		break;

	default:
		ResultSize	= 0;
		Status		= STATUS_INVALID_DEVICE_REQUEST;
	}

	return CfixkrpCompleteRequest( Irp, Status, ResultSize, IO_NO_INCREMENT );
}

/*++
	Routine Description:
		Handles IRP_MJ_PNP/IRP_MN_QUERY_INTERFACE.
--*/
NTSTATUS CfixkrpDispatchPnp(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP Irp 
	)
{
	PIO_STACK_LOCATION StackLocation;
	ULONG_PTR ResultSize;
	NTSTATUS Status;

	CFIXKRP_TRACE(( "CFIXKR: CfixkrpDispatchPnp.\n" ));
	UNREFERENCED_PARAMETER( DeviceObject );

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	StackLocation = IoGetCurrentIrpStackLocation( Irp );

	if ( StackLocation->MinorFunction == IRP_MN_QUERY_INTERFACE )
	{
		if ( InlineIsEqualGUID( 
				StackLocation->Parameters.QueryInterface.InterfaceType, 
				&GUID_CFIXKR_REPORT_SINK ) &&
			 StackLocation->Parameters.QueryInterface.Size == sizeof( CFIXKR_REPORT_SINK_INTERFACE ) &&
			 StackLocation->Parameters.QueryInterface.Interface != NULL &&
			 StackLocation->Parameters.QueryInterface.InterfaceSpecificData != NULL )
		{
			ULONG_PTR BaseAddress;
			ULONG_PTR AddressWithinDriver;
			PCFIXKRP_DRIVER_CONNECTION Reporter;
			
			//
			// N.B. In cfix 1.1, drivers reported their load address. As
			// of cfix 1.2, they merely report an arbitrary address within
			// the module. Thus, calculate the load address here.
			//

			AddressWithinDriver = ( ULONG_PTR ) 
				StackLocation->Parameters.QueryInterface.InterfaceSpecificData;

			Status = CfixkrpGetModuleBaseAddress(
				AddressWithinDriver,
				&BaseAddress );
			if ( ! NT_SUCCESS( Status ) )
			{
				CFIXKRP_TRACE( ( "CFIXKR: Failed to retrieve base address "
					"for address %p.\n", ( PVOID ) AddressWithinDriver ) );

				return STATUS_DRIVER_INTERNAL_ERROR;
			}
			
			CFIXKRP_TRACE( ( "CFIXKR: Report Sink requested for %p.\n", 
				( PVOID ) BaseAddress ) );

			ResultSize	= 0;
			Status = CfixkrpCreateAndRegisterDriverConnection(
				BaseAddress,
				&Reporter );

			if ( NT_SUCCESS( Status ) )
			{
				ULONG Version = StackLocation->Parameters.QueryInterface.Version;
				switch ( Version )
				{
				case CFIXKR_REPORT_SINK_VERSION_1 :
					CfixkrpQuerySinkInterfaceDriverConnection( 
						Reporter, 
						( PCFIXKR_REPORT_SINK_INTERFACE )
							StackLocation->Parameters.QueryInterface.Interface );
					break;

				case CFIXKR_REPORT_SINK_VERSION_2:
					CfixkrpQuerySinkInterfaceDriverConnection2( 
						Reporter, 
						( PCFIXKR_REPORT_SINK_INTERFACE_2 )
							StackLocation->Parameters.QueryInterface.Interface );
					break;

				default:
					CFIXKRP_TRACE(( "CFIXKR: Unsupported interface version requested.\n" ));

					ResultSize	= 0;
					Status		= STATUS_INVALID_DEVICE_REQUEST;
				}
			}
		}
		else
		{
			CFIXKRP_TRACE(( "CFIXKR: Unsupported interface requested.\n" ));

			ResultSize	= 0;
			Status		= STATUS_INVALID_DEVICE_REQUEST;
		}
	}
	else
	{
		ResultSize	= 0;
		Status		= STATUS_INVALID_DEVICE_REQUEST;
	}

	return CfixkrpCompleteRequest( Irp, Status, 0, IO_NO_INCREMENT );
}

VOID CfixkrpUnload(
	__in PDRIVER_OBJECT DriverObject
	)
{
	PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
	UNICODE_STRING NameStringDos = RTL_CONSTANT_STRING( CFIXKR_DEVICE_DOS_NAME ) ;
	
	KdPrint(( "CFIXKR: Unload.\n" ));

	IoDeleteSymbolicLink( &NameStringDos );

	if ( DeviceObject != NULL )
	{
		IoDeleteDevice( DeviceObject );
	}

	CfixkrpTeardownDriverConnectionRegistry();
}

/*----------------------------------------------------------------------
 *
 * DriverEntry.
 *
 */
NTSTATUS DriverEntry(
	__in PDRIVER_OBJECT DriverObject,
	__in PUNICODE_STRING RegistryPath
	)
{
	PDEVICE_OBJECT DeviceObject;
	UNICODE_STRING NameStringDos = RTL_CONSTANT_STRING( CFIXKR_DEVICE_DOS_NAME ) ;
	UNICODE_STRING NameStringNt = RTL_CONSTANT_STRING( CFIXKR_DEVICE_NT_NAME ) ;
	NTSTATUS Status;

	UNREFERENCED_PARAMETER( RegistryPath );

	//
	// We'll need AuxKlib.
	//
	Status = AuxKlibInitialize();
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}
	
	Status = CfixkrpInitializeDriverConnectionRegistry();
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	//
	// Create the single device.
	//
	Status = IoCreateDevice(
		DriverObject,
		sizeof ( CFIXKRP_DEVICE_EXTENSION ),
		&NameStringNt,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		FALSE,
		&DeviceObject );
	if ( ! NT_SUCCESS( Status ) )
	{
		goto Cleanup;
	}

	//
	// Create symlink.
	//
	Status = IoCreateSymbolicLink(
		&NameStringDos,
		&NameStringNt );

	if ( ! NT_SUCCESS( Status ) )
	{
		goto Cleanup;
	}

	//
	// Install routines.
	//
	DriverObject->MajorFunction[ IRP_MJ_CREATE ]	= CfixkrpDispatchCreate;
	DriverObject->MajorFunction[ IRP_MJ_CLEANUP ]	= CfixkrpDispatchCleanup;
	DriverObject->MajorFunction[ IRP_MJ_CLOSE ]		= CfixkrpDispatchClose;
	DriverObject->MajorFunction[ IRP_MJ_PNP ]		= CfixkrpDispatchPnp;
	DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = CfixkrpDispatchDeviceControl;
	DriverObject->DriverUnload						= CfixkrpUnload;

Cleanup:
	if ( ! NT_SUCCESS( Status ) )
	{
		//
		// Initialization failed.
		//

		if ( DeviceObject != NULL )
		{
			IoDeleteDevice( DeviceObject );
		}

		CfixkrpTeardownDriverConnectionRegistry();
	}

	return Status;
}

