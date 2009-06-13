/*----------------------------------------------------------------------
 *	Purpose:
 *		Wrapper functions for cfixkr IOCTLs.
 *
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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

#include "cfixklp.h"
#include <stdlib.h>
#include <shlwapi.h>
#include <psapi.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

static VOID CfixklsCopyExceptionRecord(
	__in PCFIXKR_EXECUTION_EVENT Event,
	__out PEXCEPTION_RECORD Target
	)
{
#ifdef _WIN32
	if ( Event->Info.UncaughtException.Type == CfixkrExceptionRecord64 )
	{
		//
		// Structure in Event is 64 bit - convert it to 32 bit.
		//
		PEXCEPTION_RECORD64 Source	= &Event->Info.UncaughtException.u.ExceptionRecord64;
		Target->ExceptionCode		= Source->ExceptionCode;
		Target->ExceptionFlags		= Source->ExceptionFlags;
		Target->NumberParameters	= 0;

		//
		// ExceptionAddress, ExceptionRecord and ExceptionInformation
		// do not fit into the 32 bit struct.
		//
		Target->ExceptionAddress	= ( PVOID ) ( ULONG_PTR ) 0xFFFFFFFF;
		Target->ExceptionRecord		= 0;
	}
	else
	{
		CopyMemory( 
			Target, 
			&Event->Info.UncaughtException.u.ExceptionRecord32,
			sizeof( EXCEPTION_RECORD32 ) );
	}
#else
	CopyMemory( 
		Target, 
		&Event->Info.UncaughtException.u.ExceptionRecord64,
		sizeof( EXCEPTION_RECORD64 ) );
#endif
}
	

static HRESULT CfixklsFindBaseAddressDriver(
	__in HANDLE ReflectorHandle,
	__in PCWSTR DriverImagePath,
	__out PULONGLONG BaseAddress
	)
{
	DWORD BytesTransferred;
	WCHAR DriverBaseName[ MAX_PATH ];
	HRESULT Hr;
	ULONG Index;
	WCHAR FileName[ MAX_PATH ];
	CFIXKR_IOCTL_GET_MODULES ResponseStatic;
	PCFIXKR_IOCTL_GET_MODULES ResponseAllocated = NULL;
	PCFIXKR_IOCTL_GET_MODULES Response = NULL;

	ASSERT( ReflectorHandle );
	ASSERT( DriverImagePath );
	ASSERT( BaseAddress );

	*BaseAddress = 0;

	//
	// Get basename of driver file.
	//
	Hr = StringCchCopy(
		DriverBaseName,
		_countof( DriverBaseName ),
		DriverImagePath );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	PathStripPath( DriverBaseName );

	//
	// Get list of drivers that have registered at the reflector.
	// Response only has space for a single driver address, so we
	// may need to adapt the buffer size and query again.
	//
	if ( DeviceIoControl(
		ReflectorHandle,
		CFIXKR_IOCTL_GET_TEST_MODULES,
		NULL,
		0,
		&ResponseStatic,
		sizeof( CFIXKR_IOCTL_GET_MODULES ),
		&BytesTransferred,
		NULL ) )
	{
		Response = &ResponseStatic;
	}
	else
	{
		DWORD Err = GetLastError();
		if ( Err == ERROR_MORE_DATA )
		{
			DWORD ResponseSize;

			//
			// Buffer was too small, ResponseStatic.Count contains
			// required element count.
			//
			ResponseSize = RTL_SIZEOF_THROUGH_FIELD(
					CFIXKR_IOCTL_GET_MODULES,
					DriverLoadAddress[ ResponseStatic.Count - 1 ] );

			ResponseAllocated = malloc( ResponseSize );
			if ( ResponseAllocated == NULL )
			{
				Hr = E_OUTOFMEMORY;
				goto Cleanup;
			}

			//
			// Query again with dynamically allocated response buffer.
			//
			if ( DeviceIoControl(
				ReflectorHandle,
				CFIXKR_IOCTL_GET_TEST_MODULES,
				NULL,
				0,
				ResponseAllocated,
				ResponseSize,
				&BytesTransferred,
				NULL ) )
			{
				Response = ResponseAllocated;
			}
			else
			{
				Hr = HRESULT_FROM_WIN32( GetLastError() );
				goto Cleanup;
			}
		}
		else
		{
			Hr = HRESULT_FROM_WIN32( Err );
			goto Cleanup;
		}
	}

	ASSERT( Response != NULL );

	if ( BytesTransferred < sizeof( ULONG ) ||
		 BytesTransferred < Response->Count * sizeof( ULONGLONG ) )
	{
		Hr = CFIXKL_E_INVALID_REFLECTOR_RESPONSE;
		goto Cleanup;
	}

	ASSERT( Response->Count < 8 );	// Sanity.

	//
	// Search for the one we are after.
	//
	for ( Index = 0; Index < Response->Count; Index++ )
	{
		ASSERT( Response->DriverLoadAddress[ Index ] & 0x80000000 );
		ASSERT( ( Response->DriverLoadAddress[ Index ] & 0x00000FFF ) == 0 );

		//
		// Get file name of driver and compare.
		//
		if ( 0 == GetDeviceDriverBaseName(
			( PVOID ) Response->DriverLoadAddress[ Index ],
			FileName,
			_countof( FileName ) ) )
		{
			Hr = HRESULT_FROM_WIN32( GetLastError() );
			
			//
			// Anyway...
			//
			continue;
		}

		if ( 0 == _wcsicmp( DriverBaseName, FileName ) )
		{
			//
			// Found it.
			//
			Hr = S_OK;
			*BaseAddress = Response->DriverLoadAddress[ Index ];
			break;
		}
	}

Cleanup:
	if ( ResponseAllocated != NULL )
	{
		free( ResponseAllocated );
	}

	if ( FAILED( Hr ) )
	{
		return Hr;
	}
	else 
	{
		if ( *BaseAddress != 0 )
		{
			return S_OK;
		}
		else
		{
			return CFIXKL_E_UNKNOWN_LOAD_ADDRESS;
		}
	}
}

static HRESULT CfixklsQuerySizeForQueryModuleRequest(
	__in HANDLE ReflectorHandle,
	__in ULONGLONG DriverBaseAddress,
	__out PULONG RequiredSize 
	)
{
	DWORD BytesTransferred;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST Request;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE Response;

	ASSERT( ReflectorHandle );
	ASSERT( DriverBaseAddress );
	ASSERT( RequiredSize );

	*RequiredSize = 0;

	Request.DriverBaseAddress = DriverBaseAddress;

	if ( DeviceIoControl(
		ReflectorHandle,
		CFIXKR_IOCTL_QUERY_TEST_MODULE,
		&Request,
		sizeof( CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST ),
		&Response,
		sizeof( CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE ),
		&BytesTransferred,
		NULL ) )
	{
		//
		// Normally, this should never succeed. If it does, it means
		// that the driver does not have any exports - thus the
		// buffer was large enough.
		//
		*RequiredSize = FIELD_OFFSET( 
			CFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE,
			u.TestModule.FixtureOffsets );
		return S_OK;
	}

	if ( GetLastError() == ERROR_MORE_DATA &&
		 BytesTransferred >= sizeof( ULONG ) )
	{
		//
		// That's what we want. Response will contain the required
		// site for a 'real' request.
		//
		*RequiredSize = Response.u.SizeRequired;
		return S_OK;
	}
	else
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}
}

static HRESULT CfixklsReportEventToExecutionContext(
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in PCFIXKR_EXECUTION_EVENT Event
	)
{
	PCFIX_TESTCASE_EXECUTION_EVENT EventTranslated;
	HRESULT Hr;

	PWSTR ExpressionBuffer = NULL;
	PWSTR FileBuffer = NULL;
	PWSTR MessageBuffer = NULL;
	PWSTR RoutineBuffer = NULL;
	
	EventTranslated = ( PCFIX_TESTCASE_EXECUTION_EVENT )
		malloc( FIELD_OFFSET( 
			CFIX_TESTCASE_EXECUTION_EVENT,
			StackTrace.Frames[ Event->StackTrace.FrameCount ] ) );
	if ( EventTranslated == NULL )
	{
		return E_OUTOFMEMORY;
	}

	EventTranslated->Type = Event->Type;

	//
	// Copy fields. This requires converting between counted strings
	// and zero-terminated strings - thus, the buffers are required.
	//
	switch ( Event->Type )
	{
	case CfixEventFailedAssertion:
		FileBuffer = malloc( Event->Info.FailedAssertion.FileLength + sizeof( UNICODE_NULL ) );
		RoutineBuffer = malloc( Event->Info.FailedAssertion.RoutineLength + sizeof( UNICODE_NULL ) );
		ExpressionBuffer = malloc( Event->Info.FailedAssertion.ExpressionLength + sizeof( UNICODE_NULL ) );

		if ( FileBuffer == NULL ||
			 RoutineBuffer == NULL ||
			 ExpressionBuffer == NULL )
		{
			Hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		ASSERT( ( Event->Info.FailedAssertion.FileLength % 2 ) == 0 );
		CopyMemory( 
			FileBuffer,
			( PUCHAR ) Event + Event->Info.FailedAssertion.FileOffset,
			Event->Info.FailedAssertion.FileLength );
		FileBuffer[ Event->Info.FailedAssertion.FileLength / 2 ] = UNICODE_NULL;
		EventTranslated->Info.FailedAssertion.File = FileBuffer;

		ASSERT( ( Event->Info.FailedAssertion.RoutineLength % 2 ) == 0 );
		CopyMemory( 
			RoutineBuffer,
			( PUCHAR ) Event + Event->Info.FailedAssertion.RoutineOffset,
			Event->Info.FailedAssertion.RoutineLength );
		RoutineBuffer[ Event->Info.FailedAssertion.RoutineLength / 2 ] = UNICODE_NULL;
		EventTranslated->Info.FailedAssertion.Routine = RoutineBuffer;

		EventTranslated->Info.FailedAssertion.Line = Event->Info.FailedAssertion.Line;

		ASSERT( ( Event->Info.FailedAssertion.ExpressionLength % 2 ) == 0 );
		CopyMemory( 
			ExpressionBuffer,
			( PUCHAR ) Event + Event->Info.FailedAssertion.ExpressionOffset,
			Event->Info.FailedAssertion.ExpressionLength );
		ExpressionBuffer[ Event->Info.FailedAssertion.ExpressionLength / 2 ] = UNICODE_NULL;
		EventTranslated->Info.FailedAssertion.Expression = ExpressionBuffer;

		EventTranslated->Info.FailedAssertion.LastError = 0;	// Does not apply.
		break;

	case CfixEventUncaughtException:
		CfixklsCopyExceptionRecord(
			Event,
			&EventTranslated->Info.UncaughtException.ExceptionRecord );
		break;

	case CfixEventInconclusiveness:
		MessageBuffer = malloc( Event->Info.Inconclusiveness.MessageLength + sizeof( UNICODE_NULL ) );
		
		if ( MessageBuffer == NULL )
		{
			Hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		ASSERT( ( Event->Info.Inconclusiveness.MessageLength % 2 ) == 0 );
		CopyMemory( 
			MessageBuffer,
			( PUCHAR ) Event + Event->Info.Inconclusiveness.MessageOffset,
			Event->Info.Inconclusiveness.MessageLength );
		MessageBuffer[ Event->Info.Inconclusiveness.MessageLength / 2 ] = UNICODE_NULL;
		EventTranslated->Info.Inconclusiveness.Message = MessageBuffer;
		break;

	case CfixEventLog:
		MessageBuffer = malloc( Event->Info.Log.MessageLength + sizeof( UNICODE_NULL ) );
		
		if ( MessageBuffer == NULL )
		{
			Hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		ASSERT( ( Event->Info.Log.MessageLength % 2 ) == 0 );
		CopyMemory( 
			MessageBuffer,
			( PUCHAR ) Event + Event->Info.Log.MessageOffset,
			Event->Info.Log.MessageLength );
		MessageBuffer[ Event->Info.Log.MessageLength / 2 ] = UNICODE_NULL;
		EventTranslated->Info.Log.Message = MessageBuffer;
		break;

	default:
		ASSERT( !"Unrecognized event type" );
		break;
	}

	CopyMemory( 
		&EventTranslated->StackTrace,
		&Event->StackTrace,
		FIELD_OFFSET( 
			CFIX_STACKTRACE,
			Frames[ Event->StackTrace.FrameCount ] ) );

	//
	// Report. Note that the dispositions are ignored,
	// since all decisions have already been made based on the default
	// dispositions.
	//
	( VOID ) Context->ReportEvent(
		Context,
		EventTranslated );

	Hr = S_OK;

Cleanup:
	if ( ExpressionBuffer	!= NULL )	free( ExpressionBuffer );
	if ( FileBuffer			!= NULL )	free( FileBuffer );
	if ( MessageBuffer		!= NULL )	free( MessageBuffer ); 
	if ( RoutineBuffer		!= NULL )	free( RoutineBuffer ); 
	
	free( EventTranslated );

	return Hr;
}
/*----------------------------------------------------------------------
 *
 * Internals.
 *
 */

HRESULT CfixklpQueryModule(
	__in HANDLE ReflectorHandle,
	__in PCWSTR DriverPath,
	__out ULONGLONG *DriverBaseAddress,
	__out PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE *Response
	)
{
	DWORD BytesTransferred;
	ULONGLONG BaseAddress;
	HRESULT Hr;
	CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST QueryRequest;
	PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE QueryResponse;
	ULONG SizeRequiredForQueryRequest;

	ASSERT( ReflectorHandle );
	ASSERT( DriverPath );
	ASSERT( DriverBaseAddress );
	ASSERT( Response );

	*Response = NULL;

	//
	// Find address where driver loaded.
	//
	Hr = CfixklsFindBaseAddressDriver(
		ReflectorHandle,
		DriverPath,
		&BaseAddress );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	//
	// Query information about module.
	//
	Hr = CfixklsQuerySizeForQueryModuleRequest(
		ReflectorHandle,
		BaseAddress,
		&SizeRequiredForQueryRequest );
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	QueryRequest.DriverBaseAddress = BaseAddress;
	QueryResponse = ( PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE )
		malloc( SizeRequiredForQueryRequest );
	if ( QueryResponse == NULL )
	{
		return E_OUTOFMEMORY;
	}

	if ( DeviceIoControl(
		ReflectorHandle,
		CFIXKR_IOCTL_QUERY_TEST_MODULE,
		&QueryRequest,
		sizeof( CFIXKR_IOCTL_QUERY_TEST_MODULE_REQUEST ),
		QueryResponse,
		SizeRequiredForQueryRequest,
		&BytesTransferred,
		NULL ) )
	{
		ASSERT( BytesTransferred == SizeRequiredForQueryRequest );

		Hr = S_OK;
		*DriverBaseAddress	= BaseAddress;
		*Response			= QueryResponse;
	}
	else
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
	}

	return Hr;
}

VOID CfixklpFreeQueryModuleResponse(
	__in PCFIXKR_IOCTL_QUERY_TEST_MODULE_RESPONSE Response
	)
{
	ASSERT( Response );
	free( Response );
}

HRESULT CfixklpCallRoutine(
	__in HANDLE ReflectorHandle,
	__in ULONGLONG DriverBaseAddress,
	__in PCFIX_EXECUTION_CONTEXT Context,
	__in USHORT FixtureKey,
	__in USHORT RoutineKey,
	__out PBOOL RoutineRanToCompletion,
	__out PBOOL AbortRun
	)
{
	DWORD BytesTransferred;
	PCFIXKR_EXECUTION_EVENT Event;
	HRESULT Hr;
	ULONG Index;
	CFIXKR_IOCTL_CALL_ROUTINE_REQUEST Request;
	PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE Response;
	ULONG ResponseSize = 4096;

	ASSERT( ReflectorHandle );
	ASSERT( DriverBaseAddress );
	ASSERT( Context );
	ASSERT( RoutineRanToCompletion );
	ASSERT( AbortRun );

	*RoutineRanToCompletion		= FALSE;
	*AbortRun					= FALSE;

	Request.DriverBaseAddress	= DriverBaseAddress;
	Request.FixtureKey			= FixtureKey;
	Request.RoutineKey			= RoutineKey;

	Request.Dispositions.FailedAssertion = Context->QueryDefaultDisposition(
		Context, CfixEventFailedAssertion );
	Request.Dispositions.UnhandledException = Context->QueryDefaultDisposition(
		Context, CfixEventUncaughtException );

	//
	// Allocate response, which has to provide space for all events
	// that may be generated during the call.
	//
	Response = ( PCFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) malloc( ResponseSize );
	if ( Response == NULL )
	{
		return E_OUTOFMEMORY;
	}

	if ( ! DeviceIoControl(
		ReflectorHandle,
		CFIXKR_IOCTL_CALL_ROUTINE,
		&Request,
		sizeof( CFIXKR_IOCTL_CALL_ROUTINE_REQUEST ),
		Response,
		ResponseSize,
		&BytesTransferred,
		NULL ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	if ( BytesTransferred < sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) )
	{
		Hr = CFIXKL_E_INVALID_REFLECTOR_RESPONSE;
		goto Cleanup;
	}

	*RoutineRanToCompletion = Response->RoutineRanToCompletion;
	*AbortRun				= Response->AbortRun;

	//
	// Pass events to context. 
	//
	Event = ( PCFIXKR_EXECUTION_EVENT ) 
		( ( PUCHAR ) Response + sizeof( CFIXKR_IOCTL_CALL_ROUTINE_RESPONSE ) );
	for ( Index = 0; Index < Response->Events.Count; Index++ )
	{
		Hr = CfixklsReportEventToExecutionContext( Context, Event );
		if ( FAILED( Hr ) )
		{
			break;
		}

		Event = ( PCFIXKR_EXECUTION_EVENT ) ( ( PUCHAR ) Event + Event->Size );
	}

	if ( Response->Events.Flags & CFIXKR_CALL_ROUTINE_FLAG_EVENTS_TRUNCATED )
	{
		CFIX_TESTCASE_EXECUTION_EVENT Note;
		Note.Type = CfixEventLog;
		Note.StackTrace.FrameCount = 0;
		Note.Info.Log.Message = L"Note: At least one event has been dropped";

		( VOID ) Context->ReportEvent( Context, &Note );
	}

	Hr = S_OK;
	
Cleanup:
	free( Response );
	return Hr;
}