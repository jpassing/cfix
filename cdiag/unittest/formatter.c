/*----------------------------------------------------------------------
 * Purpose:
 *		Test case for the Formatter
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

#include "stdafx.h"
#include "iatpatch.h"
#include "eventpkt.h"

typedef struct _FMT_SAMPLE_TYPE
{
	DWORD Type;
	PCWSTR String;
} FMT_SAMPLE_TYPE, *PFMT_SAMPLE_TYPE;
static FMT_SAMPLE_TYPE SampleType[] = 
{
	{ 0xFFFF,				L"(null)" },
	{ CdiagLogEvent,		L"Log" },
	{ CdiagAssertionEvent,	L"Assertion" },
	{ 0, 0 }
};

typedef struct _FMT_SAMPLE_SEVERITY
{
	UCHAR Severity;
	PCWSTR String;
} FMT_SAMPLE_SEVERITY, *PFMT_SAMPLE_SEVERITY;
static FMT_SAMPLE_SEVERITY SampleSeverity[] = 
{
	{ CdiagDebugSeverity,	L"Debug" },
	{ 0xFF,					L"(null)" },
	{ CdiagFatalSeverity,	L"Fatal" },
	{ 0, 0 }
};

typedef struct _FMT_SAMPLE_MODE
{
	UCHAR Mode;
	PCWSTR String;
} FMT_SAMPLE_MODE, *PFMT_SAMPLE_MODE;
static FMT_SAMPLE_MODE SampleMode[] = 
{
	{ CdiagUserMode,	L"User" },
	{ CdiagKernelMode, L"Kernel" },
	{ 0xFF,				L"(null)" },
	{ 0, 0 }
};

typedef struct _FMT_SAMPLE_MACHINE
{
	PCWSTR Machine;
	PCWSTR String;
} FMT_SAMPLE_MACHINE, *PFMT_SAMPLE_MACHINE;
static FMT_SAMPLE_MACHINE SampleMachine[] = 
{
	{ NULL,				L"(null)" },
	{ L"",				L"" },
	{ L"foo",			L"foo" },
	{ 0, 0 }
};

typedef struct _FMT_SAMPLE_ID
{
	DWORD Id;
	PCWSTR String;
} FMT_SAMPLE_ID, *PFMT_SAMPLE_ID;
static FMT_SAMPLE_ID SampleId[] = 
{
	{ 0,			L"0" },
	{ 4200000000,	L"4200000000" },
	{ 42,			L"42" },
	{ 0, 0 }
};

static FILETIME TimeNow;		
static FILETIME TimeZero;
static WCHAR TimeNowString[ 60 ];
static WCHAR TimeZeroString[ 60 ];

typedef struct _FMT_SAMPLE_TIMESTAMP
{
	PFILETIME Ts;
	PCWSTR String;
} FMT_SAMPLE_TIMESTAMP, *PFMT_SAMPLE_TIMESTAMP;
static FMT_SAMPLE_TIMESTAMP SampleTimestamp[] = 
{
	{ NULL,			L"(null)" },
	{ &TimeNow,		TimeNowString },
	{ &TimeZero,	TimeZeroString },
	{ 0, 0 }
};

typedef struct _FMT_SAMPLE_FLAGS
{
	USHORT Flag;
	PCWSTR String;
} FMT_SAMPLE_FLAGS, *PFMT_SAMPLE_FLAGS;
static FMT_SAMPLE_FLAGS SampleFlags[] = 
{
	{ 0,		L"0x0000" },
	{ 0xFFFF,	L"0xFFFF" },
	{ 0x1122,	L"0x1122" },
	{ 0, 0 }
};

typedef struct _FMT_SAMPLE_MESSAGE_AND_CODE
{
	BOOL UseResolver;

	DWORD Code;
	PCWSTR Message;

	PCWSTR CodeString;
	PCWSTR MessageString;
} FMT_SAMPLE_MESSAGE_AND_CODE, *PFMT_SAMPLE_MESSAGE_AND_CODE; 
static FMT_SAMPLE_MESSAGE_AND_CODE SampleMessageAndCode[] =
{
	{ TRUE,	0,						NULL,	L"0x00000000",	L"The operation completed successfully.\r\n" },
	{ TRUE, ERROR_FILE_NOT_FOUND,	NULL,	L"0x00000002",	L"The system cannot find the file specified.\r\n" },
	{ TRUE, ERROR_FILE_NOT_FOUND,	L"test",L"0x00000002",	L"test" },
	{ FALSE, 0,						NULL,	L"0x00000000",	L"(null)" },
	{ FALSE, ERROR_FILE_NOT_FOUND,	NULL,	L"0x00000002",	L"(null)" },
	{ FALSE, ERROR_FILE_NOT_FOUND,	L"test",L"0x00000002",	L"test" },
	{ 0, 0, 0, 0, 0 }
};

typedef struct _FMT_SAMPLE_DEBUG_INFO
{
	BOOL ProvideDebugInfo;

	PCWSTR Module;
	PCWSTR Function;
	PCWSTR SrcFile;
	DWORD Line;

	PCWSTR ModuleString;
	PCWSTR FunctionString;
	PCWSTR SrcFileString;
	PCWSTR LineString;
} FMT_SAMPLE_DEBUG_INFO, *PFMT_SAMPLE_DEBUG_INFO;
static FMT_SAMPLE_DEBUG_INFO SampleDebugInfo[] = 
{
	{ FALSE,	NULL,	NULL,	NULL,	0,			L"(null)",	L"(null)",	L"(null)",	L"0" },
	{ TRUE,		NULL,	NULL,	NULL,	0,			L"(null)",	L"(null)",	L"(null)",	L"0" },
	{ TRUE,		L"",	L"",	L"",	0,			L"",		L"",		L"",		L"0" },
	{ TRUE,		L"Mod",	L"Fun",	L"Src",	4100000000,	L"Mod",		L"Fun",		L"Src",		L"4100000000" },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};



static VOID TestFormatter()
{
	PCDIAG_FORMATTER Fmt;
	PCDIAG_MESSAGE_RESOLVER Resolver;
	SIZE_T BufferLen;
	WCHAR Buffer[ 400 ];
	WCHAR ExpectedString[ 400 ];

	PFMT_SAMPLE_TYPE 			Type;
	PFMT_SAMPLE_SEVERITY 		Severity;
	PFMT_SAMPLE_MODE			Mode;
	PFMT_SAMPLE_MACHINE			Machine;
	PFMT_SAMPLE_ID				ProcId;
	PFMT_SAMPLE_ID				ThrId;
	PFMT_SAMPLE_TIMESTAMP		Timestamp;
	PFMT_SAMPLE_FLAGS			Flags;
	PFMT_SAMPLE_MESSAGE_AND_CODE MessageAndCode;
	PFMT_SAMPLE_DEBUG_INFO		DebugInfo;

	PCWSTR SampleFormat = 
		L"%Type, "
		L"%Flags, "			
		L"%Severity, "		
		L"%ProcessorMode, " 
		L"%Machine, "
		L"%ProcessId, "		
		L"%ThreadId, "		
		L"%Code, "			
		L"%Module, "		
		L"%Function, "		
		L"%File, "			
		L"%Line, "
		L"%Message";

	//
	// Setup timstamps.
	//
	SYSTEMTIME SysTime;
	TimeZero.dwHighDateTime = 0;
	TimeZero.dwLowDateTime = 0;

	FileTimeToSystemTime( &TimeZero, &SysTime );
	GetTimeFormat(
		LOCALE_USER_DEFAULT,
		0,
		&SysTime,
		NULL,
		TimeZeroString,
		_countof( TimeZeroString ) );

	GetSystemTimeAsFileTime( &TimeNow );
	FileTimeToSystemTime( &TimeNow, &SysTime );
	GetTimeFormat(
		LOCALE_USER_DEFAULT,
		0,
		&SysTime,
		NULL,
		TimeNowString,
		_countof( TimeNowString ) );

	TEST_HR( CdiagCreateMessageResolver( &Resolver ) );

	for ( Type = 		SampleType; Type->String != 0; Type++ )
	for ( Severity = 	SampleSeverity; Severity->String != 0; Severity++ )
	for ( Mode = 		SampleMode; Mode->String != 0; Mode++ )
	for ( Machine = 	SampleMachine; Machine->String != 0; Machine++ )
	for ( ProcId = 		SampleId; ProcId->String != 0; ProcId++ )
	for ( ThrId = 		SampleId; ThrId->String != 0; ThrId++ )
	for ( Timestamp =	SampleTimestamp; Timestamp->String != 0; Timestamp++ )
	for ( Flags = 		SampleFlags; Flags->String != 0; Flags++ )
	for ( MessageAndCode = SampleMessageAndCode; MessageAndCode->CodeString != 0; MessageAndCode++ )
	for ( DebugInfo =	SampleDebugInfo; DebugInfo->ModuleString != 0; DebugInfo++ )
	{
		EVENT_PACKET_WITH_DATA EvPktWithData;
		HRESULT Hr;
		
		TEST_HR( CdiagCreateFormatter( 
			SampleFormat, 
			( MessageAndCode->UseResolver ? Resolver : NULL ),
			0,
			&Fmt ) );

		InitializeEventPacket( 
			( CDIAG_EVENT_TYPE ) Type->Type,
			Flags->Flag,
			Severity->Severity,
			Mode->Mode,
			Machine->Machine,
			ProcId->Id,
			ThrId->Id,
			Timestamp->Ts,
			MessageAndCode->Code,
			MessageAndCode->Message,
			DebugInfo->ProvideDebugInfo,
			DebugInfo->Module,
			DebugInfo->Function,
			DebugInfo->SrcFile,
			DebugInfo->Line,
			&EvPktWithData );

		//
		// Construct expected string.
		//
		StringCchPrintf(
			ExpectedString,
			_countof( ExpectedString ),
			L"%s, "
			L"%s, "			
			L"%s, "		
			L"%s, " 
			L"%s, "
			L"%s, "		
			L"%s, "		
			L"%s, "			
			L"%s, "		
			L"%s, "		
			L"%s, "			
			L"%s, "
			L"%s",
			Type->String, 
			Flags->String, 			
			Severity->String, 		
			Mode->String,  
			Machine->String, 
			ProcId->String, 		
			ThrId->String, 		
			MessageAndCode->CodeString, 			
			DebugInfo->ModuleString, 		
			DebugInfo->FunctionString, 		
			DebugInfo->SrcFileString, 			
			DebugInfo->LineString, 
			MessageAndCode->MessageString );

		BufferLen = _countof( Buffer ) - 1;

		// 
		// Mark end of buffer
		//
		memset( Buffer, 0, sizeof( Buffer ) );
		BufferLen = wcslen( ExpectedString ) + 1 ;
		Buffer[ BufferLen ] = 0xBAD1;

		Hr = Fmt->Format(
			Fmt,
			&EvPktWithData.EventPacket,
			BufferLen,
			Buffer );
		TEST( CDIAG_E_BUFFER_TOO_SMALL == Hr ||
			  ( S_OK == Hr &&
				0 == wcscmp( Buffer, ExpectedString ) ) );

		TEST( Buffer[ BufferLen ] == 0xBAD1 );


		Fmt->Dereference( Fmt );
	}
	Resolver->Dereference( Resolver );

}

CFIX_BEGIN_FIXTURE( Formatter )
	CFIX_FIXTURE_ENTRY( TestFormatter )
CFIX_END_FIXTURE()

