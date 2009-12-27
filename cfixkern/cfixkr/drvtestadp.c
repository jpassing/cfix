/*----------------------------------------------------------------------
 *	Purpose:
 *		Driver Test Interface Adapter.
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
#include <ntimage.h>
#include "cfixkrp.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <ntstrsafe.h>
#pragma warning( pop )

#define CFIXKRP_DRIVER_TEST_ADAPTER_SIGNATUE 'ftIT'

//
// Maximal length of an entry name, including NULL terminator.
//
#define CFIX_MAX_ENTRY_NAME_CCH 64

/*++
	N.B. This structure is immutable once it has been initialized.
--*/
typedef struct _CFIXKRP_TEST_FIXTURE
{
	UNICODE_STRING FixtureName;
	struct
	{
		ULONG EntryCount;
		PCFIX_PE_DEFINITION_ENTRY Entries;
	} Definition;

	WCHAR __NameBuffer[ CFIX_MAX_FIXTURE_NAME_CCH ];
} CFIXKRP_TEST_FIXTURE, *PCFIXKRP_TEST_FIXTURE;

/*++
	N.B. This structure is immutable once it has been initialized.
--*/
typedef struct _CFIXKRP_TEST_ADAPTER
{
	ULONG Signature;

	//
	// Information ontained via PE inspection.
	//
	struct
	{
		ULONG EntryCount;
		PCFIXKRP_TEST_FIXTURE Entries;
	} Fixtures;
} CFIXKRP_TEST_ADAPTER, *PCFIXKRP_TEST_ADAPTER;

/*----------------------------------------------------------------------
 *
 * Helpers.
 *
 */

static BOOLEAN CfixkrsIsFixtureExport(
	__in PCSTR ExportName 
	)
{
	//
	// Check if name begins with FIXTURE_EXPORT_PREFIX.
	//
	size_t Len = strlen( ExportName );

	return ( Len > CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH && 
		     0 == memcmp( 
				ExportName, 
				CFIX_FIXTURE_EXPORT_PREFIX_MANGLED, 
				CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH ) ) ? TRUE : FALSE;
}

static VOID CfixkrsUnmangleName(
	__in PUNICODE_STRING Name 
	)
{
	//
	// The leading '_Cfix...' has already been removed, remove the trailing
	// @N.
	//
	USHORT Index;
	ASSERT( ( Name->Length % 2 ) == 0 );
	for ( Index = 0; Index < Name->Length / 2; Index++ )
	{
		if ( Name->Buffer[ Index ] == L'@' )
		{
			Name->Length = Index * sizeof( WCHAR );
			break;
		}
	}
}

static NTSTATUS CfixkrsInitializeFixture(
	__in PCSTR ExportName,
	__in CFIX_GET_FIXTURE_ROUTINE GetFixtureRoutine,
	__out PCFIXKRP_TEST_FIXTURE Fixture
	)
{
	ULONG EntryCount = 0;
	ANSI_STRING FixtureNameAnsi;
	NTSTATUS Status;
	PCFIX_TEST_PE_DEFINITION TestDef;
	
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// Fixture name.
	//
	RtlInitAnsiString( 
		&FixtureNameAnsi, 
		ExportName + CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH );

	Fixture->FixtureName.Length			= 0;
	Fixture->FixtureName.MaximumLength	= sizeof( Fixture->__NameBuffer );
	Fixture->FixtureName.Buffer			= Fixture->__NameBuffer;
	
	Status = RtlAnsiStringToUnicodeString(
		&Fixture->FixtureName,
		&FixtureNameAnsi,
		FALSE );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	CfixkrsUnmangleName( &Fixture->FixtureName );

	//
	// Definition. Obtain from DLL.
	//
	TestDef = ( GetFixtureRoutine )();
	if ( ! TestDef )
	{
		//
		// TODO: Log to event log.
		//
		return STATUS_DRIVER_UNABLE_TO_LOAD;
	}

	//
	// N.B. Using legacy ApiVersion -- no alternate API types or
	// flags supported.
	//
	if ( TestDef->Head.ApiVersion != CFIX_PE_API_VERSION )
	{
		//
		// TODO: Log to event log.
		//
		return STATUS_DRIVER_UNABLE_TO_LOAD;
	}

	//
	// Count entries (includes Setup and Teardown if available).
	//
	while ( TestDef->Entries[ EntryCount ].Type != CfixEntryTypeEnd )
	{
		EntryCount++;
	}

	Fixture->Definition.EntryCount	= EntryCount;
	Fixture->Definition.Entries		= TestDef->Entries;

	return STATUS_SUCCESS;
}

static NTSTATUS CfixkrsWalkExportDirectoryAndExtractFixtures(
	__in ULONGLONG LoadAddress,
	__in PIMAGE_DATA_DIRECTORY ExportDataDir,
	__in PIMAGE_EXPORT_DIRECTORY ExportDirectory,
	__in PCFIXKRP_TEST_ADAPTER Adapter
	)
{
	PULONG FunctionRvaArray;
	PULONG NamesRvaArray;
	PUSHORT OrdinalsArray;
	NTSTATUS Status;

	ULONG Index;
	
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
	ASSERT( LoadAddress != 0 );
	ASSERT( Adapter );

	if ( ExportDirectory->AddressOfNames == 0 ||
		 ExportDirectory->AddressOfFunctions == 0 ||
		 ExportDirectory->AddressOfNameOrdinals == 0 )
	{
		//
		// This driver does not have any exports.
		//
		return STATUS_SUCCESS;
	}

	NamesRvaArray = ( PULONG ) CfixPtrFromRva(
		LoadAddress,
		ExportDirectory->AddressOfNames );

	FunctionRvaArray = ( PULONG ) CfixPtrFromRva(
		LoadAddress,
		ExportDirectory->AddressOfFunctions );

	OrdinalsArray = ( PUSHORT ) CfixPtrFromRva(
		LoadAddress,
		ExportDirectory->AddressOfNameOrdinals );

	Status = STATUS_SUCCESS;
	
	for ( Index = 0; Index < ExportDirectory->NumberOfNames; Index++ )
	{
		//
		// Get name of export.
		//
		PCSTR Name = ( PCSTR ) CfixPtrFromRva(
			LoadAddress,
			NamesRvaArray[ Index ] );

		//
		// Get corresponding export ordinal.
		//
		USHORT Ordinal = ( USHORT ) OrdinalsArray[ Index ] 
			+ ( USHORT ) ExportDirectory->Base;

		//
		// Get corresponding function RVA.
		//
		ULONG FuncRva = FunctionRvaArray[ Ordinal - ExportDirectory->Base ];

		if ( FuncRva >= ExportDataDir->VirtualAddress && 
			 FuncRva <  ExportDataDir->VirtualAddress + ExportDataDir->Size )
		{
			//
			// It is a forwarder.
			//
		}
		else if ( CfixkrsIsFixtureExport( Name ) )
		{
			CFIX_GET_FIXTURE_ROUTINE ExportedRoutine;
			PVOID ExportedRoutinePtr;

			//
			// That will become a fixture.
			//
			ExportedRoutinePtr = ( PVOID ) CfixPtrFromRva(
				LoadAddress,
				FuncRva );

			#pragma warning( push )
			#pragma warning( disable: 4055 )
			ExportedRoutine = ( CFIX_GET_FIXTURE_ROUTINE ) ExportedRoutinePtr;
			#pragma warning( pop )

			//
			// N.B. Adapter->Fixtures.Entries is guaranteed to
			// be large enough.
			//

			Status = CfixkrsInitializeFixture(
				Name,
				ExportedRoutine,
				&Adapter->Fixtures.Entries[ Adapter->Fixtures.EntryCount ] );
			if ( NT_SUCCESS( Status ) )
			{
				Adapter->Fixtures.EntryCount++;
			}
			else
			{
				break;
			}
		}
	}

	return Status;
}

static NTSTATUS CfixkrsInitializeAdapter(
	__in ULONGLONG LoadAddress,
	__in PCFIXKRP_TEST_ADAPTER Adapter
	)
{
	NTSTATUS Status;

	PIMAGE_DOS_HEADER DosHeader = ( PIMAGE_DOS_HEADER ) LoadAddress;
	PIMAGE_NT_HEADERS NtHeader; 
	PIMAGE_DATA_DIRECTORY ExportDataDir;
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;

	ASSERT( LoadAddress != 0 );
	ASSERT( Adapter );
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	if ( ! DosHeader )
	{
		return STATUS_INVALID_PARAMETER;
	}

	//
	// Peek into PE image to obtain exports.
	//
	NtHeader = ( PIMAGE_NT_HEADERS ) 
		CfixPtrFromRva( DosHeader, DosHeader->e_lfanew );
	if( IMAGE_NT_SIGNATURE != NtHeader->Signature )
	{
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	ExportDataDir = &NtHeader->OptionalHeader.DataDirectory
			[ IMAGE_DIRECTORY_ENTRY_EXPORT ];

	ExportDirectory = ( PIMAGE_EXPORT_DIRECTORY ) CfixPtrFromRva( 
		LoadAddress, 
		ExportDataDir->VirtualAddress );

	//
	// Begin initializing structure.
	//
	RtlZeroMemory( Adapter, sizeof( CFIXKRP_TEST_ADAPTER ) );
	Adapter->Signature = CFIXKRP_DRIVER_TEST_ADAPTER_SIGNATUE;

	//
	// Fixtures.
	//
	// It is likely that all exports are cfix-related, so it is ok
	// to allocate ExportDirecory->NumberOfNames fixtures, although
	// that may be too much.
	//
	Adapter->Fixtures.EntryCount = 0;
	Adapter->Fixtures.Entries = ( PCFIXKRP_TEST_FIXTURE )
		ExAllocatePoolWithTag( 
			PagedPool, 
			ExportDirectory->NumberOfNames * sizeof( CFIXKRP_TEST_FIXTURE ),
			CFIXKR_POOL_TAG );
	if ( ! Adapter->Fixtures.Entries )
	{
		return STATUS_NO_MEMORY;
	}

	Status = CfixkrsWalkExportDirectoryAndExtractFixtures(
		LoadAddress, 
		ExportDataDir,
		ExportDirectory,
		Adapter );
	if ( ! NT_SUCCESS( Status ) )
	{
		ExFreePoolWithTag( Adapter->Fixtures.Entries, CFIXKR_POOL_TAG );
	}
	
	return Status;
}

static NTSTATUS CfixkrsCalculateBufferSize(
	__in PCFIXKRP_TEST_ADAPTER Adapter,
	__out PULONG RequiredStructSize,
	__out PULONG RequiredStringsSize
	)
{
	PCFIXKRP_TEST_FIXTURE CurrentFixture;
	ULONG FixIndex;
	NTSTATUS Status;
	ULONG StructSize;
	ULONG StringsSize;

	ASSERT( Adapter );
	ASSERT( Adapter->Signature == CFIXKRP_DRIVER_TEST_ADAPTER_SIGNATUE );
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// N.B. No need to use safe int operations here.
	//
	StructSize = RTL_SIZEOF_THROUGH_FIELD( 
		CFIXKRIO_TEST_MODULE,
		FixtureOffsets[ Adapter->Fixtures.EntryCount - 1 ] );
	StringsSize = 0;

	for ( FixIndex = 0; FixIndex < Adapter->Fixtures.EntryCount; FixIndex++ )
	{
		ULONG DefIndex;
		CurrentFixture = &Adapter->Fixtures.Entries[ FixIndex ];

		//
		// Fixture.
		//
		StructSize += RTL_SIZEOF_THROUGH_FIELD( 
			CFIXKRIO_FIXTURE,
			Entries[ CurrentFixture->Definition.EntryCount - 1 ] );
		StringsSize += CurrentFixture->FixtureName.Length;

		//
		// Entries.
		//
		for ( DefIndex = 0; DefIndex < CurrentFixture->Definition.EntryCount; DefIndex++ )
		{
			size_t StringLen;
			Status = RtlStringCbLengthW(
				CurrentFixture->Definition.Entries[ DefIndex ].Name,
				( CFIX_MAX_ENTRY_NAME_CCH - 1 ) * sizeof( WCHAR ),
				&StringLen );
			if ( ! NT_SUCCESS( Status ) )
			{
				return Status;
			}
			ASSERT( StringLen > 0 );

			StringsSize += ( ULONG ) StringLen;
		}
	}

	*RequiredStructSize = StructSize;
	*RequiredStringsSize = StringsSize;

	return STATUS_SUCCESS;
}
/*----------------------------------------------------------------------
 *
 * Internals.
 *
 */
NTSTATUS CfixkrpCreateTestAdapter(
	__in ULONGLONG DriverLoadAddress,
	__out PCFIXKRP_TEST_ADAPTER *Adapter
	)
{
	PCFIXKRP_TEST_ADAPTER TempAdapter;
	NTSTATUS Status;

	ASSERT( DriverLoadAddress != 0 );
	ASSERT( Adapter );
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	TempAdapter = ( PCFIXKRP_TEST_ADAPTER ) ExAllocatePoolWithTag( 
		PagedPool, 
		sizeof( CFIXKRP_TEST_ADAPTER ),
		CFIXKR_POOL_TAG );
	if ( ! TempAdapter )
	{
		return STATUS_NO_MEMORY;
	}

	Status = CfixkrsInitializeAdapter(
		DriverLoadAddress,
		TempAdapter );
	if ( NT_SUCCESS( Status ) )
	{
		*Adapter = TempAdapter;
	}
	else
	{
		ExFreePoolWithTag( TempAdapter, CFIXKR_POOL_TAG );
	}

	return Status;
}

VOID CfixkrpDeleteTestAdapter(
	__in PCFIXKRP_TEST_ADAPTER Adapter
	)
{
	ASSERT( Adapter );
	ASSERT( Adapter->Signature == CFIXKRP_DRIVER_TEST_ADAPTER_SIGNATUE );
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

	ExFreePoolWithTag( Adapter->Fixtures.Entries, CFIXKR_POOL_TAG );
	ExFreePoolWithTag( Adapter, CFIXKR_POOL_TAG );
}

NTSTATUS CfixkrpGetRoutineTestAdapter(
	__in PCFIXKRP_TEST_ADAPTER Adapter,
	__in USHORT FixtureKey,
	__in USHORT RoutineKey,
	__out CFIX_PE_TESTCASE_ROUTINE *Routine
	)
{
	PCFIXKRP_TEST_FIXTURE Fixture;

	ASSERT( Adapter );
	ASSERT( Adapter->Signature == CFIXKRP_DRIVER_TEST_ADAPTER_SIGNATUE );
	ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL );

	if ( FixtureKey >= Adapter->Fixtures.EntryCount )
	{
		return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
	}

	Fixture = &Adapter->Fixtures.Entries[ FixtureKey ];
	ASSERT( Fixture != NULL );

	if ( RoutineKey >= Fixture->Definition.EntryCount )
	{
		return STATUS_DRIVER_ENTRYPOINT_NOT_FOUND;
	}

	*Routine = Fixture->Definition.Entries[ RoutineKey ].Routine;
	return STATUS_SUCCESS;
}

NTSTATUS CfixkrpQueryModuleTestAdapter(
	__in PCFIXKRP_TEST_ADAPTER Adapter,
	__in ULONG MaximumBufferSize,
	__out PUCHAR IoBuffer,
	__out PULONG BufferSize
	)
{
	NTSTATUS Status;
	ULONG StructSize;
	ULONG StringsSize;

	ULONG BufferStructOffset;
	ULONG BufferStringsOffset;

	USHORT FixIndex;
	PCFIXKRP_TEST_FIXTURE CurrentFixture;

	PCFIXKRIO_TEST_MODULE IoModule;
	PULONG FixtureOffsetsArray;

	ASSERT( Adapter );
	ASSERT( Adapter->Signature == CFIXKRP_DRIVER_TEST_ADAPTER_SIGNATUE );
	ASSERT( IoBuffer );
	ASSERT( BufferSize );
	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// Check if buffer large enough.
	//
	Status = CfixkrsCalculateBufferSize( Adapter, &StructSize, &StringsSize );
	if ( ! NT_SUCCESS( Status ) )
	{
		return Status;
	}

	*BufferSize = StructSize + StringsSize;

	if ( StructSize + StringsSize > MaximumBufferSize )
	{
		return STATUS_BUFFER_OVERFLOW;
	}

	//
	// Fill buffer. In order to get all fields properly aligned,
	// the buffer is filled as follows:
	//   All structures (all DWORD aligned)
	//   ...
	//   All strings	(all WCHAR aligned)
	//   ...
	//
	// Both areas are filled on one pass, thefore, the two
	// offsets BufferStructOffset and BufferStringsOffset are used.
	//
	BufferStructOffset = 0;
	BufferStringsOffset = StructSize;

	//
	// CFIXKRIO_TEST_MODULE.
	// FixtureOffsetsArray will be filled as we go. 
	//
	IoModule = ( PCFIXKRIO_TEST_MODULE ) IoBuffer;
	IoModule->FixtureCount = Adapter->Fixtures.EntryCount;
	FixtureOffsetsArray = ( PULONG ) &IoModule->FixtureOffsets;

	BufferStructOffset += RTL_SIZEOF_THROUGH_FIELD( 
		CFIXKRIO_TEST_MODULE,
		FixtureOffsets[ IoModule->FixtureCount - 1 ] );

	//
	// CFIXKRIO_FIXTUREs.
	//
	for ( FixIndex = 0; FixIndex < Adapter->Fixtures.EntryCount; FixIndex++ )
	{
		USHORT DefIndex;
		PCFIXKRIO_FIXTURE IoFixture;

		CurrentFixture = &Adapter->Fixtures.Entries[ FixIndex ];

		FixtureOffsetsArray[ FixIndex ] = BufferStructOffset;
		IoFixture = ( PCFIXKRIO_FIXTURE ) 
			( IoBuffer + BufferStructOffset );

		IoFixture->Key			= FixIndex;
		IoFixture->EntryCount	= CurrentFixture->Definition.EntryCount;

		BufferStructOffset += RTL_SIZEOF_THROUGH_FIELD( 
			CFIXKRIO_FIXTURE,
			Entries[ IoFixture->EntryCount - 1 ] );

		//
		// Name.
		//
		IoFixture->NameLength	= CurrentFixture->FixtureName.Length;
		IoFixture->NameOffset	= BufferStringsOffset;
		RtlCopyMemory( 
			IoBuffer + IoFixture->NameOffset, 
			CurrentFixture->FixtureName.Buffer,
			IoFixture->NameLength );
		BufferStringsOffset += IoFixture->NameLength;

		//
		// Entries.
		//
		for ( DefIndex = 0; DefIndex < CurrentFixture->Definition.EntryCount; DefIndex++ )
		{
			size_t StringLen;
			Status = RtlStringCbLengthW(
				CurrentFixture->Definition.Entries[ DefIndex ].Name,
				( CFIX_MAX_ENTRY_NAME_CCH - 1 ) * sizeof( WCHAR ),
				&StringLen );
			if ( ! NT_SUCCESS( Status ) )
			{
				return Status;
			}
			ASSERT( StringLen > 0 );

			IoFixture->Entries[ DefIndex ].Type = CurrentFixture->Definition.Entries[ DefIndex ].Type;
			IoFixture->Entries[ DefIndex ].Key = DefIndex;
			IoFixture->Entries[ DefIndex ].NameLength = ( ULONG ) StringLen;
			IoFixture->Entries[ DefIndex ].NameOffset = BufferStringsOffset;

			RtlCopyMemory( 
				IoBuffer + IoFixture->Entries[ DefIndex ].NameOffset, 
				CurrentFixture->Definition.Entries[ DefIndex ].Name,
				StringLen );
			BufferStringsOffset += IoFixture->Entries[ DefIndex ].NameLength;
		}
	}

	ASSERT( BufferStringsOffset == StructSize + StringsSize );

	return STATUS_SUCCESS;
}
