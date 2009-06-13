/*----------------------------------------------------------------------
 * Purpose:
 *		Stack trace generation.
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
#define CFIXAPI
#define DBGHELP_TRANSLATE_TCHAR

#define CFIXP_MAX_SYMBOL_NAME_CCH 384

#include "cfixp.h"
#include <dbghelp.h>

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

//
// Lock for serializing dbghelp usage.
//
static CRITICAL_SECTION CfixsDbgHelpLock;
static BOOL CfixsDbgSymInitialized = FALSE;

typedef struct _CFIXP_SYMBOL_WITH_NAME
{
	SYMBOL_INFO Base;
	WCHAR __NameBuffer[ CFIXP_MAX_SYMBOL_NAME_CCH - 1 ];
} CFIXP_SYMBOL_WITH_NAME, *PCFIXP_SYMBOL_WITH_NAME;


BOOL CfixpSetupStackTraceCapturing()
{
	InitializeCriticalSection( &CfixsDbgHelpLock );
	return TRUE;
}

VOID CfixpTeardownStackTraceCapturing()
{
	DeleteCriticalSection( &CfixsDbgHelpLock );
}

#ifdef _M_IX86
	//
	// Disable global optimization and ignore /GS waning caused by 
	// inline assembly.
	//
	#pragma optimize( "g", off )
	#pragma warning( push )
	#pragma warning( disable : 4748 )
#endif
HRESULT CfixpCaptureStackTrace(
	__in_opt CONST PCONTEXT InitialContext,
	__in PCFIX_STACKTRACE StackTrace,
	__in UINT MaxFrames 
	)
{
	DWORD MachineType;
	CONTEXT Context;
	HRESULT Hr;
	STACKFRAME64 StackFrame;

	ASSERT( StackTrace );
	ASSERT( MaxFrames > 0 );

	if ( InitialContext == NULL )
	{
		//
		// Use current context.
		//
		// N.B. GetThreadContext cannot be used on the current thread.
		// Capture own context - on i386, there is no API for that.
		//
#ifdef _M_IX86
		ZeroMemory( &Context, sizeof( CONTEXT ) );

		Context.ContextFlags = CONTEXT_CONTROL;
		
		__asm
		{
		Label:
			mov [Context.Ebp], ebp;
			mov [Context.Esp], esp;
			mov eax, [Label];
			mov [Context.Eip], eax;
		}
#else
		RtlCaptureContext( &Context );
#endif	
	}
	else
	{
		CopyMemory( &Context, InitialContext, sizeof( CONTEXT ) ); 
	}

	//
	// Set up stack frame.
	//
	ZeroMemory( &StackFrame, sizeof( STACKFRAME64 ) );
#ifdef _M_IX86
	MachineType						= IMAGE_FILE_MACHINE_I386;
	StackFrame.AddrPC.Offset		= Context.Eip;
	StackFrame.AddrPC.Mode			= AddrModeFlat;
	StackFrame.AddrFrame.Offset		= Context.Ebp;
	StackFrame.AddrFrame.Mode		= AddrModeFlat;
	StackFrame.AddrStack.Offset		= Context.Esp;
	StackFrame.AddrStack.Mode		= AddrModeFlat;
#elif _M_X64
	MachineType						= IMAGE_FILE_MACHINE_AMD64;
	StackFrame.AddrPC.Offset		= Context.Rip;
	StackFrame.AddrPC.Mode			= AddrModeFlat;
	StackFrame.AddrFrame.Offset		= Context.Rsp;
	StackFrame.AddrFrame.Mode		= AddrModeFlat;
	StackFrame.AddrStack.Offset		= Context.Rsp;
	StackFrame.AddrStack.Mode		= AddrModeFlat;
#elif _M_IA64
	MachineType						= IMAGE_FILE_MACHINE_IA64;
	StackFrame.AddrPC.Offset		= Context.StIIP;
	StackFrame.AddrPC.Mode			= AddrModeFlat;
	StackFrame.AddrFrame.Offset		= Context.IntSp;
	StackFrame.AddrFrame.Mode		= AddrModeFlat;
	StackFrame.AddrBStore.Offset	= Context.RsBSP;
	StackFrame.AddrBStore.Mode		= AddrModeFlat;
	StackFrame.AddrStack.Offset		= Context.IntSp;
	StackFrame.AddrStack.Mode		= AddrModeFlat;
#else
	#error "Unsupported platform"
#endif

	StackTrace->FrameCount = 0;

	//
	// Dbghelp is is singlethreaded.
	//
	EnterCriticalSection( &CfixsDbgHelpLock );

	if ( CfixsDbgSymInitialized )
	{
		Hr = S_OK;
	}
	else
	{
		if ( SymInitialize( GetCurrentProcess(), NULL, TRUE ) )
		{
			Hr = S_OK;
			CfixsDbgSymInitialized = TRUE;
		}
		else
		{
			Hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}

	if ( SUCCEEDED( Hr ) )
	{
		while ( StackTrace->FrameCount < MaxFrames )
		{
			if ( ! StackWalk64(
				MachineType,
				GetCurrentProcess(),
				GetCurrentThread(),
				&StackFrame,
				MachineType == IMAGE_FILE_MACHINE_I386 
					? NULL
					: &Context,
				NULL,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				NULL ) )
			{
				//
				// Maybe it failed, maybe we have finished walking the stack.
				//
				break;
			}

			if ( StackFrame.AddrPC.Offset != 0 )
			{
				//
				// Valid frame.
				//
				StackTrace->Frames[ StackTrace->FrameCount++ ] = 
					StackFrame.AddrPC.Offset;
			}
			else
			{
				//
				// Base reached.
				//
				break;
			}
		}
	}

	LeaveCriticalSection( &CfixsDbgHelpLock );

	return S_OK;
}
#ifdef _M_IX86
	#pragma warning( pop )
	#pragma optimize( "g", on )
#endif

HRESULT CFIXCALLTYPE CfixpGetInformationStackframe(
	__in ULONGLONG Frame,
	__in SIZE_T ModuleNameCch,
	__out_ecount(ModuleNameCch) PWSTR ModuleName,
	__in SIZE_T FunctionNameCch,
	__out_ecount(FunctionNameCch) PWSTR FunctionName,
	__out PDWORD Displacement,
	__in SIZE_T SourceFileCch,
	__out_ecount(SourceFileCch) PWSTR SourceFile,
	__out PDWORD SourceLine 
	)
{
	HRESULT Hr;
	DWORD64 Displacement64;
	DWORD LineDisplacement;
	IMAGEHLP_LINE64 LineInfo;
	IMAGEHLP_MODULE64 ModuleInfo;
	CFIXP_SYMBOL_WITH_NAME Symbol;

	if ( Frame == 0 ||
		 ModuleNameCch == 0 ||
		 ! ModuleName ||
		 FunctionNameCch == 0 ||
		 ! FunctionName ||
		 ! Displacement ||
		 SourceFileCch == 0 ||
		 ! SourceFile ||
		 ! SourceLine )
	{
		return E_INVALIDARG;
	}

	EnterCriticalSection( &CfixsDbgHelpLock );

	//
	// Module name.
	//
	ModuleInfo.SizeOfStruct = sizeof( IMAGEHLP_MODULE64 );
	if ( ! SymGetModuleInfo64(
		GetCurrentProcess(),
		Frame,
		&ModuleInfo ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	Hr = StringCchCopy( 
		ModuleName,
		ModuleNameCch,
		ModuleInfo.ModuleName );
	if ( FAILED( Hr ) )
	{
		goto Cleanup;
	}

	//
	// Function.
	//
	ZeroMemory( &Symbol, sizeof( CFIXP_SYMBOL_WITH_NAME ) );
	Symbol.Base.SizeOfStruct = sizeof( SYMBOL_INFO );
	Symbol.Base.MaxNameLen = CFIXP_MAX_SYMBOL_NAME_CCH;
	if ( ! SymFromAddr(
		GetCurrentProcess(),
		Frame,
		&Displacement64,
		&Symbol.Base ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	*Displacement = ( DWORD ) Displacement64;

	if ( 0 == UnDecorateSymbolName(
		Symbol.Base.Name,
		FunctionName,
		( DWORD ) FunctionNameCch,
		UNDNAME_COMPLETE ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	//
	// Source file/line.
	//
	LineInfo.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
	if ( SymGetLineFromAddr64(
		GetCurrentProcess(),
		Frame,
		&LineDisplacement,
		&LineInfo ) )
	{
		*SourceLine = LineInfo.LineNumber;
		Hr = StringCchCopy( 
			SourceFile,
			SourceFileCch,
			LineInfo.FileName );
		if ( FAILED( Hr ) )
		{
			goto Cleanup;
		}
	}
	else
	{
		*SourceLine = 0;
		SourceFile[ 0 ] = L'\0';
	}

	Hr = S_OK;

Cleanup:
	LeaveCriticalSection( &CfixsDbgHelpLock );

	return Hr;
}