#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Cfix auxilliary header file - defines structures used for
 *		PE-based test modules. Do not include directly - 
 *		include either cfixapi.h or cfix.h.
 *
 *		Note: Test code should include cfix.h.
 *
 *            cfixaux.h        cfixkrio.h
 *              ^ ^ ^--------+     ^
 *             /   \          \   /
 *            /     \          \ /
 *		cfixapi.h  cfixpe.h  cfixkr.h
 *			^	  ^	  ^         
 *			|	 /	  |         
 *			|	/	  |         
 *		  [cfix]	cfix.h      
 *                    ^         
 *                    |         
 *                    |         
 *          [Test DLLs/Drivers] 
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

#include <cfixaux.h>

#ifndef MAKELONG
#define MAKELONG(a, b)      ((LONG)(((SHORT)((ULONG_PTR)(a) & 0xffff)) | ((ULONG)((SHORT)((DWORD_PTR)(b) & 0xffff))) << 16))
#endif

#define CFIX_MAX_THREADS	MAXIMUM_WAIT_OBJECTS

/*++
	Exception Description:
		Thrown when a test case turns out to be inconclusive. 
		Handled internally.
--*/
#define EXCEPTION_TESTCASE_INCONCLUSIVE ( ( ULONG ) 0x8004AFFEUL )

/*++
	Exception Description:
		Thrown when a test case has failed.
		Handled internally.
--*/
#define EXCEPTION_TESTCASE_FAILED		( ( ULONG ) 0x8004AFFFUL )

/*++
	Exception Description:
		Thrown when a test case has failed and the test case should
		be aborted.
		Handled internally.
--*/
#define EXCEPTION_TESTCASE_FAILED_ABORT	( ( ULONG ) 0x8004AFFDUL )

/*----------------------------------------------------------------------
 *
 * Embedding.
 *
 */

#if ! defined( CFIX_KERNELMODE ) && ! defined( CFIX_NO_EMBEDDING )

typedef void ( __cdecl * CFIX_CRT_INIT_ROUTINE )();

/*++
	Routine Description:
		Embedding initialization routine.

		If the routine returns CFIX_S_EXIT_PROCESS, the process
		will be terminated so that main is not called.
--*/
typedef HRESULT ( CFIXCALLTYPE * CFIX_EMBEDDING_ROUTINE )();

/*++
	Routine Description:
		CRT initializer function, called before main(). Accepts
		a routine to be specified via a environment variable that
		will be called at the end of CRT initialization.

		The routine has to conform to CFIX_EMBEDDING_ROUTINE.
--*/
EXTERN_C __inline void __cdecl CfixpCrtInitEmbedding()
{
	PSTR Bang;
	HRESULT Hr;
	PCSTR ModuleName;
	CHAR RoutineNameBuffer[ 100 ];
	PCSTR RoutineName;

	HMODULE Module;
	CFIX_EMBEDDING_ROUTINE Routine;

	//
	// N.B. No threadsafe initialization required.
	//
	static BOOL Initialized = FALSE;
	if ( Initialized )
	{
		//
		// Call from test map; ignore.
		//
		return;
	}
	else
	{
		Initialized = TRUE;
	}

	// printf( "CfixpCrtInitEmbedding()\n" );

	if ( 0 == GetEnvironmentVariableA(
		CFIX_EMB_INIT_ENVVAR_NAMEA,
		RoutineNameBuffer,
		sizeof( RoutineNameBuffer ) / sizeof( *RoutineNameBuffer ) ) )
	{
		return;
	}

	//
	// Variable has format module!routine. Split accordingly.
	//

	Bang = strchr( RoutineNameBuffer, '!' );
	if ( Bang == NULL )
	{
		return;
	}

	*Bang = UNICODE_NULL;
	ModuleName = RoutineNameBuffer;
	RoutineName = Bang + 1;

	Module = LoadLibraryA( ModuleName );
	if ( Module == NULL )
	{
		//
		// Do not bail out here - the executable may have been started
		// without the intent to run tests.
		//
		return;
	}

	Routine = ( CFIX_EMBEDDING_ROUTINE ) 
		GetProcAddress( Module, RoutineName );

	if ( Routine == NULL )
	{
		return;
	}

	Hr = ( Routine )();
	( VOID ) FreeLibrary( Module );

	if ( CFIX_S_EXIT_PROCESS == Hr )
	{
		ExitProcess( 0 );
	}
	else if ( FAILED( Hr ) )
	{
		ExitProcess( ( UINT ) Hr );
	}
}

#ifdef __cplusplus
extern "C" {
#endif

//
// Register as CRT initializer that runs after all C++ constructors
// (these live in .CRT$XCU -- see MSDN).
//
#pragma section( ".CRT$XCX", read )
__declspec( allocate( ".CRT$XCX" ) )
extern const CFIX_CRT_INIT_ROUTINE CfixpCrtInitEmbeddingRegistration;

//
// N.B. To avoid /OPT:REF-caused COMDAT elimination, this variable
// must be referenced elsewhere. This is done by having the test maps
// call CFIX_CALL_CRT_INIT_EMBEDDING_REGISTRATION(), which is effectively
// a no-op.
//
__declspec( selectany )
const CFIX_CRT_INIT_ROUTINE CfixpCrtInitEmbeddingRegistration = CfixpCrtInitEmbedding;

#define CFIX_CALL_CRT_INIT_EMBEDDING_REGISTRATION() \
	( CfixpCrtInitEmbeddingRegistration ) ()


#ifdef __cplusplus
}
#endif

#else
#define CFIX_CALL_CRT_INIT_EMBEDDING_REGISTRATION() 
#endif

/*----------------------------------------------------------------------
 *
 * Definitions for test map.
 *
 * Sampe usage:
 * 
 * CFIX_BEGIN_FIXTURE(Ts01)
 * 	CFIX_FIXTURE_SETUP(Setup)
 * 	CFIX_FIXTURE_TEARDOWN(Tdown)
 * 	CFIX_FIXTURE_ENTRY(TcFunc1)
 * 	CFIX_FIXTURE_ENTRY(TcFunc2)
 * CFIX_END_FIXTURE()
 *
 */
typedef enum
{
	CfixEntryTypeEnd			= 0,
	CfixEntryTypeSetup			= 1,
	CfixEntryTypeTeardown		= 2,
	CfixEntryTypeTestcase		= 3,
	CfixEntryTypeBefore			= 4,
	CfixEntryTypeAfter			= 5
} CFIX_ENTRY_TYPE;


/*++
	Description:
		Prototype of setup, teardown and testcase routines.
--*/
typedef VOID ( CFIXCALLTYPE * CFIX_PE_TESTCASE_ROUTINE )();

/*++
	Struct Description:
		Defines an entry in a test map. Only used by
		CFIX_GET_FIXTURE_ROUTINE-routines.
--*/
typedef struct _CFIX_PE_DEFINITION_ENTRY
{
	CFIX_ENTRY_TYPE Type;
	PCWSTR Name;
	CFIX_PE_TESTCASE_ROUTINE Routine;
} CFIX_PE_DEFINITION_ENTRY, *PCFIX_PE_DEFINITION_ENTRY;

/*++
	Struct Description:
		Defines a test map. Only used by
		CFIX_GET_FIXTURE_ROUTINE-routines.
--*/
typedef struct _CFIX_TEST_PE_DEFINITION
{
	ULONG ApiVersion;
	PCFIX_PE_DEFINITION_ENTRY Entries;
} CFIX_TEST_PE_DEFINITION, *PCFIX_TEST_PE_DEFINITION;

/*++
	Description:
		Prototype of export of test DLL that is generated by test map.
--*/
typedef PCFIX_TEST_PE_DEFINITION ( CFIXCALLTYPE * CFIX_GET_FIXTURE_ROUTINE )();

#define CFIX_PE_API_VERSION	MAKELONG( 1, 0 )

#define CFIX_BEGIN_FIXTURE(name)									\
EXTERN_C __declspec(dllexport)										\
PCFIX_TEST_PE_DEFINITION CFIXCALLTYPE __CfixFixturePe##name()		\
{																	\
	static CFIX_PE_DEFINITION_ENTRY Entries[] = {					\

#define CFIX_FIXTURE_SETUP(func)									\
	{ CfixEntryTypeSetup, __CFIX_WIDE( #func ), func },								

#define CFIX_FIXTURE_TEARDOWN(func)									\
	{ CfixEntryTypeTeardown,__CFIX_WIDE( #func ), func },								

#define CFIX_FIXTURE_ENTRY(func)									\
	{ CfixEntryTypeTestcase, __CFIX_WIDE( #func ), func },								

#define CFIX_FIXTURE_BEFORE(func)									\
	{ CfixEntryTypeBefore, __CFIX_WIDE( #func ), func },								

#define CFIX_FIXTURE_AFTER(func)									\
	{ CfixEntryTypeAfter, __CFIX_WIDE( #func ), func },								

#define CFIX_END_FIXTURE()											\
	{ CfixEntryTypeEnd, NULL, NULL }								\
	};																\
	static CFIX_TEST_PE_DEFINITION Fixture = {						\
		CFIX_PE_API_VERSION,										\
		Entries														\
	};																\
	CFIX_CALL_CRT_INIT_EMBEDDING_REGISTRATION();							\
	return &Fixture;												\
}			


#define CfixPtrFromRva( base, rva ) ( ( ( PUCHAR ) base ) + rva )

#define CFIX_FIXTURE_EXPORT_PREFIX "__CfixFixturePe"
#define CFIX_FIXTURE_EXPORT_PREFIX_CCH 15

#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED64		CFIX_FIXTURE_EXPORT_PREFIX
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH64	( CFIX_FIXTURE_EXPORT_PREFIX_CCH )
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED32		"_" CFIX_FIXTURE_EXPORT_PREFIX
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH32	( CFIX_FIXTURE_EXPORT_PREFIX_CCH + 1 )

#if _WIN64
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED		CFIX_FIXTURE_EXPORT_PREFIX_MANGLED64
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH	CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH64
#else
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED		CFIX_FIXTURE_EXPORT_PREFIX_MANGLED32
#define CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH	CFIX_FIXTURE_EXPORT_PREFIX_MANGLED_CCH32
#endif
