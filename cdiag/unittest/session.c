/*----------------------------------------------------------------------
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
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
	CDIAG_SESSION_HANDLE Ssn;
	PCDIAG_FORMATTER Fmt, ActualFmt;
	PCDIAG_MESSAGE_RESOLVER Resv, ActualResv;

	TEST_HR( CdiagCreateMessageResolver( &Resv ) );
	TEST_HR( CdiagCreateFormatter( L"%Code", Resv, 0, &Fmt ) );


	//
	// No fmt/resv.
	//
	TEST_HR( CdiagCreateSession( NULL, NULL, &Ssn ) );
	TEST( Ssn );

	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt != NULL );
	if ( ActualFmt == NULL ) return;

	ActualFmt->Dereference( ActualFmt );

	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv != NULL );
	if ( ActualResv == NULL ) return;

	ActualResv->Dereference( ActualResv );

	//
	// Set fmt/resv.
	//
	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionFormatter,
		0,
		Fmt ) );
	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionResolver,
		0,
		Resv ) );

	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt == Fmt );
	ActualFmt->Dereference( ActualFmt );

	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv == Resv );
	ActualResv->Dereference( ActualResv );

	TEST_HR( CdiagDereferenceSession( Ssn ) );




	//
	// Resv only.
	//
	TEST_HR( CdiagCreateSession( NULL, Resv, &Ssn ) );
	TEST( Ssn );

	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt != NULL );
	if ( ActualFmt == NULL ) return ;
	ActualFmt->Dereference( ActualFmt );
	
	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv == Resv );
	ActualResv->Dereference( ActualResv );
	
	TEST_HR( CdiagDereferenceSession( Ssn ) );




	//
	// Resv and fmt.
	//
	TEST_HR( CdiagCreateSession( Fmt, Resv, &Ssn ) );
	TEST( Ssn );

	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionFormatter,
		0,
		( PVOID* ) &ActualFmt ) );
	TEST( ActualFmt == Fmt );
	ActualFmt->Dereference( ActualFmt );
	
	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionResolver,
		0,
		( PVOID* ) &ActualResv ) );
	TEST( ActualResv == Resv );
	ActualResv->Dereference( ActualResv );
	
	TEST_HR( CdiagReferenceSession( Ssn ) );
	TEST_HR( CdiagDereferenceSession( Ssn ) );
	TEST_HR( CdiagDereferenceSession( Ssn ) );

	Fmt->Dereference( Fmt );
	Resv->Dereference( Resv );
}

static VOID TestHandling()
{
	EVENT_PACKET_WITH_DATA Pkt;
	FILETIME Ft = { 1, 2 };
	PCDIAG_HANDLER DefHdl;
	PCDIAG_HANDLER Hdl;
	CDIAG_SESSION_HANDLE Ssn;
	DWORD Filter;

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

	TEST_HR( CdiagCreateSession( NULL, NULL, &Ssn ) );
	TEST( Ssn );

	TEST_HR( CdiagQueryInformationSession( 
		Ssn,
		CdiagSessionDefaultHandler,
		0,
		( PVOID* ) &DefHdl ) );
	TEST( DefHdl == NULL );

	//
	// Handle w/o handlers.
	//
	TEST( S_FALSE == CdiagHandleEvent( Ssn, &Pkt.EventPacket ) );

	//
	// Install def. hdl. (twice).
	//
	TEST_HR( CdiagCreateOutputHandler(
		Ssn,
		DefHdlOutput,
		&DefHdl ) );

	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionDefaultHandler,
		0,
		DefHdl ) );
	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionDefaultHandler,
		0,
		DefHdl ) );
	DefHdl->Dereference( DefHdl );

	TEST_HR( CdiagQueryInformationSession( 
		Ssn,
		CdiagSessionDefaultHandler,
		0,
		( PVOID* ) &DefHdl ) );
	TEST( DefHdl != NULL );
	if ( DefHdl == NULL ) return;

	DefHdl->Dereference( DefHdl );

	//
	// Handle via def. hdl.
	//
	TEST( S_OK == CdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( DefHdlOutputCalls == 1 );
	DefHdlOutputCalls = 0;



	//
	// Install specific handler #1 (twice).
	//
	TEST_HR( CdiagCreateOutputHandler(
		Ssn,
		SpecificHdlOutput,
		&Hdl ) );

	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionHandler,
		CdiagLogEvent,
		Hdl ) );
	
	// again...
	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionHandler,
		CdiagLogEvent,
		Hdl ) );
	Hdl->Dereference( Hdl );
	Hdl = NULL;

	TEST_HR( CdiagQueryInformationSession( 
		Ssn,
		CdiagSessionHandler,
		CdiagLogEvent,
		( PVOID* ) &Hdl ) );
	TEST( Hdl != NULL );
	if ( Hdl == NULL ) return;

	Hdl->Dereference( Hdl );
	Hdl = NULL;

	//
	// Query nonex. hdl.
	//
	TEST( S_FALSE == CdiagQueryInformationSession( 
		Ssn,
		CdiagSessionHandler,
		CdiagTraceEvent,
		( PVOID* ) &Hdl ) );
	TEST( Hdl == NULL );

	//
	// Handle one via defhdl, one via specific hdl.
	//
	Pkt.EventPacket.Type = CdiagTraceEvent;
	TEST( S_OK == CdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( DefHdlOutputCalls == 1 );
	DefHdlOutputCalls = 0;

	Pkt.EventPacket.Type = CdiagLogEvent;
	TEST( S_OK == CdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( SpecificHdlOutputCalls == 1 );
	SpecificHdlOutputCalls = 0;

	//
	// Get Filter.
	//
	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionSeverityFilter,
		CdiagLogEvent,
		( PVOID* ) &Filter ) );
	TEST( Filter == 0xffffffff );

	Filter = 2;
	TEST_HR( CdiagSetInformationSession(
		Ssn,
		CdiagSessionSeverityFilter,
		CdiagLogEvent,
		( PVOID* ) &Filter ) );
	TEST_HR( CdiagQueryInformationSession(
		Ssn,
		CdiagSessionSeverityFilter,
		CdiagLogEvent,
		( PVOID* ) &Filter ) );
	TEST( Filter == 2 );

	//
	// Get/Set filter of nonex. hdl.
	//
	Filter = 1234;
	TEST( S_FALSE == CdiagSetInformationSession(
		Ssn,
		CdiagSessionSeverityFilter,
		CdiagTraceEvent,
		( PVOID* ) &Filter ) );
	TEST( S_FALSE == CdiagQueryInformationSession(
		Ssn,
		CdiagSessionSeverityFilter,
		CdiagTraceEvent,
		( PVOID* ) &Filter ) );
	TEST( Filter == 0 );

	//
	// Output should be filtered now.
	//
	TEST( S_FALSE == CdiagHandleEvent( Ssn, &Pkt.EventPacket ) );
	TEST( SpecificHdlOutputCalls == 0 );

	TEST_HR( CdiagDereferenceSession( Ssn ) );
}

CFIX_BEGIN_FIXTURE( Session )
	CFIX_FIXTURE_ENTRY( TestCreateAndCloseSession )
	CFIX_FIXTURE_ENTRY( TestHandling )
CFIX_END_FIXTURE()

