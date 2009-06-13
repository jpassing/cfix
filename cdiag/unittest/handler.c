/*----------------------------------------------------------------------
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
#include "eventpkt.h"

static VOID TestNonChainableHandler(
	__in PCDIAG_HANDLER Hdl
	)
{
	PCDIAG_HANDLER OtherHdl;
	EVENT_PACKET_WITH_DATA Pkt;
	FILETIME Ft = { 1, 2 };
	UINT Index;

	for ( Index = 0; Index < 5; Index++ )
	{
		OtherHdl = NULL;
		TEST( CDIAG_E_CHAINING_NOT_SUPPORTED == Hdl->GetNextHandler( Hdl, &OtherHdl ) );
		TEST( OtherHdl == NULL );

		OtherHdl = Hdl;
		TEST( CDIAG_E_CHAINING_NOT_SUPPORTED == Hdl->SetNextHandler( Hdl, Hdl ) );
		
		Hdl->Reference( Hdl );
		Hdl->Dereference( Hdl );

		InitializeEventPacket(
			CdiagLogEvent,
			0,
			CdiagFatalSeverity,
			CdiagUserMode,
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
	CDIAG_SESSION_HANDLE Session;
	PCDIAG_HANDLER Handler;

	TEST_HR( CdiagCreateSession( NULL, NULL, &Session ) );

	//
	// DBG handler.
	//
	TEST_HR( CdiagCreateOutputHandler( Session, OutputDebugString, &Handler ) );
	TestNonChainableHandler( Handler );
	Handler->Dereference( Handler );

	//
	// UTF16 textfile handler.
	//
	TEST_HR( CdiagCreateTextFileHandler( 
		Session, 
		L"__utf16.txt",
		CdiagEncodingUtf16, 
		&Handler ) );
	TestNonChainableHandler( Handler );
	Handler->Dereference( Handler );

	TEST( GetFileAttributes( L"__utf16.txt" ) != INVALID_FILE_ATTRIBUTES );

	//
	// UTF8 textfile handler.
	//
	TEST_HR( CdiagCreateTextFileHandler( 
		Session, 
		L"__utf8.txt",
		CdiagEncodingUtf8, 
		&Handler ) );
	TestNonChainableHandler( Handler );
	Handler->Dereference( Handler );

	TEST( GetFileAttributes( L"__utf8.txt" ) != INVALID_FILE_ATTRIBUTES );

	TEST_HR( CdiagDereferenceSession( Session ) );
}

CFIX_BEGIN_FIXTURE( Handlers )
	CFIX_FIXTURE_ENTRY( TestHandlers )
CFIX_END_FIXTURE()

