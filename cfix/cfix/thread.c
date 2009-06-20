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
	PCFIX_EXECUTION_CONTEXT ExecutionContext;
	PVOID ParentContext;
	ULONG ParentThreadId;
} THREAD_START_PARAMETERS, *PTHREAD_START_PARAMETERS;

static DWORD CfixsThreadStart(
	__in PTHREAD_START_PARAMETERS Parameters
	)
{
	BOOL Dummy;
	DWORD ExitCode;

	ASSERT( Parameters->StartAddress );
	ASSERT( Parameters->ExecutionContext );
	__assume( Parameters->ExecutionContext );

	//
	// Set current context s.t. it is accessible by callees
	// without having to pass it explicitly.
	//
	VERIFY( S_OK == CfixpSetCurrentExecutionContext( 
		Parameters->ExecutionContext, 
		Parameters->ParentThreadId,
		NULL ) );

	//
	// Notify execution context about the thread having been spawned.
	//
	Parameters->ExecutionContext->BeforeChildThreadStart(
		Parameters->ExecutionContext,
		Parameters->ParentThreadId,
		Parameters->ParentContext );

	__try
	{
		ExitCode = ( Parameters->StartAddress )( Parameters->UserParaneter );
	}
	__except ( CfixpExceptionFilter( 
		GetExceptionInformation(), 
		Parameters->ExecutionContext,
		Parameters->ParentThreadId,
		&Dummy ) )
	{
		NOP;
		ExitCode = ( DWORD ) CFIX_EXIT_THREAD_ABORTED;
	}

	Parameters->ExecutionContext->AfterChildThreadFinish(
		Parameters->ExecutionContext,
		Parameters->ParentThreadId,
		Parameters->ParentContext );

	VERIFY( S_OK == CfixpSetCurrentExecutionContext( 
		NULL, 
		Parameters->ParentThreadId,
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
	HRESULT Hr;
	PTHREAD_START_PARAMETERS Parameters;
	ULONG ParentThreadId;
	PCFIX_EXECUTION_CONTEXT CurrentContext;

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
	// The current context is inherited to the new thread.
	//
	Hr = CfixpGetCurrentExecutionContext( &CurrentContext, &ParentThreadId );
	if ( FAILED( Hr ) )
	{
		SetLastError( Hr );
		return NULL;
	}

	Parameters->StartAddress		= StartAddress;
	Parameters->UserParaneter		= UserParameter;
	Parameters->ExecutionContext	= CurrentContext;
	Parameters->ParentThreadId		= ParentThreadId;

	//
	// Notify parent and obtain ParentContext.
	//
	Hr = CurrentContext->CreateChildThread(
		CurrentContext,
		ParentThreadId,
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
