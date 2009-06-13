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
#include <stdlib.h>
#include <shlwapi.h>

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

typedef BOOL ( *CFIXP_SYMINITIALIZEW_PROC )(
	__in HANDLE hProcess,
	__in_opt PCWSTR UserSearchPath,
	__in BOOL fInvadeProcess
	);

typedef PVOID ( *CFIXP_SYMFUNCTIONTABLEACCESS64_PROC )(
	__in HANDLE hProcess,
	__in DWORD64 AddrBase
	);

typedef DWORD64 ( *CFIXP_SYMGETMODULEBASE64_PROC )(
	__in HANDLE hProcess,
	__in DWORD64 qwAddr
	);

typedef BOOL ( *CFIXP_STACKWALK64_PROC )(
	__in DWORD MachineType,
	__in HANDLE hProcess,
	__in HANDLE hThread,
	__inout LPSTACKFRAME64 StackFrame,
	__inout PVOID ContextRecord,
	__in_opt PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
	__in_opt PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
	__in_opt PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
	__in_opt PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
	);

typedef BOOL ( *CFIXP_SYMGETLINEFROMADDR64_PROC )(
	__in HANDLE hProcess,
	__in DWORD64 qwAddr,
	__out PDWORD pdwDisplacement,
	__out PIMAGEHLP_LINE64 Line64
	);

typedef DWORD ( WINAPI *CFIXP_UNDECORATESYMBOLNAMEW_PROC )(
	__in PCWSTR name,
	__out_ecount(maxStringLength) PWSTR outputString,
	__in DWORD maxStringLength,
	__in DWORD flags
	);

typedef BOOL ( *CFIXP_SYMFROMADDRW_PROC )(
	__in HANDLE hProcess,
	__in DWORD64 Address,
	__out_opt PDWORD64 Displacement,
	__inout PSYMBOL_INFOW Symbol
	);

typedef BOOL ( *CFIXP_SYMGETMODULEINFO64_PROC )(
	__in HANDLE hProcess,
	__in DWORD64 qwAddr,
	__out PIMAGEHLP_MODULE64 ModuleInfo
	);

typedef struct _CFIXP_DBGHELP
{
	HMODULE Module;

	CFIXP_SYMINITIALIZEW_PROC SymInitialize;
	CFIXP_SYMFUNCTIONTABLEACCESS64_PROC SymFunctionTableAccess64;
	CFIXP_SYMGETMODULEBASE64_PROC SymGetModuleBase64;
	CFIXP_STACKWALK64_PROC StackWalk64;
	CFIXP_SYMGETLINEFROMADDR64_PROC SymGetLineFromAddr64;
	CFIXP_UNDECORATESYMBOLNAMEW_PROC UnDecorateSymbolName;
	CFIXP_SYMFROMADDRW_PROC SymFromAddr;
	CFIXP_SYMGETMODULEINFO64_PROC SymGetModuleInfo64;
} CFIXP_DBGHELP, *PCFIXP_DBGHELP;

static CFIXP_DBGHELP CfixsDbghelp = { 0 };

/*------------------------------------------------------------------------------
 *
 * Initialization/Teardown - called by DllMain.
 *
 */

BOOL CfixpSetupStackTraceCapturing()
{
	InitializeCriticalSection( &CfixsDbgHelpLock );
	return TRUE;
}

VOID CfixpTeardownStackTraceCapturing()
{
	DeleteCriticalSection( &CfixsDbgHelpLock );

	if ( CfixsDbghelp.Module )
	{
		FreeLibrary( CfixsDbghelp.Module );
	}
}

static HRESULT CfixsLinkDbghelp()
{
	WCHAR ModulePath[ MAX_PATH ];

	//
	// Use dbghelp residing in same directory as this module.
	//

	if ( 0 == GetModuleFileName( 
		CfixpModule,
		ModulePath,
		_countof( ModulePath ) ) )
	{
		return HRESULT_FROM_WIN32( GetLastError() );
	}

	PathRemoveFileSpec( ModulePath );
	PathAppend( ModulePath, L"dbghelp.dll" );

	if ( GetFileAttributes( ModulePath ) == INVALID_FILE_ATTRIBUTES )
	{
		return CFIX_E_DBGHELP_MISSING;
	}

	CfixsDbghelp.Module = LoadLibrary( ModulePath );
	if ( ! CfixsDbghelp.Module )
	{
		return CFIX_E_LOADING_DBGHELP_FAILED;
	}

	CfixsDbghelp.SymInitialize = ( CFIXP_SYMINITIALIZEW_PROC )
		GetProcAddress( CfixsDbghelp.Module, "SymInitializeW" );
	CfixsDbghelp.SymFunctionTableAccess64 = ( CFIXP_SYMFUNCTIONTABLEACCESS64_PROC )
		GetProcAddress( CfixsDbghelp.Module, "SymFunctionTableAccess64" );
	CfixsDbghelp.SymGetModuleBase64 = ( CFIXP_SYMGETMODULEBASE64_PROC )
		GetProcAddress( CfixsDbghelp.Module, "SymGetModuleBase64" );
	CfixsDbghelp.StackWalk64 = ( CFIXP_STACKWALK64_PROC )
		GetProcAddress( CfixsDbghelp.Module, "StackWalk64" );
	CfixsDbghelp.SymGetLineFromAddr64 = ( CFIXP_SYMGETLINEFROMADDR64_PROC )
		GetProcAddress( CfixsDbghelp.Module, "SymGetLineFromAddrW64" );
	CfixsDbghelp.UnDecorateSymbolName = ( CFIXP_UNDECORATESYMBOLNAMEW_PROC )
		GetProcAddress( CfixsDbghelp.Module, "UnDecorateSymbolNameW" );
	CfixsDbghelp.SymFromAddr = ( CFIXP_SYMFROMADDRW_PROC )
		GetProcAddress( CfixsDbghelp.Module, "SymFromAddrW" );
	CfixsDbghelp.SymGetModuleInfo64 = ( CFIXP_SYMGETMODULEINFO64_PROC )
		GetProcAddress( CfixsDbghelp.Module, "SymGetModuleInfoW64" );

	if (
		CfixsDbghelp.SymInitialize == NULL ||
		CfixsDbghelp.SymFunctionTableAccess64 == NULL ||
		CfixsDbghelp.SymGetModuleBase64 == NULL ||
		CfixsDbghelp.StackWalk64 == NULL ||
		CfixsDbghelp.SymGetLineFromAddr64 == NULL ||
		CfixsDbghelp.UnDecorateSymbolName == NULL ||
		CfixsDbghelp.SymFromAddr == NULL ||
		CfixsDbghelp.SymGetModuleInfo64 == NULL )
	{
		return CFIX_E_LOADING_DBGHELP_FAILED;
	}

	return S_OK;
}

static HRESULT CfixsLazyInitializeDbgHelp()
{
	if ( CfixsDbgSymInitialized )
	{
		return S_OK;
	}
	else
	{
		HRESULT Hr = CfixsLinkDbghelp();
		if ( FAILED( Hr ) )
		{
			return Hr;
		}

		if ( CfixsDbghelp.SymInitialize( GetCurrentProcess(), NULL, TRUE ) )
		{
			CfixsDbgSymInitialized = TRUE;
			return S_OK;
		}
		else
		{
			return HRESULT_FROM_WIN32( GetLastError() );
		}
	}
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

	Hr = CfixsLazyInitializeDbgHelp();
	if ( SUCCEEDED( Hr ) )
	{
		while ( StackTrace->FrameCount < MaxFrames )
		{
			if ( ! CfixsDbghelp.StackWalk64(
				MachineType,
				GetCurrentProcess(),
				GetCurrentThread(),
				&StackFrame,
				MachineType == IMAGE_FILE_MACHINE_I386 
					? NULL
					: &Context,
				NULL,
				CfixsDbghelp.SymFunctionTableAccess64,
				CfixsDbghelp.SymGetModuleBase64,
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

	return Hr;
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

	Hr = CfixsLazyInitializeDbgHelp();
	if ( FAILED( Hr ) )
	{
		return Hr;
	}

	//
	// Module name.
	//
	ModuleInfo.SizeOfStruct = sizeof( IMAGEHLP_MODULE64 );
	if ( ! CfixsDbghelp.SymGetModuleInfo64(
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
	if ( ! CfixsDbghelp.SymFromAddr(
		GetCurrentProcess(),
		Frame,
		&Displacement64,
		&Symbol.Base ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	*Displacement = ( DWORD ) Displacement64;

	if ( 0 == CfixsDbghelp.UnDecorateSymbolName(
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
	if ( CfixsDbghelp.SymGetLineFromAddr64(
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