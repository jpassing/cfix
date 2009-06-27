/*----------------------------------------------------------------------
 * Purpose:
 *		Child thread creation.
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
#include "cfixkrp.h"

typedef struct _CFIXKRP_THREAD_START_PARAMETERS
{
	PCFIXKRP_DRIVER_CONNECTION Connection;

	PKSTART_ROUTINE StartAddress;
	PVOID UserParaneter;
	
	PCFIXKRP_FILAMENT Filament;

	//
	// Event set by child thread when its cfix-related initialization
	// has been completed.
	//
	KEVENT InitializationCompleted;
	NTSTATUS InitializationStatus;
} CFIXKRP_THREAD_START_PARAMETERS, *PCFIXKRP_THREAD_START_PARAMETERS;

static VOID CfixkrsThreadStart(
	__in PCFIXKRP_THREAD_START_PARAMETERS Parameters
	)
{
	BOOLEAN Dummy;
	NTSTATUS Status;
	CFIX_THREAD_ID ThreadId;

	PCFIXKRP_FILAMENT_REGISTRY FilamentRegistry = 
		CfixkrpGetFilamentRegistryFromConnection( Parameters->Connection );
	PKSTART_ROUTINE StartAddress = Parameters->StartAddress;
	PVOID UserParaneter			 = Parameters->UserParaneter;
	PCFIXKRP_FILAMENT Filament	 = Parameters->Filament;

	ASSERT( FilamentRegistry );
	ASSERT( StartAddress );
	ASSERT( Filament );
	__assume( Filament );

	CfixkrpInitializeThreadId( 
		&ThreadId,
		Filament->MainThreadId,
		CfixkrGetCurrentThreadId() );

	//
	// Set current filament s.t. it is accessible by callees
	// without having to pass it explicitly.
	//
	// This may fail when there are too many child threads already.
	//
	Status = CfixkrpSetCurrentFilament( 
		FilamentRegistry,
		Filament );

	Parameters->InitializationStatus = Status;
	( VOID ) KeSetEvent( 
		&Parameters->InitializationCompleted,
		0,
		FALSE );

	if ( NT_SUCCESS( Status ) )
	{
		//
		// N.B. From now on, Parameters may not be touched any more; use
		// CopyOfParameters instead.
		//

		__try
		{
			( StartAddress )( UserParaneter );
		}
		__except ( CfixkrpExceptionFilter( 
			GetExceptionInformation(), 
			Filament,
			&Dummy ) )
		{
			NOP;
		}

		CfixkrpResetCurrentFilament( FilamentRegistry );
	}
}

NTSTATUS CfixkrpCreateSystemThread(
    __out PHANDLE ThreadHandle,
    __in ULONG DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt HANDLE ProcessHandle,
    __out_opt PCLIENT_ID ClientId,
    __in PKSTART_ROUTINE StartRoutine,
    __in PVOID StartContext,
	__in ULONG Flags,
	__in PCFIXKRP_DRIVER_CONNECTION Connection
    )
{
	PCFIXKRP_FILAMENT Filament;
	CFIXKRP_THREAD_START_PARAMETERS Parameters;
	NTSTATUS Status;

	ASSERT( Connection );
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	if ( Flags > CFIX_SYSTEM_THREAD_FLAG_SYSTEM_CONTEXT )
	{
		return STATUS_INVALID_PARAMETER;
	}
	else if ( ! ( Flags & CFIX_SYSTEM_THREAD_FLAG_SYSTEM_CONTEXT ) )
	{
		ULONG OsMajorVersion;
		ULONG OsMinorVersion;
		
		( VOID ) PsGetVersion( 
			&OsMajorVersion,
			&OsMinorVersion,
			NULL,
			NULL );

		if ( OsMajorVersion < 5 ||
			 OsMajorVersion == 5 && OsMinorVersion == 0 )
		{
			//
			// Windows 2000 or older. These OS do not support threads
			// to be created in a context other than system.
			//

			Flags |= CFIX_SYSTEM_THREAD_FLAG_SYSTEM_CONTEXT;
		}
	}

	//
	// The current filament is inherited to the new thread.
	//
	Filament = CfixkrpGetCurrentFilament( 
		CfixkrpGetFilamentRegistryFromConnection( Connection ) );
	if ( Filament == NULL )
	{
		return STATUS_UNSUCCESSFUL;
	}

	Parameters.Connection		= Connection;
	Parameters.StartAddress		= StartRoutine;
	Parameters.UserParaneter	= StartContext;
	Parameters.Filament			= Filament;

	KeInitializeEvent(
		&Parameters.InitializationCompleted,
		SynchronizationEvent,
		FALSE );

	//
	// Spawn thread using proxy ThreadStart routine.
	//
	Status = PsCreateSystemThread(
		ThreadHandle,
		DesiredAccess,
		ObjectAttributes,
		ProcessHandle,
		ClientId,
		CfixkrsThreadStart,
		&Parameters );

	if ( NT_SUCCESS( Status ) )
	{
		//
		// Thread has been spawned. However, we need to make sure
		// That execution on *this* thread does not continue until
		// the new thread has properly completed its cfix-related
		// initialization work and has registered wit the current
		// filament. Otherwise, it may occur that the current
		// test case completed before the child thread has got
		// a chance to begin its initialization -> race.
		//

		Status = KeWaitForSingleObject( 
			&Parameters.InitializationCompleted,
			Executive,
			KernelMode,
			FALSE,
			NULL );
		if ( NT_SUCCESS( Status ) &&
			 ! NT_SUCCESS( Parameters.InitializationStatus ) )
		{
			ZwClose( *ThreadHandle );
			ThreadHandle = NULL;
			Status = Parameters.InitializationStatus;
		}
	}

	return Status;
}
