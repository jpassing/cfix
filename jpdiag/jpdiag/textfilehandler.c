/*----------------------------------------------------------------------
 * Purpose:
 *		Handler that writes to a text file.
 *
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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

#define JPDIAGAPI

#include <stdlib.h>
#include "internal.h"

typedef struct _TEXTFILE_HANDLER
{
	JPDIAG_HANDLER Base;

	volatile LONG ReferenceCount;

	PJPDIAG_FORMATTER Formatter;

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
} TEXTFILE_HANDLER, *PTEXTFILE_HANDLER;

/*----------------------------------------------------------------------
 *
 * Methods.
 *
 */
static HRESULT JpdiagsAppendToFile(
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

static HRESULT JpdiagsConvertToUtf8AndAppendToFile(
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
		return JpdiagsAppendToFile( 
			File, 
			Utf8Buffer, 
			( DWORD ) strlen( Utf8Buffer ) );
	}
}

static HRESULT JpdiagsTextFileHandle(
	__in PJPDIAG_HANDLER This,
	__in PJPDIAG_EVENT_PACKET Packet
	)
{
	PTEXTFILE_HANDLER FileHandler = ( PTEXTFILE_HANDLER ) This;
	WCHAR Buffer[ 1024 ];
	HRESULT Hr;
	UINT Index;
	UINT BufferCch;

	if ( ! FileHandler ||
		! JpdiagpIsValidHandler( FileHandler ) )
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
	BufferCch = ( DWORD ) wcslen( Buffer );

	//
	// Strip CR, LF, TAB within text.
	//
	for ( Index = 0; Index < BufferCch; Index++ )
	{
		if ( Buffer[ Index ] == L'\r' ||
			 Buffer[ Index ] == L'\n' ||
			 Buffer[ Index ] == L'\t' )
		{
			Buffer[ Index ] = L' ';
		}
	}

	//
	// Eat trailing spaces and make sure buffer ends with a CRLF.
	//
	while ( BufferCch > 0 )
	{
		if ( Buffer[ BufferCch - 1 ] == L' ' )
		{
			BufferCch--;
		}
		else
		{
			break;
		}
	}

	if ( BufferCch + 3 > _countof( Buffer ) )
	{
		BufferCch -= 3;
	}

	Buffer[ BufferCch++ ] = L'\r';
	Buffer[ BufferCch++ ] = L'\n';
	Buffer[ BufferCch++ ] = L'\0';

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


static VOID JpdiagsTextFileDeleteHandler( 
	__in PTEXTFILE_HANDLER FileHandler 
	)
{
	if ( FileHandler->Formatter )
	{
		FileHandler->Formatter->Dereference( FileHandler->Formatter );
	}

	DeleteCriticalSection( &FileHandler->File.Lock );

	JpdiagpFree( FileHandler );
}

static HRESULT JpdiagsTextFileSetNextHandler(
	__in PJPDIAG_HANDLER This,
	__in PJPDIAG_HANDLER Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return JPDIAG_E_CHAINING_NOT_SUPPORTED;
}

static HRESULT JpdiagsTextFileGetNextHandler(
	__in PJPDIAG_HANDLER This,
	__out_opt PJPDIAG_HANDLER *Handler
	)
{
	UNREFERENCED_PARAMETER( This );
	UNREFERENCED_PARAMETER( Handler );
	return JPDIAG_E_CHAINING_NOT_SUPPORTED;
}

static VOID JpdiagsTextFileReferenceHandler(
	__in PJPDIAG_HANDLER This
	)
{
	PTEXTFILE_HANDLER FileHandler = ( PTEXTFILE_HANDLER ) This;
	_ASSERTE( JpdiagpIsValidHandler( FileHandler ) );

	InterlockedIncrement( &FileHandler->ReferenceCount );
}

static VOID JpdiagsTextFileDereferenceHandler(
	__in PJPDIAG_HANDLER This
	)
{
	PTEXTFILE_HANDLER FileHandler = ( PTEXTFILE_HANDLER ) This;
	_ASSERTE( JpdiagpIsValidHandler( FileHandler ) );

	if ( 0 == InterlockedDecrement( &FileHandler->ReferenceCount ) )
	{
		JpdiagsTextFileDeleteHandler( FileHandler );
	}
}

/*----------------------------------------------------------------------
 *
 * Public.
 *
 */

JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateTextFileHandler(
	__in JPDIAG_SESSION_HANDLE Session,
	__in PCWSTR FilePath,
	__in JPDIAG_TEXTFILE_ENCODING Encoding,
	__out PJPDIAG_HANDLER *Handler
	)
{
	PTEXTFILE_HANDLER FileHandler = NULL;
	HRESULT Hr = E_UNEXPECTED;
	LARGE_INTEGER Offset; 
	HANDLE File;

	if ( ! Session || 
		 ! FilePath || 
		 ( Encoding != JpdiagEncodingUtf16 && Encoding != JpdiagEncodingUtf8 ) ||
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
	FileHandler = JpdiagpMalloc( sizeof( TEXTFILE_HANDLER ), TRUE );
	if ( ! FileHandler )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize.
	//
	FileHandler->ReferenceCount = 1;
	FileHandler->Base.Size = sizeof( JPDIAG_HANDLER );
	
	FileHandler->Base.Reference				= JpdiagsTextFileReferenceHandler;
	FileHandler->Base.Dereference			= JpdiagsTextFileDereferenceHandler;
	FileHandler->Base.GetNextHandler		= JpdiagsTextFileGetNextHandler;
	FileHandler->Base.SetNextHandler		= JpdiagsTextFileSetNextHandler;
	FileHandler->Base.Handle				= JpdiagsTextFileHandle;

	FileHandler->File.Handle				= File;
	InitializeCriticalSection( &FileHandler->File.Lock );

	if ( Encoding == JpdiagEncodingUtf8 )
	{
		FileHandler->AppendRoutine = JpdiagsConvertToUtf8AndAppendToFile;
	}
	else
	{
		FileHandler->AppendRoutine = JpdiagsAppendToFile;
	}

	//
	// Obtain formatter.
	//
	Hr = JpdiagQueryInformationSession(
		Session,
		JpdiagSessionFormatter,
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
			JpdiagsTextFileDeleteHandler( FileHandler );
		}
	}

	return Hr;
}
