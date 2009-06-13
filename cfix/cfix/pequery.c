/*----------------------------------------------------------------------
 * Purpose:
 *		PE scanning.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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

#include "cfixp.h"

static PVOID CfixPtrFromVaNonRelocated( 
	__in PIMAGE_DOS_HEADER DosHeader,
	__in PIMAGE_NT_HEADERS NtHeader,
	__in ULONG Va
	)
{
	PIMAGE_SECTION_HEADER SectionHeader = IMAGE_FIRST_SECTION( NtHeader );
	ULONG SectionCount = NtHeader->FileHeader.NumberOfSections;

	ULONG Index;

	//
	// Lookup corresponding section.
	//
	for( Index = 0; Index < SectionCount; Index++ )
	{
		if( SectionHeader->VirtualAddress <= Va &&
			Va < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize )
		{
			ULONG RvaWithinSection = Va - SectionHeader->VirtualAddress;
			return CfixPtrFromRva( 
				DosHeader, 
				SectionHeader->PointerToRawData + RvaWithinSection );
		}

		SectionHeader++;
	}

	return NULL;
}

static PIMAGE_DATA_DIRECTORY CfixsQueryOptionalHeader(
	__in PIMAGE_NT_HEADERS NtHeader,
	__out PWORD Subsystem,
	__out PBOOL Win64
	)
{
	if ( NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
	{
		//
		// PE32.
		//
		PIMAGE_OPTIONAL_HEADER32 OptHeader = ( PIMAGE_OPTIONAL_HEADER32 )
			&NtHeader->OptionalHeader;
		
		*Subsystem = OptHeader->Subsystem;
		*Win64 = FALSE;
		
		return &OptHeader->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
	}
	else if ( NtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
	{
		//
		// PE32+.
		//
		PIMAGE_OPTIONAL_HEADER64 OptHeader = ( PIMAGE_OPTIONAL_HEADER64 )
			&NtHeader->OptionalHeader;
		
		*Subsystem = OptHeader->Subsystem;
		*Win64 = TRUE;
		
		return &OptHeader->DataDirectory[ IMAGE_DIRECTORY_ENTRY_EXPORT ];
	}
	else
	{
		return NULL;
	}
}

static HRESULT CfixsQueryPeImage(
	__in PIMAGE_DOS_HEADER DosHeader,
	__in PIMAGE_NT_HEADERS NtHeader,
	__out PWORD Subsystem,
	__out PBOOL CfixExportsFound,
	__out CFIX_MODULE_TYPE *ModuleType
	)
{
	PIMAGE_DATA_DIRECTORY ExportDataDir;
	PIMAGE_EXPORT_DIRECTORY ExportDir;
	PDWORD FunctionRvaArray;
	PDWORD NamesRvaArray;
	
	BOOL Win64;

	*CfixExportsFound = FALSE;

	ExportDataDir = CfixsQueryOptionalHeader( NtHeader, Subsystem, &Win64 );
	if ( ExportDataDir == NULL )
	{
		return HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
	}

	if ( NtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL )
	{
		*ModuleType = CfixModuleDll;
	}
	else if ( *Subsystem == IMAGE_SUBSYSTEM_NATIVE )
	{
		*ModuleType = CfixModuleDriver;
	}
	else
	{
		*ModuleType = CfixModuleExe;
	}

	ExportDir = ( PIMAGE_EXPORT_DIRECTORY ) CfixPtrFromVaNonRelocated( 
		DosHeader, 
		NtHeader,
		ExportDataDir->VirtualAddress );

	if ( ExportDir != NULL )
	{
		//
		// There are exports -- see if it might be a cfix module.
		//
		NamesRvaArray = ( PDWORD ) CfixPtrFromVaNonRelocated(
			DosHeader,
			NtHeader,
			ExportDir->AddressOfNames );

		FunctionRvaArray = ( PDWORD ) CfixPtrFromVaNonRelocated(
			DosHeader,
			NtHeader,
			ExportDir->AddressOfFunctions );

		if ( NamesRvaArray == NULL || FunctionRvaArray == NULL )
		{
			return HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
		}

		if ( ExportDir->NumberOfFunctions > 0 )
		{
			ULONG Index;

			for ( Index = 0; Index < ExportDir->NumberOfNames; Index++ )
			{
				//
				// Get name of export.
				//
				PCSTR Name = ( PCSTR ) CfixPtrFromVaNonRelocated(
					DosHeader,
					NtHeader,
					NamesRvaArray[ Index ] );

				if ( Name != NULL )
				{
					if ( Win64 && CfixpIsFixtureExport64( Name ) ||
						 ! Win64 && CfixpIsFixtureExport32( Name ) )
					{
						*CfixExportsFound = TRUE;
						return S_OK;
					}
				}
			}
		}
		
		ASSERT( ! *CfixExportsFound );
	}

	return S_OK;
}

HRESULT CfixQueryPeImage(
	__in PCWSTR Path,
	__out PCFIX_MODULE_INFO Info
	)
{
	HANDLE File = NULL;
	HANDLE FileMapping = NULL;
	HRESULT Hr;

	PIMAGE_DOS_HEADER DosHeader = NULL;
	PIMAGE_NT_HEADERS NtHeader; 
	
	if ( ! Path || 
		 ! Info || 
		 Info->SizeOfStruct != sizeof ( CFIX_MODULE_INFO ) )
	{
		return E_INVALIDARG;
	}

	//
	// N.B. We use a file mapping rather than LoadModule to avoid
	// running initialization code and to be able to analyze modules
	// of different architectures.
	//

	File = CreateFile( 
		Path,
		GENERIC_READ,
		FILE_SHARE_READ, 
		NULL,
		OPEN_EXISTING,
		0,
		NULL );
	if ( File == INVALID_HANDLE_VALUE )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	FileMapping = CreateFileMapping(
		File,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL );
	if ( FileMapping == NULL )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	DosHeader = MapViewOfFile(
		FileMapping,
		FILE_MAP_READ,
		0,
		0,
		0 );
	if ( DosHeader == NULL )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	if( DosHeader->e_magic != IMAGE_DOS_SIGNATURE )
	{
		Hr = HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
		goto Cleanup;
	}

	NtHeader = ( PIMAGE_NT_HEADERS ) 
		CfixPtrFromRva( DosHeader, DosHeader->e_lfanew );
	if( NtHeader->Signature != IMAGE_NT_SIGNATURE )
	{
		Hr = HRESULT_FROM_WIN32( ERROR_BAD_EXE_FORMAT );
		goto Cleanup;
	}

	Info->MachineType = NtHeader->FileHeader.Machine;
	Hr = CfixsQueryPeImage( 
		DosHeader, 
		NtHeader, 
		&Info->Subsystem,
		&Info->FixtureExportsPresent,
		&Info->ModuleType );

Cleanup:
	if ( DosHeader != NULL )
	{
		VERIFY( UnmapViewOfFile( DosHeader ) );
	}

	if ( FileMapping != NULL )
	{
		VERIFY( CloseHandle( FileMapping ) );
	}

	if ( File != INVALID_HANDLE_VALUE )
	{
		VERIFY( CloseHandle( File ) );
	}

	return Hr;
}