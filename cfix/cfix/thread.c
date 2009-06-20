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
		return ( HANDLE ) _beginthreadex(
			ThreadAttributes,
			( unsigned) StackSize,
			( unsigned ( * )( void * ) ) CfixsThreadStart,
			Parameters,
			CreationFlags,
			( unsigned * ) ThreadId );
	}
	else
	{
		return CreateThread(
			ThreadAttributes,
			StackSize,
			CfixsThreadStart,
			Parameters,
			CreationFlags,
			ThreadId );
	}
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
