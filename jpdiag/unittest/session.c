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

static UINT DefHdlOutputCalls = 0;
static UINT SpecificHdlOutputCalls = 0;

static VOID CALLBACK DefHdlOutput( PCWSTR Text )
{
	UNREFERENCED_PARAMETER( Text );
	DefHdlOutputCalls++;
}

static VOID CALLBACK SpecificHdlOutput( PCWSTR Text )
{
	UNREFERENCED_PARAMETER( Text );
	SpecificHdlOutputCalls++;
}

static VOID TestCreateAndCloseSession()
{
	JPDIAG_SESSION_HANDLE Ssn;
	PJPDIAG_FORMATTER Fmt, ActualFmt;
	PJPDIAG_MESSAGE_RESOLVER Resv, ActualResv;

	TEST_HR( JpdiagCreateMessageResolver( &Resv ) );
	TEST_HR( JpdiagCreateFormatter( L"%Code", Resv, 0, &Fmt ) );


	//
	// No fmt/resv.
	//
	TEST_HR( JpdiagCreateSession( NULL, NULL, &Ssn ) );
	TEST( Ssn );

	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt != NULL );
	ActualFmt->Dereference( ActualFmt );

	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv != NULL );
	ActualResv->Dereference( ActualResv );

	//
	// Set fmt/resv.
	//
	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionFormatter,
		0,
		Fmt ) );
	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionResolver,
		0,
		Resv ) );

	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt == Fmt );
	ActualFmt->Dereference( ActualFmt );

	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv == Resv );
	ActualResv->Dereference( ActualResv );

	TEST_HR( JpdiagDereferenceSession( Ssn ) );




	//
	// Resv only.
	//
	TEST_HR( JpdiagCreateSession( NULL, Resv, &Ssn ) );
	TEST( Ssn );

	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt != NULL );
	ActualFmt->Dereference( ActualFmt );
	
	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv == Resv );
	ActualResv->Dereference( ActualResv );
	
	TEST_HR( JpdiagDereferenceSession( Ssn ) );




	//
	// Resv and fmt.
	//
	TEST_HR( JpdiagCreateSession( Fmt, Resv, &Ssn ) );
	TEST( Ssn );

	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt == Fmt );
	ActualFmt->Dereference( ActualFmt );
	
	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv == Resv );
	ActualResv->Dereference( ActualResv );
	
	TEST_HR( JpdiagReferenceSession( Ssn ) );
	TEST_HR( JpdiagDereferenceSession( Ssn ) );
	TEST_HR( JpdiagDereferenceSession( Ssn ) );

	Fmt->Dereference( Fmt );
	Resv->Dereference( Resv );
}

static VOID TestHandling()
{
	EVENT_PACKET_WITH_DATA Pkt;
	FILETIME Ft = { 1, 2 };
	PJPDIAG_HANDLER DefHdl;
	PJPDIAG_HANDLER Hdl;
	JPDIAG_SESSION_HANDLE Ssn;
	DWORD Filter;

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

	TEST_HR( JpdiagCreateSession( NULL, NULL, &Ssn ) );
	TEST( Ssn );

	TEST_HR( JpdiagQueryInformationSession( 
		Ssn,
		JpdiagSessionDefaultHandler,
		0,
		( PVOID* ) &DefHdl ) );
	TEST( DefHdl == NULL );

	//
	// Handle w/o handlers.
	//
	TEST( S_FALSE == JpdiagHandleEvent( Ssn, &Pkt.EventPacket ) );

	//
	// Install def. hdl. (twice).
	//
	TEST_HR( JpdiagCreateOutputHandler(
		Ssn,
		DefHdlOutput,
		&DefHdl ) );

	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionDefaultHandler,
		0,
		DefHdl ) );
	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionDefaultHandler,
		0,
		DefHdl ) );
	DefHdl->Dereference( DefHdl );

	TEST_HR( JpdiagQueryInformationSession( 
		Ssn,
		JpdiagSessionDefaultHandler,
		0,
		( PVOID* ) &DefHdl ) );
	TEST( DefHdl != NULL );
	DefHdl->Dereference( DefHdl );

	//
	// Handle via def. hdl.
	//
	TEST( S_OK == JpdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( DefHdlOutputCalls == 1 );
	DefHdlOutputCalls = 0;



	//
	// Install specific handler #1 (twice).
	//
	TEST_HR( JpdiagCreateOutputHandler(
		Ssn,
		SpecificHdlOutput,
		&Hdl ) );

	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionHandler,
		JpdiagLogEvent,
		Hdl ) );
	
	// again...
	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionHandler,
		JpdiagLogEvent,
		Hdl ) );
	Hdl->Dereference( Hdl );
	Hdl = NULL;

	TEST_HR( JpdiagQueryInformationSession( 
		Ssn,
		JpdiagSessionHandler,
		JpdiagLogEvent,
		( PVOID* ) &Hdl ) );
	TEST( Hdl != NULL );
	Hdl->Dereference( Hdl );
	Hdl = NULL;

	//
	// Query nonex. hdl.
	//
	TEST( S_FALSE == JpdiagQueryInformationSession( 
		Ssn,
		JpdiagSessionHandler,
		JpdiagTraceEvent,
		( PVOID* ) &Hdl ) );
	TEST( Hdl == NULL );

	//
	// Handle one via defhdl, one via specific hdl.
	//
	Pkt.EventPacket.Type = JpdiagTraceEvent;
	TEST( S_OK == JpdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( DefHdlOutputCalls == 1 );
	DefHdlOutputCalls = 0;

	Pkt.EventPacket.Type = JpdiagLogEvent;
	TEST( S_OK == JpdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( SpecificHdlOutputCalls == 1 );
	SpecificHdlOutputCalls = 0;

	//
	// Get Filter.
	//
	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionSeverityFilter,
		JpdiagLogEvent,
		( PVOID* ) &Filter ) );
	TEST( Filter == 0xffffffff );

	Filter = 2;
	TEST_HR( JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionSeverityFilter,
		JpdiagLogEvent,
		( PVOID* ) &Filter ) );
	TEST_HR( JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionSeverityFilter,
		JpdiagLogEvent,
		( PVOID* ) &Filter ) );
	TEST( Filter == 2 );

	//
	// Get/Set filter of nonex. hdl.
	//
	Filter = 1234;
	TEST( S_FALSE == JpdiagSetInformationSession(
		Ssn,
		JpdiagSessionSeverityFilter,
		JpdiagTraceEvent,
		( PVOID* ) &Filter ) );
	TEST( S_FALSE == JpdiagQueryInformationSession(
		Ssn,
		JpdiagSessionSeverityFilter,
		JpdiagTraceEvent,
		( PVOID* ) &Filter ) );
	TEST( Filter == 0 );

	//
	// Output should be filtered now.
	//
	TEST( S_FALSE == JpdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( SpecificHdlOutputCalls == 0 );

	TEST_HR( JpdiagDereferenceSession( Ssn ) );
}

VOID TestSession()
{
	TestCreateAndCloseSession();
	TestHandling();
}