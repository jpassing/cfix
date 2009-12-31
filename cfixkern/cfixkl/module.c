
/*----------------------------------------------------------------------
 *	Purpose:
 *		Loading of test modules.
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

#include "cfixklp.h"
#include <stdlib.h>
#include <shlwapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

#define CFIXKLP_ROUTINE_PRESENT	0x80000000

typedef struct _CFIXKLP_TEST_MODULE
{
	CFIX_TEST_MODULE Base;

	LONG References;
	SC_HANDLE DriverHandle;

	//
	// Indicates whether the driver has been installed/loaded on
	// behalf of the creator of this object.
	//
	BOOL DriverInstalled;
	BOOL DriverLoaded;

	//
	// Name of driver (!= image base name).
	//
	WCHAR DriverName[ 100 ];

	//
	// Load address of driver under test. Typed as ULONGLONG (rather
	// than ULONG_PTR) for WOW64.
	//
	ULONGLONG DriverBaseAddress;

	//
	// Handle to cfixkr.
	//
	HANDLE ReflectorHandle;
} CFIXKLP_TEST_MODULE, *PCFIXKLP_TEST_MODULE;

/*----------------------------------------------------------------------
 *
 * TLS.
 *
 */

static DWORD CfixklsTlsSlot = TLS_OUT_OF_INDEXES;

BOOL CfixklpSetupTestTls()
{
	CfixklsTlsSlot	= TlsAlloc();
	return ( CfixklsTlsSlot != TLS_OUT_OF_INDEXES );
}

VOID CfixklpTeardownTestTls()
{
	if ( CfixklsTlsSlot != TLS_OUT_OF_INDEXES )
	{
		TlsFree( CfixklsTlsSlot );
	}
}

static PULONGLONG CfixklsGetTlsValuePtr()
{
	PULONGLONG Ptr = ( PULONGLONG ) TlsGetValue( CfixklsTlsSlot );
	return Ptr;
}

static VOID CfixklsSetTlsValuePtr( 
	__in_opt PULONGLONG Ptr
	)
{
	TlsSetValue( CfixklsTlsSlot, Ptr );
}

/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */

/*++
	Routine Description:
		Derive a driver/service name from the driver's path.
--*/
HRESULT CfixklsGetDriverNameFromPath(
	__in PCWSTR DriverPath,
	__in SIZE_T NameSizeInChars,
	__out_ecount(NameSizeInChars) PWSTR Name
	)
{
	WCHAR DriverPathBuffer[ MAX_PATH ];
	HRESULT Hr;

	//
	// Get basename of driver file.
	//
	Hr = StringCchCopy(
		DriverPathBuffer,
		_countof( DriverPathBuffer ),
		DriverPath );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	PathStripPath( DriverPathBuffer );
	PathRemoveExtension( DriverPathBuffer );

	//
	// The driver/service will be cfixkr_<basename>.
	//
	return StringCchPrintf(
		Name,
		NameSizeInChars,
		L"cfixkr_%s",
		DriverPathBuffer );
}

/*----------------------------------------------------------------------
 *
 * Module initialization.
 *
 */
static HRESULT CfixklsCreateFixture( 
	__in PCFIXKRIO_TEST_MODULE IoModule,
	__in PCFIXKRIO_FIXTURE IoFixture, 
	__in PCFIXKLP_TEST_MODULE Module,
	__out PCFIX_FIXTURE *Fixture 
	)
{
	HRESULT Hr;
	ULONG Index;
	PCFIX_FIXTURE NewFixture;
	ULONG StringSizesTotalCb = 0;
	ULONG CurrentStringOffset;
	ULONG TestCaseCount = 0;
	ULONG TestCaseIndex = 0;

	ASSERT( IoModule );
	ASSERT( IoFixture );
	ASSERT( Module );
	ASSERT( Fixture );

	*Fixture = NULL;

	//
	// See how many test cases and string space we have to 
	// provide space for.
	//
	// CFIX_FIXTURE has a variable length array to hold the test cases.
	// Moreover, after this array, we need to store the strings for
	// the test case names.
	//
	for ( Index = 0; Index < IoFixture->EntryCount; Index++ )
	{
		if ( IoFixture->Entries[ Index ].Type == CfixEntryTypeTestcase )
		{
			TestCaseCount++;
			StringSizesTotalCb += 
				IoFixture->Entries[ Index ].NameLength + sizeof( WCHAR );
		}
	}

	NewFixture = ( PCFIX_FIXTURE ) malloc( 
		RTL_SIZEOF_THROUGH_FIELD( 
			CFIX_FIXTURE,
			TestCases[ TestCaseCount - 1 ] ) +
		StringSizesTotalCb );
	if ( NewFixture == NULL )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Let CurrentStringOffset hold the offset of the first byte after the 
	// end of the fixtures array.
	//
	CurrentStringOffset = RTL_SIZEOF_THROUGH_FIELD( 
		CFIX_FIXTURE,
		TestCases[ TestCaseCount - 1 ] );

	//
	// Name.
	//
	ASSERT( ( IoFixture->NameLength % 2 ) == 0 );
	if ( IoFixture->NameLength > sizeof( NewFixture->Name ) - sizeof( WCHAR ) )
	{
		Hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
		goto Cleanup;
	}

	CopyMemory( 
		NewFixture->Name, 
		( PUCHAR ) IoModule + IoFixture->NameOffset,
		IoFixture->NameLength );
	NewFixture->Name[ IoFixture->NameLength / 2 ] = L'\0';

	NewFixture->SetupRoutine	= 0;
	NewFixture->TeardownRoutine = 0;
	NewFixture->BeforeRoutine	= 0;
	NewFixture->AfterRoutine	= 0;
	NewFixture->Reserved		= IoFixture->Key;
	NewFixture->Module			= &Module->Base;
	NewFixture->TestCaseCount	= TestCaseCount;
	NewFixture->ApiType			= CfixApiTypeBase;
	NewFixture->Flags			= 0;	// Not supported for KM.

	//
	// Iterate over entries and create setup/teardown/testcases.
	//
	Hr = S_OK;
	for ( Index = 0; Index < IoFixture->EntryCount; Index++ )
	{
		PWSTR TestCaseName;

		//
		// N.B. Index walks over IoFixture, which includes setup
		// and teardown entries.
		//
		switch ( IoFixture->Entries[ Index ].Type )
		{
		case CfixEntryTypeTestcase:
			NewFixture->TestCases[ TestCaseIndex ].Routine = 
				( ULONG_PTR ) IoFixture->Entries[ Index ].Key;

			TestCaseName = ( PWSTR )
				( ( PUCHAR ) NewFixture + CurrentStringOffset );
			NewFixture->TestCases[ TestCaseIndex ].Name = TestCaseName;

			CopyMemory( 
				TestCaseName, 
				( PUCHAR ) IoModule + IoFixture->Entries[ Index ].NameOffset,
				IoFixture->Entries[ Index ].NameLength );

			//
			// Zero-terminate it.
			//
			TestCaseName[ IoFixture->Entries[ Index ].NameLength / 2 ] = L'\0';

			//
			// Adjust CurrentStringOffset for next string.
			//
			CurrentStringOffset += 
				IoFixture->Entries[ Index ].NameLength + sizeof( WCHAR );

			NewFixture->TestCases[ TestCaseIndex ].Fixture = NewFixture;

			TestCaseIndex++;
			break;

		case CfixEntryTypeSetup:
			if ( NewFixture->SetupRoutine & CFIXKLP_ROUTINE_PRESENT )
			{
				//
				// There is a setup routine already.
				//
				Hr = CFIX_E_DUP_SETUP_ROUTINE;
			}
			else
			{
				NewFixture->SetupRoutine = 
					( ULONG_PTR ) IoFixture->Entries[ Index ].Key |
					CFIXKLP_ROUTINE_PRESENT;
			}
			break;

		case CfixEntryTypeTeardown:
			if ( NewFixture->TeardownRoutine & CFIXKLP_ROUTINE_PRESENT )
			{
				//
				// There is a teardown routine already.
				//
				Hr = CFIX_E_DUP_TEARDOWN_ROUTINE;
			}
			else
			{
				NewFixture->TeardownRoutine = 
					( ULONG_PTR ) IoFixture->Entries[ Index ].Key |
					CFIXKLP_ROUTINE_PRESENT;
			}
			break;

		case CfixEntryTypeBefore:
			if ( NewFixture->BeforeRoutine & CFIXKLP_ROUTINE_PRESENT )
			{
				//
				// There is a before-routine already.
				//
				Hr = CFIX_E_DUP_BEFORE_ROUTINE;
			}
			else
			{
				NewFixture->BeforeRoutine = 
					( ULONG_PTR ) IoFixture->Entries[ Index ].Key |
					CFIXKLP_ROUTINE_PRESENT;
			}
			break;

		case CfixEntryTypeAfter:
			if ( NewFixture->AfterRoutine & CFIXKLP_ROUTINE_PRESENT )
			{
				//
				// There is an after-routine already.
				//
				Hr = CFIX_E_DUP_AFTER_ROUTINE;
			}
			else
			{
				NewFixture->AfterRoutine = 
					( ULONG_PTR ) IoFixture->Entries[ Index ].Key |
					CFIXKLP_ROUTINE_PRESENT;
			}
			break;

		default:
			Hr = CFIXKL_E_INVALID_REFLECTOR_RESPONSE;
			break;
		}

		if ( FAILED( Hr ) )
		{
			break;
		}
	}

	if ( SUCCEEDED( Hr ) )
	{
		ASSERT( TestCaseIndex == TestCaseCount );
		ASSERT( CurrentStringOffset == RTL_SIZEOF_THROUGH_FIELD( 
					CFIX_FIXTURE,
					TestCases[ TestCaseCount - 1 ] ) + StringSizesTotalCb );

		*Fixture = NewFixture;
	}

Cleanup:
	if ( FAILED( Hr ) )
	{
		free( NewFixture );
	}

	return Hr;
}

static VOID CfixklsDeleteFixture(
	__in PCFIX_FIXTURE Fixture
	)
{
	ASSERT( Fixture );
	free( Fixture );
}

static HRESULT CfixklsInitializeModule(
	__in PCWSTR DriverPath,
	__in PCFIXKLP_TEST_MODULE Module
	)
{
	HRESULT Hr;
	ULONG Index;
	PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE QueryResponse;

	ASSERT( DriverPath );
	ASSERT( Module );

	//
	// Connect to cfixkr and open a handle.
	//
	Module->ReflectorHandle = CreateFile(
		L"\\\\.\\Cfixkr",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL );
	if ( Module->ReflectorHandle == INVALID_HANDLE_VALUE )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	//
	// Query information about module.
	//
	Hr = CfixklpQueryModule(
		Module->ReflectorHandle,
		DriverPath,
		&Module->DriverBaseAddress,
		&QueryResponse );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	ASSERT( QueryResponse != NULL );
	ASSERT( Module->DriverBaseAddress != 0 );

	//
	// Allocate fixtures array and load fixtures.
	//
	Module->Base.FixtureCount = QueryResponse->u.TestModule.FixtureCount;
	Module->Base.Fixtures = ( PCFIX_FIXTURE* )
		malloc( sizeof( PCFIX_FIXTURE ) * Module->Base.FixtureCount );
	if ( Module->Base.Fixtures == NULL )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	ZeroMemory( 
		Module->Base.Fixtures, 
		sizeof( PCFIX_FIXTURE ) * Module->Base.FixtureCount );

	Hr = S_OK;
	for ( Index = 0; Index < Module->Base.FixtureCount; Index++ )
	{
		PCFIXKRIO_FIXTURE IoFixture = ( PCFIXKRIO_FIXTURE )
			( ( ( PUCHAR ) QueryResponse ) + 
			QueryResponse->u.TestModule.FixtureOffsets[ Index ] );

		Hr = CfixklsCreateFixture( 
			&QueryResponse->u.TestModule,
			IoFixture, 
			Module,
			&Module->Base.Fixtures[ Index ] );
		if ( FAILED( Hr ) )
		{
			break;
		}
	}

Cleanup:
	if ( FAILED( Hr ) )
	{
		//
		// Free fixtures.
		//
		if ( Module->Base.Fixtures )
		{
			for ( Index = 0; Index < Module->Base.FixtureCount; Index++ )
			{
				if ( Module->Base.Fixtures[ Index ] != NULL )
				{
					CfixklsDeleteFixture( Module->Base.Fixtures[ Index ] );
				}
			}

			free( Module->Base.Fixtures );
			Module->Base.Fixtures = NULL;
		}
	}

	CfixklpFreeQueryModuleResponse( QueryResponse );

	return Hr;
}

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static VOID CfixklsDeleteModule(
	__in PCFIXKLP_TEST_MODULE Module
	)
{
	if ( Module )
	{
		//
		// Free fixtures.
		//
		if ( Module->Base.Fixtures != NULL )
		{
			ULONG Index;

			for ( Index = 0; Index < Module->Base.FixtureCount; Index++ )
			{
				if ( Module->Base.Fixtures[ Index ] != NULL )
				{
					CfixklsDeleteFixture( Module->Base.Fixtures[ Index ] );
				}
			}

			free( Module->Base.Fixtures );
		}

		if ( Module->ReflectorHandle != NULL &&
			  Module->ReflectorHandle != INVALID_HANDLE_VALUE )
		{
			VERIFY( CloseHandle( Module->ReflectorHandle ) );
		}
		
		if ( Module->DriverHandle != NULL )
		{
			if ( Module->DriverLoaded )
			{
				VERIFY( SUCCEEDED( CfixklpStopDriverAndCloseHandle(
					Module->DriverHandle,
					Module->DriverInstalled ) ) );
			}
			else
			{
				VERIFY( CloseServiceHandle( Module->DriverHandle ) );
			}
		}

		free( Module );
	}
}

static VOID CfixklsReferenceModule(
	__in PCFIX_TEST_MODULE This
	)
{
	PCFIXKLP_TEST_MODULE Module = CONTAINING_RECORD(
		This,
		CFIXKLP_TEST_MODULE,
		Base );

	ASSERT( Module );
	if ( Module )
	{
		InterlockedIncrement( &Module->References );
	}
}

static VOID CfixklsDereferenceModule(
	__in PCFIX_TEST_MODULE This
	)
{
	PCFIXKLP_TEST_MODULE Module = CONTAINING_RECORD(
		This,
		CFIXKLP_TEST_MODULE,
		Base );

	ASSERT( Module );
	if ( Module )
	{
		if ( 0 == InterlockedDecrement( &Module->References ) )
		{
			CfixklsDeleteModule( Module );
		}
	}
}

static HRESULT CfixklsRunSetupRoutine(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG Flags
	)
{
	PCFIXKLP_TEST_MODULE Module;
	PULONGLONG TlsValuePtr;

	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	UNREFERENCED_PARAMETER( Flags );

	Module = CONTAINING_RECORD(
		Fixture->Module,
		CFIXKLP_TEST_MODULE,
		Base );

	//
	// Whether we have a setup routine or not, make sure that
	// TlsValue is 0 for the first call on this thread.
	//
	ASSERT( NULL == CfixklsGetTlsValuePtr() );

	TlsValuePtr = ( PULONGLONG ) malloc( sizeof( ULONGLONG ) );
	if ( TlsValuePtr == NULL )
	{
		return E_OUTOFMEMORY;
	}

	*TlsValuePtr = 0;
	CfixklsSetTlsValuePtr( TlsValuePtr );

	//
	// N.B. We may not have a setup routine. To distinguish nonexisting
	// routine from a routine with key 0, the value is ORed with
	// CFIXKLP_ROUTINE_PRESENT.
	//
	if ( Fixture->SetupRoutine & Fixture->SetupRoutine )
	{
		BOOL Abort;
		BOOL RanToCompletion;

		HRESULT Hr;
		USHORT RoutineKey = ( USHORT ) 
			( Fixture->SetupRoutine & ~CFIXKLP_ROUTINE_PRESENT );

		Hr = CfixklpCallRoutine(
			Module->ReflectorHandle,
			Module->DriverBaseAddress,
			Context,
			( USHORT ) Fixture->Reserved,		// Contains FixtureKey
			RoutineKey,
			&RanToCompletion,
			&Abort,
			TlsValuePtr );

		//
		// Reset TLS.
		//
		*TlsValuePtr = 0;

		if ( SUCCEEDED( Hr ) && Abort )
		{
			return CFIX_E_TESTRUN_ABORTED;
		}
		else if ( SUCCEEDED( Hr ) && ! RanToCompletion )
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

static HRESULT CfixklsRunTeardownRoutine(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG Flags
	)
{
	PCFIXKLP_TEST_MODULE Module;
	PULONGLONG TlsValuePtr;
	ULONGLONG TlsValueCopy;

	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	UNREFERENCED_PARAMETER( Flags );

	Module = CONTAINING_RECORD(
		Fixture->Module,
		CFIXKLP_TEST_MODULE,
		Base );

	//
	// Get TlsValue and free slot.
	//
	TlsValuePtr = CfixklsGetTlsValuePtr();
	ASSERT( TlsValuePtr != NULL );
	__assume( TlsValuePtr != NULL );

	TlsValueCopy = *TlsValuePtr;

	free( TlsValuePtr );
	CfixklsSetTlsValuePtr( NULL );

	//
	// N.B. We may not have a teardown routine. To distinguish nonexisting
	// routine from a routine with key 0, the value is ORed with
	// CFIXKLP_ROUTINE_PRESENT.
	//

	if ( Fixture->TeardownRoutine & CFIXKLP_ROUTINE_PRESENT )
	{
		BOOL Abort;
		BOOL RanToCompletion;

		HRESULT Hr;
		USHORT RoutineKey = ( USHORT ) 
			( Fixture->TeardownRoutine & ~CFIXKLP_ROUTINE_PRESENT );

		Hr = CfixklpCallRoutine(
			Module->ReflectorHandle,
			Module->DriverBaseAddress,
			Context,
			( USHORT ) Fixture->Reserved,		// Contains FixtureKey
			RoutineKey,
			&RanToCompletion,
			&Abort,
			&TlsValueCopy );

		if ( SUCCEEDED( Hr ) && Abort )
		{
			return CFIX_E_TESTRUN_ABORTED;
		}
		else if ( SUCCEEDED( Hr ) && ! RanToCompletion )
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

static HRESULT CfixklsRunBeforeOrAfterRoutine(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG_PTR RoutineTag,
	__in HRESULT FailureHr,
	__in BOOL ResetTlsAfterCall
	)
{
	PULONGLONG TlsValuePtr;
	PCFIXKLP_TEST_MODULE Module;
	
	Module = CONTAINING_RECORD(
		Fixture->Module,
		CFIXKLP_TEST_MODULE,
		Base );

	TlsValuePtr = CfixklsGetTlsValuePtr();
	ASSERT( TlsValuePtr != NULL );
	__assume( TlsValuePtr != NULL );

	//
	// N.B. We may not have a before/after routine. To distinguish nonexisting
	// routine from a routine with key 0, the value is ORed with
	// CFIXKLP_ROUTINE_PRESENT.
	//

	if ( RoutineTag & CFIXKLP_ROUTINE_PRESENT )
	{
		BOOL Abort;
		BOOL RanToCompletion;

		HRESULT Hr;
		USHORT RoutineKey = ( USHORT ) 
			( RoutineTag & ~CFIXKLP_ROUTINE_PRESENT );

		Hr = CfixklpCallRoutine(
			Module->ReflectorHandle,
			Module->DriverBaseAddress,
			Context,
			( USHORT ) Fixture->Reserved,		// Contains FixtureKey
			RoutineKey,
			&RanToCompletion,
			&Abort,
			TlsValuePtr );

		if ( ResetTlsAfterCall )
		{
			*TlsValuePtr = 0;
		}

		if ( SUCCEEDED( Hr ) && Abort )
		{
			return CFIX_E_TESTRUN_ABORTED;
		}
		else if ( SUCCEEDED( Hr ) && ! RanToCompletion )
		{
			return FailureHr;
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

static HRESULT CfixklsRunBeforeRoutine(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG Flags
	)
{
	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	UNREFERENCED_PARAMETER( Flags );

	return CfixklsRunBeforeOrAfterRoutine(
		Fixture,
		Context,
		Fixture->BeforeRoutine,
		CFIX_E_BEFORE_ROUTINE_FAILED,
		FALSE );
}

static HRESULT CfixklsRunAfterRoutine(
	__in PCFIX_FIXTURE Fixture,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG Flags
	)
{
	if ( ! Fixture || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	UNREFERENCED_PARAMETER( Flags );

	return CfixklsRunBeforeOrAfterRoutine(
		Fixture,
		Context,
		Fixture->AfterRoutine,
		CFIX_E_AFTER_ROUTINE_FAILED,
		TRUE );
}


static HRESULT CfixklsRunTestCaseRoutine(
	__in PCFIX_TEST_CASE TestCase,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in ULONG Flags
	)
{
	HRESULT Hr;
	PCFIXKLP_TEST_MODULE Module;
	PULONGLONG TlsValuePtr;

	BOOL Abort;
	BOOL RanToCompletion;

	if ( ! TestCase || 
		 ! Context ||
		 Context->Version != CFIX_TEST_CONTEXT_VERSION )
	{
		return E_INVALIDARG;
	}

	UNREFERENCED_PARAMETER( Flags );

	Module = CONTAINING_RECORD(
		TestCase->Fixture->Module,
		CFIXKLP_TEST_MODULE,
		Base );

	TlsValuePtr = CfixklsGetTlsValuePtr();
	ASSERT( TlsValuePtr != NULL );
	__assume( TlsValuePtr != NULL );

	Hr = CfixklpCallRoutine(
		Module->ReflectorHandle,
		Module->DriverBaseAddress,
		Context,
		( USHORT ) TestCase->Fixture->Reserved,		// Contains FixtureKey
		( USHORT ) TestCase->Routine,
		&RanToCompletion,
		&Abort,
		TlsValuePtr );

	//
	// N.B. Whether the routine ran to completion or triggered some
	// report does not matter here.
	//

	if ( SUCCEEDED( Hr ) && Abort )
	{
		return CFIX_E_TESTRUN_ABORTED;
	}
	else if ( SUCCEEDED( Hr ) && ! RanToCompletion )
	{
		return CFIX_E_TEST_ROUTINE_FAILED;
	}
	else
	{
		return Hr;
	}
}

/*----------------------------------------------------------------------
 *
 * Exports.
 *
 */
HRESULT CFIXCALLTYPE CfixklCreateTestModuleFromDriver(
	__in PCWSTR DriverPath,
	__out PCFIX_TEST_MODULE *TestModule,
	__out_opt PBOOL Installed,
	__out_opt PBOOL Loaded
	)
{
	HRESULT Hr;
	PCFIXKLP_TEST_MODULE NewModule;

	if ( ! DriverPath || 
		 ! TestModule )
	{
		return E_INVALIDARG;
	}

	*TestModule	= NULL;

	NewModule = ( PCFIXKLP_TEST_MODULE ) malloc( sizeof( CFIXKLP_TEST_MODULE ) );
	if ( ! NewModule )
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory( NewModule, sizeof( CFIXKLP_TEST_MODULE ) );

	Hr = CfixklsGetDriverNameFromPath(
		DriverPath,
		_countof( NewModule->DriverName ),
		NewModule->DriverName );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	//
	// Start cfixkr - it might not have been started yet.
	//
	Hr = CfixklpStartCfixKernelReflector();
	if ( HRESULT_FROM_WIN32( ERROR_SERVICE_ALREADY_RUNNING ) == Hr )
	{
		NOP;
	}
	else if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}
	
	//
	// Start the actual driver under test.
	//
	Hr = CfixklpStartDriver(
		DriverPath,
		NewModule->DriverName,
		NewModule->DriverName,
		&NewModule->DriverInstalled,
		&NewModule->DriverLoaded,
		&NewModule->DriverHandle );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	//
	// Now that the driver is running, contact it and initializte
	// the module.
	//
	Hr = CfixklsInitializeModule(
		DriverPath,
		NewModule );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	ASSERT( memcmp( NewModule->DriverName, L"cfixkr_", 14 ) == 0 );

	NewModule->Base.Version					= CFIX_TEST_MODULE_VERSION;
	NewModule->Base.Name					= &NewModule->DriverName[ 7 ];
	NewModule->Base.Routines.Reference		= CfixklsReferenceModule;
	NewModule->Base.Routines.Dereference	= CfixklsDereferenceModule;
	NewModule->Base.Routines.Setup			= CfixklsRunSetupRoutine;
	NewModule->Base.Routines.Teardown		= CfixklsRunTeardownRoutine;
	NewModule->Base.Routines.Before			= CfixklsRunBeforeRoutine;
	NewModule->Base.Routines.After			= CfixklsRunAfterRoutine;
	NewModule->Base.Routines.RunTestCase	= CfixklsRunTestCaseRoutine;
	NewModule->Base.Routines.GetInformationStackFrame = NULL;

	NewModule->References					= 1;

	if ( Installed )	*Installed	= NewModule->DriverInstalled;
	if ( Loaded )		*Loaded		= NewModule->DriverLoaded;

	*TestModule = &NewModule->Base;
	Hr = S_OK;
	
Cleanup:
	if ( FAILED( Hr ) )
	{
		CfixklsDeleteModule( NewModule );
	}

	return Hr;
}