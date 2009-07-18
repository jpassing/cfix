/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
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

#include <cfix.h>

static VOID ThreadProc( PVOID Pv )
{
	LARGE_INTEGER Interval;

	UNREFERENCED_PARAMETER( Pv );

	Interval.QuadPart = - 10 * 1000 * 1000;

	CFIX_LOG( L"Test" );

	CFIX_ASSERT( NT_SUCCESS( KeDelayExecutionThread(
		KernelMode,
		FALSE,
		&Interval ) ) );
}

static void SpawnAndJoinPolitely()
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	PETHREAD Thread;
	HANDLE ThreadHandle;

	InitializeObjectAttributes(
		&ObjectAttributes, 
		NULL, 
		OBJ_KERNEL_HANDLE, 
		NULL, 
		NULL );

	CFIX_ASSERT_STATUS( STATUS_SUCCESS, CfixCreateSystemThread(
		&ThreadHandle,
		THREAD_ALL_ACCESS,
		&ObjectAttributes,
		NULL,
		NULL,
		ThreadProc,
		NULL,
		0 ) );

	CFIX_ASSERT( NT_SUCCESS( ObReferenceObjectByHandle(
		ThreadHandle,
		THREAD_ALL_ACCESS,
		*PsThreadType,
		KernelMode,
		&Thread,
		NULL ) ) );

	CFIX_ASSERT( Thread );

	CFIX_ASSERT_STATUS( STATUS_SUCCESS, KeWaitForSingleObject( 
		Thread, 
		Executive,
		KernelMode,
		FALSE,
		NULL ) );

	ObDereferenceObject( Thread );
	CFIX_ASSERT( NT_SUCCESS( ZwClose( ThreadHandle ) ) );
}

static void SpawnAndAutoJoin()
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Thread;

	InitializeObjectAttributes(
		&ObjectAttributes, 
		NULL, 
		OBJ_KERNEL_HANDLE, 
		NULL, 
		NULL );

	CFIX_ASSERT_STATUS( STATUS_SUCCESS, CfixCreateSystemThread(
		&Thread,
		THREAD_ALL_ACCESS,
		&ObjectAttributes,
		NULL,
		NULL,
		ThreadProc,
		NULL,
		0 ) );
	CFIX_ASSERT( Thread );
	CFIX_ASSERT( NT_SUCCESS( ZwClose( Thread ) ) );
}

static void SpawnAndAutoJoinLotsOfThreads()
{
	ULONG Index;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Thread;
	
	for ( Index = 0; Index < CFIX_MAX_THREADS; Index++ )
	{
		SpawnAndAutoJoin();
	}

	InitializeObjectAttributes(
		&ObjectAttributes, 
		NULL, 
		OBJ_KERNEL_HANDLE, 
		NULL, 
		NULL );

	CFIX_ASSERT_STATUS( STATUS_ALLOTTED_SPACE_EXCEEDED, CfixCreateSystemThread(
		&Thread,
		THREAD_ALL_ACCESS,
		&ObjectAttributes,
		NULL,
		NULL,
		ThreadProc,
		NULL,
		0 ) );
}

static void SpawnAndAutoJoinLotsOfThreadsUsingSystemContext()
{
	ULONG Index;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Thread;
	
	for ( Index = 0; Index < CFIX_MAX_THREADS; Index++ )
	{
		InitializeObjectAttributes(
			&ObjectAttributes, 
			NULL, 
			OBJ_KERNEL_HANDLE, 
			NULL, 
			NULL );

		CFIX_ASSERT_STATUS( STATUS_SUCCESS, CfixCreateSystemThread(
			&Thread,
			THREAD_ALL_ACCESS,
			&ObjectAttributes,
			NULL,
			NULL,
			ThreadProc,
			NULL,
			CFIX_SYSTEM_THREAD_FLAG_SYSTEM_CONTEXT ) );
		CFIX_ASSERT( Thread );
		CFIX_ASSERT( NT_SUCCESS( ZwClose( Thread ) ) );
	}

	InitializeObjectAttributes(
		&ObjectAttributes, 
		NULL, 
		OBJ_KERNEL_HANDLE, 
		NULL, 
		NULL );

	CFIX_ASSERT_STATUS( STATUS_ALLOTTED_SPACE_EXCEEDED, CfixCreateSystemThread(
		&Thread,
		THREAD_ALL_ACCESS,
		&ObjectAttributes,
		NULL,
		NULL,
		ThreadProc,
		NULL,
		CFIX_SYSTEM_THREAD_FLAG_SYSTEM_CONTEXT ) );
}


CFIX_BEGIN_FIXTURE( FilamentJoin )
	CFIX_FIXTURE_ENTRY( SpawnAndJoinPolitely )
	CFIX_FIXTURE_ENTRY( SpawnAndAutoJoin )
	CFIX_FIXTURE_ENTRY( SpawnAndAutoJoinLotsOfThreads )
	CFIX_FIXTURE_ENTRY( SpawnAndAutoJoinLotsOfThreadsUsingSystemContext )

	//
	// Threads have to work in all these situations, too.
	//
	CFIX_FIXTURE_SETUP( SpawnAndAutoJoin )
	CFIX_FIXTURE_TEARDOWN( SpawnAndAutoJoin )
	CFIX_FIXTURE_BEFORE( SpawnAndAutoJoin )
	CFIX_FIXTURE_AFTER( SpawnAndAutoJoin )
CFIX_END_FIXTURE()