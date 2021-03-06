#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Cfix main header file.
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

#include <cfixapi.h>
#include <cfixpe.h>
#include <crtdbg.h>
#include <windows.h>

#define ASSERT _ASSERTE

#ifdef DBG
#define VERIFY ASSERT
#else
#define VERIFY( x ) ( ( VOID ) ( x ) )
#endif

#define NOP ( ( VOID ) 0 )

extern HMODULE CfixpModule;

/*++
	Routines to be called from DllMain.
--*/
BOOL CfixpSetupFilamentTls();
BOOL CfixpSetupStackTraceCapturing();

BOOL CfixpTeardownFilamentTls();
VOID CfixpTeardownStackTraceCapturing();

/*----------------------------------------------------------------------
 *
 * Utility routines..
 *
 */

BOOL CfixpIsFixtureExport64(
	__in PCSTR ExportName 
	);

BOOL CfixpIsFixtureExport32(
	__in PCSTR ExportName 
	);

#if _WIN64
#define CfixpIsFixtureExport CfixpIsFixtureExport64
#else
#define CfixpIsFixtureExport CfixpIsFixtureExport32
#endif

#define CfixpFlagOn( Flags, FlagToTest ) ( ( Flags ) & ( FlagToTest ) )

__inline VOID CfixpInitializeThreadId( 
	__in PCFIX_THREAD_ID ThreadId, 
	__in ULONG MainOsThreadId, 
	__in ULONG OsThreadId 
	)
{
	ThreadId->MainThreadId	= MainOsThreadId;
	ThreadId->ThreadId		= OsThreadId;
}

/*----------------------------------------------------------------------
 *
 * Stack trace handling.
 *
 */

/*++
	Routine Description:
		Capture the stack trace based on the given CONTEXT.

	Parameters:
		Context		Thread context. If NULL, the current context is
					used.
		Event		Event structure to populate.
		MaxFrames	Max number of frames the structure can hold.
--*/
HRESULT CfixpCaptureStackTrace(
	__in_opt CONST PCONTEXT Context,
	__in PCFIX_STACKTRACE StackTrace,
	__in UINT MaxFrames 
	);

//
// 64 (63+1) frames ought to be enough... otherwise the trace will
// be cropped.
//
#define CFIXP_MAX_STACKFRAMES 64

typedef struct _CFIXP_EVENT_WITH_STACKTRACE
{
	CFIX_TESTCASE_EXECUTION_EVENT Base;
	ULONGLONG __AdditionalFrames[ CFIXP_MAX_STACKFRAMES - 1 ];
} CFIXP_EVENT_WITH_STACKTRACE, *PCFIXP_EVENT_WITH_STACKTRACE;

HRESULT CFIXCALLTYPE CfixpGetInformationStackframe(
	__in ULONGLONG Frame,
	__in SIZE_T ModuleNameCch,
	__out_ecount(ModuleNameCch) PWSTR ModuleName,
	__in SIZE_T FunctionNameCch,
	__out_ecount(FunctionNameCch) PWSTR FunctionName,
	__out PDWORD Displacement,
	__in SIZE_T SourceFileCch,
	__out_ecount(SourceFileCch) PWSTR SourceFile,
	__out PDWORD SourceLine 
	);

/*----------------------------------------------------------------------
 *
 * Filament.
 *
 */

/*++
	Structure description:
		A filament is a set of at least one thread. All thereads
		of a filament execute as part of a single test run and
		share an execution context.

		A filament starts off with one thread, the main thread. This
		thread may spawn any number of child threads, which all become
		part of the filament.
--*/		
typedef struct _CFIXP_FILAMENT
{
	PCFIX_EXECUTION_CONTEXT ExecutionContext;
	ULONG MainThreadId;

	struct
	{
		//
		// Lock guarding this sub-struct.
		//
		CRITICAL_SECTION Lock;

		ULONG ThreadCount;

		//
		// Handles of child threads.
		//
		HANDLE Threads[ CFIX_MAX_THREADS ];
	} ChildThreads;

	//
	// Capture stack backtraces for failed reports.
	//
	#define CFIXP_FILAMENT_FLAG_CAPTURE_STACK_TRACES	1
	#define CFIXP_FILAMENT_FLAG_DEFAULT_FILAMENT		2

	ULONG Flags;

	//
	// Filament local storage.
	//
	struct
	{
		PVOID DefaultSlot;
		PVOID CcSlot;
	} Storage;
} CFIXP_FILAMENT, *PCFIXP_FILAMENT;

/*++
	Routine Description:
		Initialize a filament structure. 
--*/
VOID CfixpInitializeFilament(
	__in PCFIX_EXECUTION_CONTEXT ExecutionContext,
	__in ULONG MainThreadId,
	__in ULONG Flags,
	__in BOOL RestoreStorage,
	__out PCFIXP_FILAMENT Filament
	);

/*++
	Routine Description:
		Destroy a filament structure. 
--*/
VOID CfixpDestroyFilament(
	__in PCFIXP_FILAMENT Filament
	);

/*++
	Routine Description:
		Set the filament for the current thread.

		Can be used to report events, although usage of the 
		wrapper functions is preferred.

	Parameters:
		Filament	- Filament to set.
		Prev		- context that has been set before. Can be used
					  to stack/unstack contexts.
--*/
HRESULT CfixpSetCurrentFilament(
	__in_opt PCFIXP_FILAMENT Filament,
	__out_opt PCFIXP_FILAMENT *Prev
	);

/*++
	Routine Description:
		Obtain the filament of the current thread.

		Can be used to report events, although usage of the 
		wrapper functions is preferred.

	Parameters:
		Filament			- current filament.
		DerivedFromDefault	- indicates whether filament is a result
							  of auto-registering/using default filament.
--*/
HRESULT CfixpGetCurrentFilament(
	__out PCFIXP_FILAMENT *Filament,
	__out_opt PBOOL DerivedFromDefault
	);

/*++
	Routine Description:
		Wait for all child threads to finish.
--*/
HRESULT CfixpJoinChildThreadsFilament(
	__in PCFIXP_FILAMENT Filament,
	__in ULONG Timeout
	);

/*++
	Routine Description:
		Provide an (optional) default filament to use for 'anonymous'
		threads. 

	The default filament must be revoked by setting it to NULL.
--*/
HRESULT CfixpSetDefaultFilament(
	__in_opt PCFIXP_FILAMENT Filament
	);

/*++
	Routine Description:
		Cleanup filament resources for a thread that is about to
		be detahced.

		Only to be called from DllMain.
--*/
VOID CfixpCleanupLeakedFilamentForDetachingThread();

/*++
	Routine Description:
		SEH Exception filter that is to be used for a try/except
		block around any test routines to be executed.
--*/
DWORD CfixpExceptionFilter(
	__in PEXCEPTION_POINTERS ExcpPointers,
	__in PCFIXP_FILAMENT Filament,
	__out PBOOL AbortRun
	);

