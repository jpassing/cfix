/*----------------------------------------------------------------------
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

#include "stdafx.h"
#include "eventpkt.h"

static VOID TestNonChainableHandler(
	__in PJPDIAG_HANDLER Hdl
	)
{
	PJPDIAG_HANDLER OtherHdl;
	EVENT_PACKET_WITH_DATA Pkt;
	FILETIME Ft = { 1, 2 };
	UINT Index;

	for ( Index = 0; Index < 5; Index++ )
	{
		OtherHdl = NULL;
		TEST( JPDIAG_E_CHAINING_NOT_SUPPORTED == Hdl->GetNextHandler( Hdl, &OtherHdl ) );
		TEST( OtherHdl == NULL );

		OtherHdl = Hdl;
		TEST( JPDIAG_E_CHAINING_NOT_SUPPORTED == Hdl->SetNextHandler( Hdl, Hdl ) );
		
		Hdl->Reference( Hdl );
		Hdl->Dereference( Hdl );

		InitializeEventPacket(
			JpdiagLogEvent,
			0,
			JpdiagFatalSeverity,
			JpdiagUserMode,
			L"machine",
			GetCurrentProcessId(),
			GetCurrentThreadId(),
			&Ft,
			ERROR_BAD_EXE_FORMAT,
			NULL,
			TRUE,
			L"Module",
			L"Function",
			L"SourceFile",
			42,
			&Pkt );

		TEST_HR( Hdl->Handle( Hdl, &Pkt.EventPacket ) );
	}
}

VOID TestHandlers()
{
	JPDIAG_SESSION_HANDLE Session;
	PJPDIAG_HANDLER Handler;

	TEST_HR( JpdiagCreateSession( NULL, NULL, &Session ) );

	//
	// DBG handler.
	//
	TEST_HR( JpdiagCreateOutputHandler( Session, OutputDebugString, &Handler ) );
	TestNonChainableHandler( Handler );
	Handler->Dereference( Handler );

	//
	// UTF16 textfile handler.
	//
	TEST_HR( JpdiagCreateTextFileHandler( 
		Session, 
		L"__utf16.txt",
		JpdiagEncodingUtf16, 
		&Handler ) );
	TestNonChainableHandler( Handler );
	Handler->Dereference( Handler );

	TEST( GetFileAttributes( L"__utf16.txt" ) != INVALID_FILE_ATTRIBUTES );

	//
	// UTF8 textfile handler.
	//
	TEST_HR( JpdiagCreateTextFileHandler( 
		Session, 
		L"__utf8.txt",
		JpdiagEncodingUtf8, 
		&Handler ) );
	TestNonChainableHandler( Handler );
	Handler->Dereference( Handler );

	TEST( GetFileAttributes( L"__utf8.txt" ) != INVALID_FILE_ATTRIBUTES );

	TEST_HR( JpdiagDereferenceSession( Session ) );
}