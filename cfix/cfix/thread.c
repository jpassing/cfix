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

#define CFIXAPI

#include <cfix.h>
#include "cfixp.h"
#include <stdlib.h>
#include <process.h> 

typedef struct _THREAD_START_PARAMETERS
{
	PTHREAD_START_ROUTINE StartAddress;
	PVOID UserParaneter;
	
	PVOID ParentContext;

	PCFIXP_FILAMENT Filament;

	//
	// Event set by child thread when its cfix-related initialization
	// has been completed.
	//
	HANDLE InitializationCompleted;
} THREAD_START_PARAMETERS, *PTHREAD_START_PARAMETERS;

static DWORD CfixsThreadStart(
	__in PTHREAD_START_PARAMETERS Parameters
	)
{
	BOOL Dummy;
	DWORD ExitCode;

	ASSERT( Parameters->StartAddress );
	ASSERT( Parameters->Filament );
	__assume( Parameters->Filament );

	//
	// Set current filament s.t. it is accessible by callees
	// without having to pass it explicitly.
	//
	VERIFY( S_OK == CfixpSetCurrentFilament( 
		Parameters->Filament,
		NULL ) );

	//
	// Notify execution context about the thread having been spawned.
	//
	Parameters->Filament->ExecutionContext->BeforeChildThreadStart(
		Parameters->Filament->ExecutionContext,
		Parameters->Filament->MainThreadId,
		Parameters->ParentContext );

	( VOID ) SetEvent( Parameters->InitializationCompleted );

	__try
	{
		ExitCode = ( Parameters->StartAddress )( Parameters->UserParaneter );
	}
	__except ( CfixpExceptionFilter( 
		GetExceptionInformation(), 
		Parameters->Filament,
		&Dummy ) )
	{
		NOP;
		ExitCode = ( DWORD ) CFIX_EXIT_THREAD_ABORTED;
	}

	Parameters->Filament->ExecutionContext->AfterChildThreadFinish(
		Parameters->Filament->ExecutionContext,
		Parameters->Filament->MainThreadId,
		Parameters->ParentContext );

	VERIFY( S_OK == CfixpSetCurrentFilament( 
		NULL, 
		NULL ) );
	free( Parameters );

	return ExitCode;
}

HANDLE CfixCreateThread2(
	__in PSECURITY_ATTRIBUTES ThreadAttributes,
	__in SIZE_T StackSize,
	__in PTHREAD_START_ROUTINE StartAddress,
	__in PVOID UserParameter,
	__in DWORD CreationFlags,
	__out_opt PDWORD ThreadId,
	__in ULONG Flags
	)
{
	PCFIXP_FILAMENT Filament;
	HRESULT Hr;
	PTHREAD_START_PARAMETERS Parameters;
	HANDLE Thread;

	if ( Flags > CFIX_THREAD_FLAG_CRT )
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	Parameters = malloc( sizeof( THREAD_START_PARAMETERS ) );
	if ( ! Parameters )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

	//
	// The current filament is inherited to the new thread.
	//
	Hr = CfixpGetCurrentFilament( &Filament );
	if ( FAILED( Hr ) )
	{
		SetLastError( Hr );
		return NULL;
	}

	Parameters->StartAddress		= StartAddress;
	Parameters->UserParaneter		= UserParameter;
	Parameters->Filament			= Filament;

	Parameters->InitializationCompleted = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( Parameters->InitializationCompleted == NULL )
	{
		//
		// Keep last error.
		//
		return NULL;
	}

	//
	// Notify parent and obtain ParentContext.
	//
	Hr = Filament->ExecutionContext->CreateChildThread(
		Filament->ExecutionContext,
		Filament->MainThreadId,
		&Parameters->ParentContext );
	if ( FAILED( Hr ) )
	{
		SetLastError( Hr );
		return NULL;
	}

	//
	// Spawn thread using proxy ThreadStart routine which will
	// perform the BeforeChildThreadStart and AfterChildThreadFinish
	// callbacks.
	//
	if ( Flags & CFIX_THREAD_FLAG_CRT )
	{
		Thread = ( HANDLE ) _beginthreadex(
			ThreadAttributes,
			( unsigned) StackSize,
			( unsigned ( * )( void * ) ) CfixsThreadStart,
			Parameters,
			CreationFlags,
			( unsigned * ) ThreadId );
	}
	else
	{
		Thread = CreateThread(
			ThreadAttributes,
			StackSize,
			CfixsThreadStart,
			Parameters,
			CreationFlags,
			ThreadId );
	}

	if ( Thread != NULL )
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

		( VOID ) WaitForSingleObject( 
			Parameters->InitializationCompleted,
			INFINITE );
		VERIFY( CloseHandle( Parameters->InitializationCompleted ) );
		Parameters->InitializationCompleted = NULL;
	}

	return Thread;
}

HANDLE CfixCreateThread(
	__in PSECURITY_ATTRIBUTES ThreadAttributes,
	__in SIZE_T StackSize,
	__in PTHREAD_START_ROUTINE StartAddress,
	__in PVOID UserParameter,
	__in DWORD CreationFlags,
	__out_opt PDWORD ThreadId
	)
{
	return CfixCreateThread2(
		ThreadAttributes,
		StackSize,
		StartAddress,
		UserParameter,
		CreationFlags,
		ThreadId,
		0 );
}
