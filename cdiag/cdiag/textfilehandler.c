/*----------------------------------------------------------------------
 * Purpose:
 *		Handler that writes to a text file.
 *
 * Copyright:
 *		2007-2009 Johannes Passing (passing at users.sourceforge.net)
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

#define CDIAGAPI

#include <stdlib.h>
#include "cdiagp.h"

typedef struct _CDIAGP_TEXTFILE_HANDLER
{
	CDIAG_HANDLER Base;

	volatile LONG ReferenceCount;

	PCDIAG_FORMATTER Formatter;

	HRESULT ( *AppendRoutine ) (
		__in HANDLE File,
		__in PVOID Buffer,
		__in DWORD BufferSizeInBytes
		);

	struct
	{
		CRITICAL_SECTION Lock;

		HANDLE Handle;
	} File;
} CDIAGP_TEXTFILE_HANDLER, *PCDIAGP_TEXTFILE_HANDLER;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static HRESULT CdiagsAppendToFile(
	__in HANDLE File,
	__in PVOID Buffer,
	__in DWORD BufferSizeInBytes
	)
{
	DWORD Written;
	if ( WriteFile(
		File,
		Buffer,
		BufferSizeInBytes,
		&Written,
		NULL ) )
	{
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
}

static HRESULT CdiagsConvertToUtf8AndAppendToFile(
	__in HANDLE File,
	__in PVOID Utf16Buffer,
	__in DWORD Utf16BufferSizeInBytes
	)
{
	CHAR Utf8Buffer[ 2048 ];

	UNREFERENCED_PARAMETER( Utf16BufferSizeInBytes );
	_ASSERT( ( Utf16BufferSizeInBytes % 2 ) == 0 );

	if ( 0 == WideCharToMultiByte(
		CP_UTF8,
		0,
		Utf16Buffer,
		-1,
		Utf8Buffer,
		sizeof( Utf8Buffer ),
		NULL,
		NULL ) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
	else
	{
		return CdiagsAppendToFile( 
			File, 
			Utf8Buffer, 
			( DWORD ) strlen( Utf8Buffer ) );
	}
}

static HRESULT CdiagsTextFileHandle(
	__in PCDIAG_HANDLER This,
	__in PCDIAG_EVENT_PACKET Packet
	)
{
	PCDIAGP_TEXTFILE_HANDLER FileHandler = ( PCDIAGP_TEXTFILE_HANDLER ) This;
	WCHAR Buffer[ 2048 ];
	HRESULT Hr;
	//UINT Index;
	UINT BufferCch;

	if ( ! FileHandler ||
		! CdiagpIsValidHandler( FileHandler ) )
	{
		return E_INVALIDARG;
	}
	
	Hr = FileHandler->Formatter->Format(
		FileHandler->Formatter,
		Packet,
		_countof( Buffer ) - 2,		// Account for CRLF.
		Buffer );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	//
	// Effective size.
	//
#pragma warning( push )
#pragma warning( disable: 6054 )
	BufferCch = ( DWORD ) wcslen( Buffer );
#pragma warning( pop )

	////
	//// Strip CR, LF, TAB within text.
	////
	//for ( Index = 0; Index < BufferCch; Index++ )
	//{
	//	if ( Buffer[ Index ] == L'\r' ||
	//		 Buffer[ Index ] == L'\n' ||
	//		 Buffer[ Index ] == L'\t' )
	//	{
	//		Buffer[ Index ] = L' ';
	//	}
	//}

	////
	//// Eat trailing spaces and make sure buffer ends with a CRLF.
	////
	//while ( BufferCch > 0 )
	//{
	//	if ( Buffer[ BufferCch - 1 ] == L' ' )
	//	{
	//		BufferCch--;
	//	}
	//	else
	//	{
	//		break;
	//	}
	//}

	//if ( BufferCch + 3 > _countof( Buffer ) )
	//{
	//	BufferCch -= 3;
	//}

	//Buffer[ BufferCch++ ] = L'\r';
	//Buffer[ BufferCch++ ] = L'\n';
	//Buffer[ BufferCch++ ] = L'\0';

	//
	// Protect agains concurrent appends.
	//
	EnterCriticalSection( &FileHandler->File.Lock );

	//
	// Output.
	//
	Hr = ( FileHandler->AppendRoutine )( 
		FileHandler->File.Handle, 
		Buffer, 
		2 * BufferCch );

	LeaveCriticalSection( &FileHandler->File.Lock );

	return Hr;
}


static VOID CdiagsTextFileDeleteHandler( 
	__in PCDIAGP_TEXTFILE_HANDLER FileHandler 
	)
{
	if ( FileHandler->Formatter )
	{
		FileHandler->Formatter->Dereference( FileHandler->Formatter );
	}

	DeleteCriticalSection( &FileHandler->File.Lock );

	CdiagpFree( FileHandler );
}

static HRESULT CdiagsTextFileSetNextHandler(
	__in PCDIAG_HANDLER This,
	__in PCDIAG_HANDLER Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return CDIAG_E_CHAINING_NOT_SUPPORTED;
}

static HRESULT CdiagsTextFileGetNextHandler(
	__in PCDIAG_HANDLER This,
	__out_opt PCDIAG_HANDLER *Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return CDIAG_E_CHAINING_NOT_SUPPORTED;
}

static VOID CdiagsTextFileReferenceHandler(
	__in PCDIAG_HANDLER This
	)
{
	PCDIAGP_TEXTFILE_HANDLER FileHandler = ( PCDIAGP_TEXTFILE_HANDLER ) This;
	_ASSERTE( CdiagpIsValidHandler( FileHandler ) );

	InterlockedIncrement( &FileHandler->ReferenceCount );
}

static VOID CdiagsTextFileDereferenceHandler(
	__in PCDIAG_HANDLER This
	)
{
	PCDIAGP_TEXTFILE_HANDLER FileHandler = ( PCDIAGP_TEXTFILE_HANDLER ) This;
	_ASSERTE( CdiagpIsValidHandler( FileHandler ) );

	if ( 0 == InterlockedDecrement( &FileHandler->ReferenceCount ) )
	{
		CdiagsTextFileDeleteHandler( FileHandler );
	}
}

/*----------------------------------------------------------------------
 *
 * Public.
 *
 */

CDIAGAPI HRESULT CDIAGCALLTYPE CdiagCreateTextFileHandler(
	__in CDIAG_SESSION_HANDLE Session,
	__in PCWSTR FilePath,
	__in CDIAG_TEXTFILE_ENCODING Encoding,
	__out PCDIAG_HANDLER *Handler
	)
{
	PCDIAGP_TEXTFILE_HANDLER FileHandler = NULL;
	HRESULT Hr = E_UNEXPECTED;
	LARGE_INTEGER Offset; 
	HANDLE File;

	if ( ! Session || 
		 ! FilePath || 
		 ( Encoding != CdiagEncodingUtf16 && Encoding != CdiagEncodingUtf8 ) ||
		 ! Handler )
	{
		return E_INVALIDARG;
	}

	*Handler = NULL;

	File = CreateFile(
		FilePath,
		FILE_APPEND_DATA,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		0,
		NULL );
	if ( File == INVALID_HANDLE_VALUE )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	//
	// Position at EOF.
	//
	Offset.QuadPart = 0;
	if ( ! SetFilePointerEx( File, Offset, NULL, FILE_END ) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	//
	// Allocate.
	//
	FileHandler = CdiagpMalloc( sizeof( CDIAGP_TEXTFILE_HANDLER ), TRUE );
	if ( ! FileHandler )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize.
	//
	FileHandler->ReferenceCount = 1;
	FileHandler->Base.Size = sizeof( CDIAG_HANDLER );
	
	FileHandler->Base.Reference				= CdiagsTextFileReferenceHandler;
	FileHandler->Base.Dereference			= CdiagsTextFileDereferenceHandler;
	FileHandler->Base.GetNextHandler		= CdiagsTextFileGetNextHandler;
	FileHandler->Base.SetNextHandler		= CdiagsTextFileSetNextHandler;
	FileHandler->Base.Handle				= CdiagsTextFileHandle;

	FileHandler->File.Handle				= File;
	InitializeCriticalSection( &FileHandler->File.Lock );

	if ( Encoding == CdiagEncodingUtf8 )
	{
		FileHandler->AppendRoutine = CdiagsConvertToUtf8AndAppendToFile;
	}
	else
	{
		FileHandler->AppendRoutine = CdiagsAppendToFile;
	}

	//
	// Obtain formatter.
	//
	Hr = CdiagQueryInformationSession(
		Session,
		CdiagSessionFormatter,
		0,
		&FileHandler->Formatter );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	*Handler = &FileHandler->Base;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( FileHandler ) 
		{
			CdiagsTextFileDeleteHandler( FileHandler );
		}
	}

	return Hr;
}
