/*----------------------------------------------------------------------
 * Purpose:
 *		PE file assessment
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */

#define CFIXAPI

#include "internal.h"
#include <stdlib.h>
#include <Psapi.h>
#include <windows.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define EXCEPTION_TESTCASE_INCONCLUSIVE ( ( DWORD ) CFIX_E_TESTCASE_INCONCLUSIVE )
#define EXCEPTION_TESTCASE_FAILED		( ( DWORD ) CFIX_E_TESTCASE_FAILED )

//
// Slot for holding a PCFIX_EXECUTION_CONTEXT during
// testcase execution.
//
static DWORD CfixsTlsSlotForContext = TLS_OUT_OF_INDEXES;


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

#define PtrFromRva( base, rva ) ( ( ( PBYTE ) base ) + rva )

#define FIXTURE_EXPORT_PREFIX "__CfixFixturePe"
#define FIXTURE_EXPORT_PREFIX_CCH 15

#if _WIN64
#define FIXTURE_EXPORT_PREFIX_MANGLED		FIXTURE_EXPORT_PREFIX
#define FIXTURE_EXPORT_PREFIX_MANGLED_CCH	( FIXTURE_EXPORT_PREFIX_CCH )
#else
#define FIXTURE_EXPORT_PREFIX_MANGLED		"_" FIXTURE_EXPORT_PREFIX
#define FIXTURE_EXPORT_PREFIX_MANGLED_CCH	( FIXTURE_EXPORT_PREFIX_CCH + 1 )
#endif

static HRESULT CfixsSetCurrentExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context,
	__out_opt PCFIX_EXECUTION_CONTEXT *PrevContext
	)
{
	if ( PrevContext )
	{
		*PrevContext = ( PCFIX_EXECUTION_CONTEXT ) 
			TlsGetValue( CfixsTlsSlotForContext );
	}

	return TlsSetValue( CfixsTlsSlotForContext, Context )
		? S_OK
		: HRESULT_FROM_WIN32( GetLastError() );
}

static HRESULT CfixsGetCurrentExecutionContext(
	__out PCFIX_EXECUTION_CONTEXT *Context 
	)
{
	*Context = ( PCFIX_EXECUTION_CONTEXT ) 
		TlsGetValue( CfixsTlsSlotForContext );

	if ( *Context )
	{
		return S_OK;
	}
	else
	{
		return CFIX_E_UNKNOWN_THREAD;
	}
}

static BOOL CfixsIsFixtureExport(
	__in PCSTR ExportName 
	)
{
	//
	// Check if name begins with FIXTURE_EXPORT_PREFIX.
	//
	size_t Len = strlen( ExportName );

//	#pragma warning( suppress : 6385 )
	return ( Len > FIXTURE_EXPORT_PREFIX_MANGLED_CCH && 
		     0 == memcmp( 
				ExportName, 
				FIXTURE_EXPORT_PREFIX_MANGLED, 
				FIXTURE_EXPORT_PREFIX_MANGLED_CCH ) );
}

static CfixsUnmangleName(
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
		ExportName + FIXTURE_EXPORT_PREFIX_MANGLED_CCH,
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
	NewFixture->SetupRoutine = NULL;
	NewFixture->TeardownRoutine = NULL;
	NewFixture->Module = Module;
	NewFixture->TestCaseCount = 0;

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
				NewFixture->SetupRoutine = ( PVOID ) Entry->Routine;
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
				NewFixture->TeardownRoutine = ( PVOID ) Entry->Routine;
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
				( PVOID ) Entry->Routine;
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
		PtrFromRva( DosHeader, DosHeader->e_lfanew );
	if( IMAGE_NT_SIGNATURE != NtHeader->Signature )
	{
		Hr = HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
		goto Cleanup;
	}

	ExportDataDir = &NtHeader->OptionalHeader.DataDirectory
			[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
	ExportDir = ( PIMAGE_EXPORT_DIRECTORY ) PtrFromRva( 
		DosHeader, 
		ExportDataDir->VirtualAddress );

	NamesRvaArray = ( PDWORD ) PtrFromRva(
		DosHeader,
		ExportDir->AddressOfNames );

	FunctionRvaArray = ( PDWORD ) PtrFromRva(
		DosHeader,
		ExportDir->AddressOfFunctions );

	OrdinalsArray = ( PWORD ) PtrFromRva(
		DosHeader,
		ExportDir->AddressOfNameOrdinals );

	for ( Index = 0; Index < ExportDir->NumberOfNames; Index++ )
	{
		//
		// Get name of export.
		//
		PCSTR Name = ( PCSTR ) PtrFromRva(
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
			PVOID ExportedRoutine = ( PVOID ) PtrFromRva(
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
		// Inconclusiveness should have been reported. Testcase will not 
		// be resumed as it is inconclusive.
		//
		*AbortRun = ExcpPointers->ExceptionRecord->NumberParameters == 1;
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
		CFIX_TESTCASE_EXECUTION_EVENT Event;

		//
		// Notify.
		//
		Context->OnUnhandledException(
			Context,
			ExcpPointers );

		//
		// Report unhandled exception.
		//
		Event.Type = CfixEventUncaughtException;
		memcpy( 
			&Event.Info.UncaughtException,
			ExcpPointers->ExceptionRecord,
			sizeof( EXCEPTION_RECORD ) );

		Disp = Context->ReportEvent(
			Context,
			&Event );

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
	__in PCFIX_EXECUTION_CONTEXT Context,
	__out PBOOL RoutineRanToCompletion
	)
{
	HRESULT Hr;
	BOOL AbortRun = FALSE;
	PCFIX_EXECUTION_CONTEXT PrevContext;
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
	Hr = CfixsSetCurrentExecutionContext( Context, &PrevContext );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	__try
	{
		( Routine )();
		*RoutineRanToCompletion = TRUE;
	}
	__except ( CfixsExceptionFilter( 
		GetExceptionInformation(), 
		Context,
		&AbortRun ) )
	{
		*RoutineRanToCompletion = FALSE;
	}

	VERIFY( S_OK == CfixsSetCurrentExecutionContext( PrevContext, NULL ) );

	return ( AbortRun )
		? CFIX_E_TESTRUN_ABORTED
		: S_OK;
}

static HRESULT CfixsRunTestCaseMethod(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_TEST_CASE TestCase,
	__in PCFIX_EXECUTION_CONTEXT Context
	)
{
	BOOL RoutineRanToCompletion;

	if ( ! Fixture || 
		 ! TestCase || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	return CfixsRunTestRoutine( 
		( CFIX_PE_TESTCASE_ROUTINE ) TestCase->Routine,
		Context ,
		&RoutineRanToCompletion );

	//
	// N.B. Whether the routine ran to completion or triggered some
	// report does not matter here - execution is continued in any
	// case.
	//
}

static HRESULT CfixsRunSetupMethod(
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
	// Setup routines are optional.
	//
	if ( Fixture->SetupRoutine )
	{
		BOOL RoutineRanToCompletion;
		HRESULT Hr = CfixsRunTestRoutine( 
			( CFIX_PE_TESTCASE_ROUTINE ) Fixture->SetupRoutine,
			Context,
			&RoutineRanToCompletion );

		if ( SUCCEEDED( Hr ) && ! RoutineRanToCompletion )
		{
			//
			// If setup failed, continueing executing the fixture
			// is futile.
			//
			return CFIX_E_SETUP_ROUTINE_FAILED;
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
		BOOL RoutineRanToCompletion;
		HRESULT Hr = CfixsRunTestRoutine( 
			( CFIX_PE_TESTCASE_ROUTINE ) Fixture->TeardownRoutine,
			Context,
			&RoutineRanToCompletion );

		if ( SUCCEEDED( Hr ) && ! RoutineRanToCompletion )
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

/*----------------------------------------------------------------------
 * 
 * Privates.
 *
 */
BOOL CfixpSetupTls()
{
	CfixsTlsSlotForContext = TlsAlloc();
	return CfixsTlsSlotForContext != TLS_OUT_OF_INDEXES;
}

BOOL CfixpTeardownTls()
{
	return TlsFree( CfixsTlsSlotForContext );
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
	Module->Base.Routines.RunTestCase	= CfixsRunTestCaseMethod;
	Module->Base.Routines.Setup			= CfixsRunSetupMethod;
	Module->Base.Routines.Teardown		= CfixsRunTeardownMethod;
	Module->Base.Routines.Reference		= CfixsReferenceTestModuleMethod;
	Module->Base.Routines.Dereference	= CfixsDereferenceTestModuleMethod;

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
 * Report routines.
 *
 */

CFIXAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeReportFailedAssertion(
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in UINT Line,
	__in PCWSTR Expression
	)
{
	PCFIX_EXECUTION_CONTEXT Context;
	HRESULT Hr;
	CFIX_TESTCASE_EXECUTION_EVENT Event;
	CFIX_REPORT_DISPOSITION Disp;
	
	DWORD LastError = GetLastError();
	
	//
	// Context should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixsGetCurrentExecutionContext( &Context );
	if ( FAILED( Hr ) )
	{
		WCHAR Buffer[ 200 ] = { 0 };

		//
		// Failed assertion on unknown thread - there is little
		// we can do.
		//
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Failed assertion '%s' on unknown thread - terminating.",
			Expression );
		OutputDebugString( Buffer );
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	Event.Type								= CfixEventFailedAssertion;
	Event.Info.FailedAssertion.File			= File;
	Event.Info.FailedAssertion.Routine		= Routine;
	Event.Info.FailedAssertion.Line			= Line;
	Event.Info.FailedAssertion.Expression	= Expression;
	Event.Info.FailedAssertion.LastError	= LastError;

	Disp = Context->ReportEvent(
		Context,
		&Event );

	if ( Disp == CfixBreakAlways )
	{
		//
		// Will break, even if not running in a debugger.
		//
		return CfixBreak;
	}
	else if ( IsDebuggerPresent() && Disp != CfixAbort )
	{
		return Disp;
	}
	else
	{
		//
		// Throw exception to abort testcase. Will be catched by 
		// CfixsExceptionFilter installed a few frames deeper.
		//
		if ( Disp == CfixAbort )
		{
			ULONG_PTR Abort = TRUE;
			RaiseException(
				EXCEPTION_TESTCASE_FAILED,
				0,
				1,
				&Abort );
		}
		else
		{
			RaiseException(
				EXCEPTION_TESTCASE_FAILED,
				0,
				0,
				NULL );
		}

		ASSERT( !"Should not make it here!" );
		return Disp;
	}
}

CFIXAPI CFIX_REPORT_DISPOSITION CFIXCALLTYPE CfixPeAssertEqualsDword(
	__in DWORD Expected,
	__in DWORD Actual,
	__in PCWSTR File,
	__in PCWSTR Routine,
	__in UINT Line,
	__in PCWSTR Expression,
	__reserved DWORD Reserved
	)
{
	UNREFERENCED_PARAMETER( Reserved );

	if ( Expected == Actual )
	{
		return CfixContinue;
	}
	else
	{
		WCHAR Buffer[ 200 ] = { 0 };
		
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Comparison failed. Expected: 0x%08X, Actual: 0x%08X (Expression: %s)",
			Expected,
			Actual,
			Expression );

		return CfixPeReportFailedAssertion(
			File,
			Routine,
			Line,
			Buffer );
	}
}

CFIXAPI VOID CFIXCALLTYPE CfixPeReportInconclusiveness(
	__in PCWSTR Message
	)
{
	PCFIX_EXECUTION_CONTEXT Context;
	HRESULT Hr;
	CFIX_TESTCASE_EXECUTION_EVENT Event;
	
	//
	// Context should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixsGetCurrentExecutionContext( &Context );
	if ( FAILED( Hr ) )
	{
		WCHAR Buffer[ 200 ] = { 0 };

		//
		// Failed assertion on unknown thread - there is little
		// we can do.
		//
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Inconclusiveness report '%s' on unknown thread - terminating.",
			Message );
		OutputDebugString( Buffer );
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	//
	// Report inconclusiveness.
	//
	Event.Type							= CfixEventInconclusivess;
	Event.Info.Inconclusiveness.Message = Message;

	( VOID ) Context->ReportEvent(
		Context,
		&Event );

	//
	// Throw exception to abort testcase. Will be catched by 
	// CfixsExceptionFilter installed a few frames deeper.
	//
	RaiseException(
		EXCEPTION_TESTCASE_INCONCLUSIVE,
		0,
		0,
		NULL );

	ASSERT( !"Will not make it here" );
}

CFIXAPI VOID __cdecl CfixPeReportLog(
	__in PCWSTR Format,
	...
	)
{
	PCFIX_EXECUTION_CONTEXT Context;
	HRESULT Hr;
	CFIX_TESTCASE_EXECUTION_EVENT Event;
	WCHAR Message[ 512 ] = { 0 };
	va_list lst;

	//
	// Format message.
	//
	va_start( lst, Format );
	( VOID ) StringCchVPrintf(
		Message, 
		_countof( Message ),
		Format,
		lst );
	va_end( lst );
	
	//
	// Context should be in TLS as this routine should only be called
	// from within a test routine.
	//
	Hr = CfixsGetCurrentExecutionContext( &Context );
	if ( FAILED( Hr ) )
	{
		WCHAR Buffer[ 200 ] = { 0 };

		//
		// Log on unknown thread - there is little
		// we can do.
		//
		( VOID ) StringCchPrintf( 
			Buffer, 
			_countof( Buffer ),
			L"Log report '%s' on unknown thread - terminating.",
			Message );
		OutputDebugString( Buffer );
		ExitThread( CFIX_EXIT_THREAD_ABORTED );
	}

	//
	// Report log event.
	//
	Event.Type				= CfixEventLog;
	Event.Info.Log.Message	= Message;

	( VOID ) Context->ReportEvent(
		Context,
		&Event );
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
	VERIFY( S_OK == CfixsSetCurrentExecutionContext( Parameters->Context, NULL ) );

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

	VERIFY( S_OK == CfixsSetCurrentExecutionContext( NULL, NULL ) );
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
	Hr = CfixsGetCurrentExecutionContext( &CurrentContext );
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
	// housekeeping and crete the thread for us.
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

