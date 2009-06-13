/*----------------------------------------------------------------------
 * Purpose:
 *		PE file assessment.
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
#include <Psapi.h>
#include <windows.h>
#include <shlwapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

typedef struct _PE_TEST_MODULE
{
	CFIX_TEST_MODULE Base;
	HMODULE Module;
	WCHAR ModuleName[ 64 ];
	volatile LONG ReferenceCount;
} PE_TEST_MODULE, *PPE_TEST_MODULE;

//
// Disable FunctionPtr <-> Data conversion warnings.
//
#pragma warning( push )
#pragma warning( disable: 4054; disable: 4055 )

/*----------------------------------------------------------------------
 * 
 * Statics.
 *
 */
static BOOL CfixsIsFixtureExport(
	__in PCSTR ExportName 
	)
{
	//
	// Check if name begins with CFIX_FIXTURE_EXPORT_PREFIX.
	//
	size_t Len = strlen( ExportName );

//	#pragma warning( suppress : 6385 )
	return ( Len > CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH && 
		     0 == memcmp( 
				ExportName, 
				CFIX_FIXTURE_EXPORT_PREFIX_MANGLED, 
				CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH ) );
}

static VOID CfixsUnmangleName(
	__in PWSTR Name 
	)
{
	//
	// The leading '_' has already been removed, remove the trailing
	// @N.
	//
	UINT Index;
	for ( Index = 0; Index < wcslen( Name ); Index++ )
	{
		if ( Name[ Index ] == L'@' )
		{
			Name[ Index ] = L'\0';
			break;
		}
	}
}

static HRESULT CfixsCreateFixture(
	__in PCSTR ExportName,
	__in PVOID ExportedRoutine,
	__in PCFIX_TEST_MODULE Module,
	__out PCFIX_FIXTURE *Fixture
	)
{
	CFIX_GET_FIXTURE_ROUTINE GetTcRoutine = 
		( CFIX_GET_FIXTURE_ROUTINE ) ExportedRoutine;
	PCFIX_TEST_PE_DEFINITION TestDef;
	PCFIX_PE_DEFINITION_ENTRY Entry;
	PCFIX_FIXTURE NewFixture = NULL;
	UINT EntryCount = 0;
	HRESULT Hr = E_UNEXPECTED;

	//
	// Obtain information from DLL.
	//
	TestDef = ( GetTcRoutine )();
	if ( ! TestDef )
	{
		return CFIX_E_MISBEHAVIOUD_GETTC_ROUTINE;
	}

	if ( TestDef->ApiVersion != CFIX_PE_API_VERSION )
	{
		return CFIX_E_UNSUPPORTED_VERSION;
	}
	
	//
	// Count entries (includes Setup and Teardown if available).
	//
	while ( TestDef->Entries[ EntryCount ].Type != CfixEntryTypeEnd )
	{
		EntryCount++;
	}

	//
	// Allocate fixture structure of appropriate size.
	//
	NewFixture = ( PCFIX_FIXTURE ) 
		malloc ( RTL_SIZEOF_THROUGH_FIELD( 
			CFIX_FIXTURE, 
			TestCases[ EntryCount - 1 ] ) );
	if ( ! NewFixture )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	//
	// Name.
	//
	if ( 0 == MultiByteToWideChar(
		CP_ACP,
		0,
		ExportName + CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH,
		-1,
		NewFixture->Name,
		_countof( NewFixture->Name ) ) )
	{
		DWORD Err = GetLastError();
		Hr = ( ERROR_INSUFFICIENT_BUFFER == Err )
			? CFIX_E_FIXTURE_NAME_TOO_LONG
			: HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	CfixsUnmangleName( NewFixture->Name );

	//
	// Walk map.
	//
	NewFixture->SetupRoutine	= 0;
	NewFixture->TeardownRoutine = 0;
	NewFixture->BeforeRoutine	= 0;
	NewFixture->AfterRoutine	= 0;
	NewFixture->Module			= Module;
	NewFixture->TestCaseCount	= 0;
	NewFixture->Reserved		= 0;

	for ( Entry = TestDef->Entries; Entry->Type != CfixEntryTypeEnd; Entry++ )
	{
		switch ( Entry->Type )
		{
		case CfixEntryTypeSetup:
			if ( NewFixture->SetupRoutine )
			{
				Hr = CFIX_E_DUP_SETUP_ROUTINE;
				goto Cleanup;
			}
			else
			{
				NewFixture->SetupRoutine = ( ULONG_PTR ) ( PVOID ) Entry->Routine;
			}
			break;

		case CfixEntryTypeTeardown:
			if ( NewFixture->TeardownRoutine )
			{
				Hr = CFIX_E_DUP_TEARDOWN_ROUTINE;
				goto Cleanup;
			}
			else
			{
				NewFixture->TeardownRoutine = ( ULONG_PTR ) ( PVOID ) Entry->Routine;
			}
			break;

		case CfixEntryTypeBefore:
			if ( NewFixture->BeforeRoutine )
			{
				Hr = CFIX_E_DUP_BEFORE_ROUTINE;
				goto Cleanup;
			}
			else
			{
				NewFixture->BeforeRoutine = ( ULONG_PTR ) ( PVOID ) Entry->Routine;
			}
			break;

		case CfixEntryTypeAfter:
			if ( NewFixture->AfterRoutine )
			{
				Hr = CFIX_E_DUP_AFTER_ROUTINE;
				goto Cleanup;
			}
			else
			{
				NewFixture->AfterRoutine = ( ULONG_PTR ) ( PVOID ) Entry->Routine;
			}
			break;

		case CfixEntryTypeTestcase:
			//
			// Entry->Name is statically allocated, so we can use
			// a pointer.
			//
			NewFixture->TestCases[ NewFixture->TestCaseCount ].Name =
				Entry->Name;
			NewFixture->TestCases[ NewFixture->TestCaseCount ].Routine =
				( ULONG_PTR ) ( PVOID ) Entry->Routine;
			NewFixture->TestCases[ NewFixture->TestCaseCount ].Fixture =
				NewFixture;

			NewFixture->TestCaseCount++;
			break;

		default:
			Hr = CFIX_E_UNKNOWN_ENTRY_TYPE;
			goto Cleanup;
		}
	}

	*Fixture = NewFixture;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( NewFixture )
		{
			free( NewFixture );
		}
	}

	return Hr;
}

static VOID CfixsDeleteFixture(
	__in PCFIX_FIXTURE Fixture
	)
{
	free( Fixture );
}

static HRESULT CfixsLoadFixturesFromPeImage(
	__in PPE_TEST_MODULE Module
	)
{
	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER ) Module->Module;
	PIMAGE_NT_HEADERS NtHeader; 
	PIMAGE_DATA_DIRECTORY ExportDataDir;
	PIMAGE_EXPORT_DIRECTORY ExportDir;
	UINT Index = 0;
	HRESULT Hr = E_UNEXPECTED;
	PDWORD NamesRvaArray;
	PDWORD FunctionRvaArray;
	PWORD OrdinalsArray;
	PCFIX_FIXTURE *FixturePtrArray = NULL;
	UINT FixtureCount = 0;
#ifdef DBG
	UINT AllocatedFixtures = 1;
#else
	UINT AllocatedFixtures = 16;
#endif

	//
	// FixturePtrArray will be filled as we are walking the export directory.
	// We do not know the size yet.
	//
	FixturePtrArray = ( PCFIX_FIXTURE* ) 
		malloc( AllocatedFixtures * sizeof( PCFIX_FIXTURE ) );
	if ( ! FixturePtrArray )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	NtHeader = ( PIMAGE_NT_HEADERS ) 
		CfixPtrFromRva( DosHeader, DosHeader->e_lfanew );
	if( IMAGE_NT_SIGNATURE != NtHeader->Signature )
	{
		Hr = HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
		goto Cleanup;
	}

	ExportDataDir = &NtHeader->OptionalHeader.DataDirectory
			[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
	ExportDir = ( PIMAGE_EXPORT_DIRECTORY ) CfixPtrFromRva( 
		DosHeader, 
		ExportDataDir->VirtualAddress );

	NamesRvaArray = ( PDWORD ) CfixPtrFromRva(
		DosHeader,
		ExportDir->AddressOfNames );

	FunctionRvaArray = ( PDWORD ) CfixPtrFromRva(
		DosHeader,
		ExportDir->AddressOfFunctions );

	OrdinalsArray = ( PWORD ) CfixPtrFromRva(
		DosHeader,
		ExportDir->AddressOfNameOrdinals );

	if ( ExportDir->NumberOfFunctions > 0 )
	{
		for ( Index = 0; Index < ExportDir->NumberOfNames; Index++ )
		{
			//
			// Get name of export.
			//
			PCSTR Name = ( PCSTR ) CfixPtrFromRva(
				DosHeader,
				NamesRvaArray[ Index ] );

			//
			// Get corresponding export ordinal.
			//
			WORD Ordinal = ( WORD ) OrdinalsArray[ Index ] + ( WORD ) ExportDir->Base;

			//
			// Get corresponding function RVA.
			//
			DWORD FuncRva = FunctionRvaArray[ Ordinal - ExportDir->Base ];

			if ( FuncRva >= ExportDataDir->VirtualAddress && 
				 FuncRva <  ExportDataDir->VirtualAddress + ExportDataDir->Size )
			{
				//
				// It is a forwarder.
				//
			}
			else if ( CfixsIsFixtureExport( Name ) )
			{
				PVOID ExportedRoutine = ( PVOID ) CfixPtrFromRva(
					DosHeader,
					FuncRva );

				//
				// That will become a fixture.
				//
				if ( FixtureCount == AllocatedFixtures )
				{
					//
					// Enlarge array.
					//
					PVOID NewFixtures;
					
					AllocatedFixtures *= 2;
					NewFixtures = realloc( 
						FixturePtrArray, AllocatedFixtures * sizeof( PCFIX_FIXTURE ) );
					if ( NewFixtures )
					{
						FixturePtrArray = ( PCFIX_FIXTURE* ) NewFixtures;
					}
					else
					{
						Hr = E_OUTOFMEMORY;
						goto Cleanup;
					}
				}

				Hr = CfixsCreateFixture(
					Name,
					ExportedRoutine,
					&Module->Base,
					&FixturePtrArray[ FixtureCount ] );
				if ( FAILED( Hr ) )
				{
					goto Cleanup;
				}

				FixtureCount++;
			}
		}
	}

	Hr = S_OK;
	Module->Base.FixtureCount = FixtureCount;
	Module->Base.Fixtures = FixturePtrArray;

Cleanup:
	if ( FAILED( Hr ) )
	{
		//
		// Free fixtures.
		//
		if ( FixturePtrArray )
		{
			for ( Index = 0; Index < FixtureCount; Index++ )
			{
				CfixsDeleteFixture( FixturePtrArray[ Index ] );
			}

			free( FixturePtrArray );
		}
	}

	return Hr;
}

/*----------------------------------------------------------------------
 * 
 * Methods.
 *
 */

static VOID CFIXCALLTYPE CfixsDeleteTestModule(
	__in PPE_TEST_MODULE Module
	)
{
	UINT Index;
	//
	// Delete fixtures...
	//
	for ( Index = 0; Index < Module->Base.FixtureCount; Index++ )
	{
		CfixsDeleteFixture(	Module->Base.Fixtures[ Index ] );
	}

	//
	// ...and the array...
	//
	free( Module->Base.Fixtures );

	//
	// ...and the module itself.
	//
	VERIFY( FreeLibrary( Module->Module ) );

	free( Module );
}

static VOID CfixsReferenceTestModuleMethod(
	__in PCFIX_TEST_MODULE TestModule
	)
{
	PPE_TEST_MODULE Module = ( PPE_TEST_MODULE ) TestModule;
	InterlockedIncrement( &Module->ReferenceCount );
}

static VOID CfixsDereferenceTestModuleMethod(
	__in PCFIX_TEST_MODULE TestModule
	)
{
	PPE_TEST_MODULE Module = ( PPE_TEST_MODULE ) TestModule;

	if ( 0 == InterlockedDecrement( &Module->ReferenceCount ) )
	{
		CfixsDeleteTestModule( Module );
	}
}

static DWORD CfixsExceptionFilter(
	__in PEXCEPTION_POINTERS ExcpPointers,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__out PBOOL AbortRun
	)
{
	DWORD ExcpCode = ExcpPointers->ExceptionRecord->ExceptionCode;

	if ( EXCEPTION_TESTCASE_INCONCLUSIVE == ExcpCode ||
		 EXCEPTION_TESTCASE_FAILED == ExcpCode )
	{
		//
		// Testcase failed/turned out to be inconclusive.
		//
		*AbortRun = FALSE;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else if ( EXCEPTION_TESTCASE_FAILED_ABORT == ExcpCode )
	{
		//
		// Testcase failed and is to be aborted.
		//
		*AbortRun = TRUE;
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else if ( EXCEPTION_BREAKPOINT == ExcpCode )
	{
		//
		// May happen when CfixBreakAlways was used. Do not handle
		// the exception, s.t. the debugger gets the opportunity to
		// attach.
		//
		return EXCEPTION_CONTINUE_SEARCH; 
	}
	else if ( EXCEPTION_STACK_OVERFLOW == ExcpCode )
	{
		//
		// I will not handle that one.
		//
		return EXCEPTION_CONTINUE_SEARCH; 
	}
	else
	{
		CFIX_REPORT_DISPOSITION Disp;
		CFIXP_EVENT_WITH_STACKTRACE Event;

		//
		// Notify.
		//
		Context->OnUnhandledException(
			Context,
			ExcpPointers );

		//
		// Capture stacktrace.
		//
		if ( IsDebuggerPresent() || FAILED( CfixpCaptureStackTrace(
			ExcpPointers->ContextRecord,
			&Event.Base.StackTrace,
			CFIXP_MAX_STACKFRAMES ) ) )
		{
			Event.Base.StackTrace.FrameCount = 0;
		}

		//
		// Report unhandled exception.
		//
		Event.Base.Type = CfixEventUncaughtException;
		memcpy( 
			&Event.Base.Info.UncaughtException,
			ExcpPointers->ExceptionRecord,
			sizeof( EXCEPTION_RECORD ) );

		Disp = Context->ReportEvent(
			Context,
			&Event.Base );

		*AbortRun = ( Disp == CfixAbort );
		if ( Disp == CfixBreak )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
		else
		{
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}
}

static HRESULT CfixsRunTestRoutine(
	__in CFIX_PE_TESTCASE_ROUTINE Routine,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	HRESULT Hr;
	BOOL AbortRun = FALSE;
	PCFIX_EXECUTION_CONTEXT PrevContext;
	BOOL RoutineRanToCompletion;

	if ( ! Routine )
	{
		return E_UNEXPECTED;
	}

	//
	// Set current context s.t. it is accessible by callees
	// without having to pass it explicitly.
	//
	// N.B. PrevContext is only required is this routine is called
	// recursively - which is only the case if a testcases uses 
	// the API to run another testcase (and that is what the cfix 
	// testcase does). PrevContext will be NULL otherwise.
	//
	Hr = CfixpSetCurrentExecutionContext( Context, &PrevContext );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	__try
	{
		( Routine )();
		RoutineRanToCompletion = TRUE;
	}
	__except ( CfixsExceptionFilter( 
		GetExceptionInformation(), 
		Context,
		&AbortRun ) )
	{
		RoutineRanToCompletion = FALSE;
	}

	VERIFY( S_OK == CfixpSetCurrentExecutionContext( PrevContext, NULL ) );

	if ( AbortRun )
	{
		return CFIX_E_TESTRUN_ABORTED;
	}
	else if ( ! RoutineRanToCompletion )
	{
		return CFIX_E_TEST_ROUTINE_FAILED;
	}
	else
	{
		return S_OK;
	}
}

static HRESULT CfixsRunTestCaseMethod(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_TEST_CASE TestCase,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	if ( ! Fixture || 
		 ! TestCase || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	return CfixsRunTestRoutine( 
		( CFIX_PE_TESTCASE_ROUTINE ) TestCase->Routine,
		Context );
}

static HRESULT CfixsRunSetupMethod(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	HRESULT Hr;

	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	//
	// Initialize TLS.
	//
	CfixPeSetValue( 0, NULL );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, NULL );

	//
	// Setup routines are optional.
	//
	if ( Fixture->SetupRoutine )
	{
		Hr = CfixsRunTestRoutine( 
			( CFIX_PE_TESTCASE_ROUTINE ) Fixture->SetupRoutine,
			Context );

		if ( CFIX_E_TEST_ROUTINE_FAILED == Hr )
		{
			//
			// If setup failed, continueing executing the fixture
			// is futile.
			//
			Hr = CFIX_E_SETUP_ROUTINE_FAILED;
		}
	}
	else
	{
		Hr = S_OK;
	}

	//
	// Reset TLS - values set by setup routine must not be visible 
	// afterwards.
	//
	CfixPeSetValue( 0, NULL );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, NULL );
	
	return Hr;
}

static HRESULT CfixsRunTeardownMethod(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	//
	// Teardown routines are optional.
	//
	if ( Fixture->TeardownRoutine )
	{
		HRESULT Hr = CfixsRunTestRoutine( 
			( CFIX_PE_TESTCASE_ROUTINE ) Fixture->TeardownRoutine,
			Context );

		if ( CFIX_E_TEST_ROUTINE_FAILED == Hr )
		{
			return CFIX_E_TEARDOWN_ROUTINE_FAILED;
		}
		else
		{
			return Hr;
		}
	}
	else
	{
		return S_OK;
	}
}

static HRESULT CfixsRunBeforeMethod(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	//
	// Before routines are optional.
	//
	if ( Fixture->BeforeRoutine )
	{
		HRESULT Hr = CfixsRunTestRoutine( 
			( CFIX_PE_TESTCASE_ROUTINE ) Fixture->BeforeRoutine,
			Context );

		if ( CFIX_E_TEST_ROUTINE_FAILED == Hr )
		{
			return CFIX_E_BEFORE_ROUTINE_FAILED;
		}
		else
		{
			return Hr;
		}
	}
	else
	{
		return S_OK;
	}
}

static HRESULT CfixsRunAfterMethod(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	HRESULT Hr;

	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	//
	// Before routines are optional.
	//
	if ( Fixture->AfterRoutine )
	{
		Hr = CfixsRunTestRoutine( 
			( CFIX_PE_TESTCASE_ROUTINE ) Fixture->AfterRoutine,
			Context );

		if ( CFIX_E_TEST_ROUTINE_FAILED == Hr )
		{
			Hr = CFIX_E_AFTER_ROUTINE_FAILED;
		}
	}
	else
	{
		Hr = S_OK;
	}

	//
	// Reset TLS - values must not be visible in subsequent tests and
	// teardown routine.
	//
	CfixPeSetValue( 0, NULL );
	CfixPeSetValue( CFIX_TAG_RESERVED_FOR_CC, NULL );
	
	return Hr;
}

/*----------------------------------------------------------------------
 * 
 * Exports.
 *
 */
HRESULT CFIXCALLTYPE CfixCreateTestModuleFromPeImage(
	__in PCWSTR ModulePath,
	__out PCFIX_TEST_MODULE *TestModule
	)
{
	PPE_TEST_MODULE Module;
	HRESULT Hr = E_UNEXPECTED;
	WCHAR FullPathName[ MAX_PATH ];
	PWSTR FilePart;

	if ( ! ModulePath || ! TestModule )
	{
		return E_INVALIDARG;
	}

	//
	// Make the path absoluet s.t. we do not end up loading the
	// wrong DLL due to the module load path logic.
	//
	if ( 0 == GetFullPathName(
		ModulePath,
		_countof( FullPathName ),
		FullPathName,
		&FilePart ) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	//
	// Allocate module struct.
	//
	Module = malloc( sizeof( PE_TEST_MODULE ) );
	if ( ! Module )
	{
		return E_OUTOFMEMORY;
	}

	Module->ReferenceCount = 1;

	Module->Base.Version				= CFIX_TEST_MODULE_VERSION;
	Module->Base.Routines.RunTestCase	= CfixsRunTestCaseMethod;
	Module->Base.Routines.Setup			= CfixsRunSetupMethod;
	Module->Base.Routines.Teardown		= CfixsRunTeardownMethod;
	Module->Base.Routines.Before		= CfixsRunBeforeMethod;
	Module->Base.Routines.After			= CfixsRunAfterMethod;
	Module->Base.Routines.Reference		= CfixsReferenceTestModuleMethod;
	Module->Base.Routines.Dereference	= CfixsDereferenceTestModuleMethod;
	Module->Base.Routines.GetInformationStackFrame = CfixpGetInformationStackframe;
	
	//
	// Load the module.
	//
	Module->Module = LoadLibrary( FullPathName );
	if ( ! Module->Module )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	if ( 0 == GetModuleBaseName(
		GetCurrentProcess(),
		Module->Module,
		Module->ModuleName,
		_countof( Module->ModuleName ) ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}
	PathRemoveExtension( Module->ModuleName );
	Module->Base.Name = Module->ModuleName;

	//
	// Assess PE image.
	//
	Module->Base.Fixtures = NULL;
	Hr = CfixsLoadFixturesFromPeImage( Module );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	Hr = S_OK;
	*TestModule = &Module->Base;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( Module )
		{
			if ( Module->Module )
			{
				VERIFY( FreeLibrary( Module->Module ) );
			}

			free( Module );
		}
	}

	return Hr;
}

/*----------------------------------------------------------------------
 *
 * Thread start wrapper.
 *
 */

typedef struct _THREAD_START_PARAMETERS
{
	PTHREAD_START_ROUTINE StartAddress;
	PVOID UserParaneter;
	PCFIX_EXECUTION_CONTEXT Context;
} THREAD_START_PARAMETERS, *PTHREAD_START_PARAMETERS;

static DWORD CfixsThreadStart(
	__in PTHREAD_START_PARAMETERS Parameters
	)
{
	BOOL Dummy;
	DWORD ExitCode;

	ASSERT( Parameters->StartAddress );
	ASSERT( Parameters->Context );

	//
	// Set current context s.t. it is accessible by callees
	// without having to pass it explicitly.
	//
	VERIFY( S_OK == CfixpSetCurrentExecutionContext( Parameters->Context, NULL ) );

	__try
	{
		ExitCode = ( Parameters->StartAddress )( Parameters->UserParaneter );
	}
	__except ( CfixsExceptionFilter( 
		GetExceptionInformation(), 
		Parameters->Context,
		&Dummy ) )
	{
		NOP;
		ExitCode = ( DWORD ) CFIX_EXIT_THREAD_ABORTED;
	}

	VERIFY( S_OK == CfixpSetCurrentExecutionContext( NULL, NULL ) );
	free( Parameters );

	return ExitCode;
}

HANDLE CfixCreateThread(
	__in PSECURITY_ATTRIBUTES ThreadAttributes,
	__in SIZE_T StackSize,
	__in PTHREAD_START_ROUTINE StartAddress,
	__in PVOID UserParameter,
	__in DWORD CreationFlags,
	__in PDWORD ThreadId
	)
{
	HRESULT Hr;
	PTHREAD_START_PARAMETERS Parameters;
	PCFIX_EXECUTION_CONTEXT CurrentContext;

	Parameters = malloc( sizeof( THREAD_START_PARAMETERS ) );
	if ( ! Parameters )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		return NULL;
	}

	//
	// The current context is inherited to the new thread.
	//
	Hr = CfixpGetCurrentExecutionContext( &CurrentContext );
	if ( FAILED( Hr ) )
	{
		SetLastError( Hr );
		return NULL;
	}

	Parameters->StartAddress		= StartAddress;
	Parameters->UserParaneter	= UserParameter;
	Parameters->Context			= CurrentContext;

	//
	// Give the execution context the opportunity to do some
	// housekeeping and create the thread for us.
	//
	return CurrentContext->CreateChildThread(
		CurrentContext,
		ThreadAttributes,
		StackSize,
		CfixsThreadStart,
		Parameters,
		CreationFlags,
		ThreadId );
}

#pragma warning( pop )

