#pragma once

/*----------------------------------------------------------------------
 * JP Diagnostics Library 
 *
 * Purpose:
 *		Main header file for the library.
 *
 * Copyright:
 *		2007, 2008 2008, Johannes Passing (passing at users.sourceforge.net)
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

#include <windows.h>
#include <jpdiagmsg.h>

#ifdef _WIN64
#define JPDIAGCALLTYPE
#else
#define JPDIAGCALLTYPE __stdcall
#endif

#if !defined(JPDIAGAPI)
#define JPDIAGAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SETTINGGROUPNAME_CCH 32
#define MAX_SETTINGNAME_CCH 32

typedef PVOID JPDIAG_SESSION_HANDLE;

typedef struct _JPDIAG_MODULE_VERSION
{
	WORD Major;
	WORD Minor;
	WORD Revision;
	WORD Build;
} JPDIAG_MODULE_VERSION, *PJPDIAG_MODULE_VERSION;

JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagGetModuleVersion(
	__in PCWSTR ModulePath,
	__out PJPDIAG_MODULE_VERSION Version 
	);

/*----------------------------------------------------------------------
 *
 * Configuration store
 *
 */

typedef enum _JPDIAG_CONFIGURATION_SCOPES
{
	/*++
		Use global scope.
	--*/
	JpdiagGlobalScope		= 1,

	/*++
		Use user scope.
	--*/
	JpdiagUserScope			= 2,

	/*++
		Use effective scope, i.e. use user scope if available, else
		use global scope.

		Only valid for read operations.
	--*/
	JpdiagEffectiveScope	= 3
};

/*++
	Routine Description:
		Callback routine for being notified about a change in
		the underlying configuration store.

	Parameters:
		Context		Caller-supplied context
--*/
typedef VOID ( CALLBACK * JPDIAG_CONFIGSTORE_UPDATE_CALLBACK ) (
	__in_opt PVOID Context
	);

/*++
	Structure Description:
		Configuration store object. Can be used for storing
		and retrieving settings. Settings are scoped within 
		a group.

--*/
typedef struct _JPDIAG_CONFIGURATION_STORE
{
	DWORD Size;

	/*++
		Routine Description:
			Read a DWORD.

		Parameters:
			This		Pointer to self
			Name		Name of setting to read
			GroupName	Setting group
			Scope		Scope, see _JPDIAG_CONFIGURATION_SCOPES
			Value		Result

		Returns:
			S_OK on success
			JPDIAG_E_SETTING_NOT_FOUND if setting not found
			JPDIAG_E_SETTING_MISMATCH if datatype mismatch
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *ReadDwordSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope,
		__out DWORD *Value
		);

	/*++
		Routine Description:
			Write a DWORD. If not existing yet, the setting is created.

		Parameters:
			This		Pointer to self
			GroupName	Setting group
			Name		Name of setting to read
			Scope		Scope, see _JPDIAG_CONFIGURATION_SCOPES
			Value		Value to write

		Returns:
			S_OK on success
			E_ACCESSDENIED
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *WriteDwordSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope,
		__in DWORD Value
		);

	/*++
		Routine Description:
			Read a string.

		Parameters:
			This				Pointer to self
			GroupName			Setting group
			Name				Name of setting to read
			Scope				Scope, see _JPDIAG_CONFIGURATION_SCOPES
			BufferSizeInChars	Size of buffer in chars
			StringBuffer		Result, always null-terminated
			ActualSize			Actual size read (optional)

		Returns:
			S_OK on success
			JPDIAG_E_SETTING_NOT_FOUND if setting not found
			JPDIAG_E_BUFFER_TOO_SMALL if supplied buffer too small - 
				actual size (incl. null termination) is returned via 
				ActualSize parameter
			JPDIAG_E_SETTING_MISMATCH if datatype mismatch
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *ReadStringSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope,
		__in SIZE_T BufferSizeInChars,
		__out_ecount( BufferSize ) PWSTR StringBuffer,
		__out_opt DWORD *ActualSize
		);

	/*++
		Routine Description:
			Read a multistring.

		Parameters:
			This				Pointer to self
			GroupName			Setting group
			Name				Name of setting to read
			Scope				Scope, see _JPDIAG_CONFIGURATION_SCOPES
			BufferSizeInChars	Size of buffer in chars
			StringBuffer		Result, sequence of null-terminated strings,
								followed by another \0:
									<string1>\0<string2>\0\0
			ActualSize			Actual size read (optional)

		Returns:
			S_OK on success
			JPDIAG_E_SETTING_NOT_FOUND if setting not found
			JPDIAG_E_BUFFER_TOO_SMALL if supplied buffer too small - 
				actual size (incl. null termination) is returned via 
				ActualSize parameter
			JPDIAG_E_SETTING_MISMATCH if datatype mismatch
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *ReadMultiStringSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope,
		__in SIZE_T BufferSizeInChars,
		__out_ecount( BufferSize ) PWSTR StringBuffer,
		__out_opt DWORD *ActualSize
		);

	/*++
		Routine Description:
			Write a string If not existing yet, the setting is created.

		Parameters:
			This		Pointer to self
			GroupName	Setting group
			Name		Name of setting to read
			Scope		Scope, see _JPDIAG_CONFIGURATION_SCOPES
			Value		Value to write. The string must be a sequence 
						of null-terminated strings, followed by another \0:
							<string1>\0<string2>\0\0
		
		Returns:
			S_OK on success
			E_ACCESSDENIED
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *WriteMultiStringSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope,
		__in PCWSTR Value
		);

	/*++
		Routine Description:
			Write a multistring If not existing yet, the setting is created.

		Parameters:
			This		Pointer to self
			GroupName	Setting group
			Name		Name of setting to read
			Scope		Scope, see _JPDIAG_CONFIGURATION_SCOPES
			Value		Value to write
		
		Returns:
			S_OK on success
			E_ACCESSDENIED
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *WriteStringSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope,
		__in PCWSTR Value
		);

	/*++
		Routine Description:
			Delete a setting.

		Parameters:
			This		Pointer to self
			GroupName	Setting group
			Name		Name of setting to delete
			Scope		Scope, see _JPDIAG_CONFIGURATION_SCOPES
			
		Returns:
			S_OK on success
			E_ACCESSDENIED
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *DeleteSetting )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in PCWSTR GroupName,
		__in PCWSTR Name,
		__in DWORD Scope
		);

	/*++
		Routine Description:
			Register an update callback routine for being notified about
			changes in the underlying configuration store. At most one
			routine can be registered.

			The store must have been created with access mode
			JPDIAG_CFGS_ACCESS_READ.

		Parameters:
			This			Pointer to self
			UpdateCallback	Callback routine to call whenever a change
							has occured on the underlying configuration
							store.
							If NULL is passed or a callback was previously
							registered, the previous callback is 
							unregistered.
							Note that the callback may be performed on
							a different thread.
			UpdateCallbackContext	
							Context passed to UpdateCallback. The pointer
							must remain valid until unregistration.
				
		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *RegisterUpdateCallback )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This,
		__in_opt JPDIAG_CONFIGSTORE_UPDATE_CALLBACK UpdateCallback,
		__in_opt PVOID UpdateCallbackContext
		);

	/*++
		Routine Description:
			Release all resources and delete this object from memory

		Parameters:
			This				Pointer to self

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *Delete )(
		__in struct _JPDIAG_CONFIGURATION_STORE *This
		);
} JPDIAG_CONFIGURATION_STORE, *PJPDIAG_CONFIGURATION_STORE;


#define JPDIAG_CFGS_ACCESS_READ_USER		0x1
#define JPDIAG_CFGS_ACCESS_WRITE_USER		0x2
#define JPDIAG_CFGS_ACCESS_READ_MACHINE		0x4
#define JPDIAG_CFGS_ACCESS_WRITE_MACHINE	0x8
#define JPDIAG_CFGS_ACCESS_ALL				0xF

/*++
	Routine Description:
		Return the default implementation of the configuration
		store, which stores the settings in the Windows Registry. 
		The returned object has to be released by calling
		its Delete method.

	Parameters:
		BaseKeyName		Base key path in which settings should be
						stored (as values). Both HKLM and HKCU will
						be used, depending on the flags to the
						ReadXxx and WriteXxx methods.
		AccessMode		Combination of JPDIAG_CFGS_ACCESS_* indicating
						the desired access to the store
		Store			Result. The methods of the object are threadsafe.
						Call Store->Delete to free the object.
						Note that the resultant struct may be larger
						than JPDIAG_CONFIGURATION_STORE.
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateRegistryStore(
	__in PCWSTR BaseKeyName,
	__in DWORD AccessMode,
	__out PJPDIAG_CONFIGURATION_STORE *Store
	);





/*----------------------------------------------------------------------
 *
 * Message resolver
 *
 */
#define JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH	1

#define JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS	1
#define JPDIAG_MSGRES_NO_SYSTEM					2
#define JPDIAG_MSGRES_FALLBACK_TO_DEFAULT		4

typedef struct _JPDIAG_MESSAGE_RESOLVER
{
	DWORD Size;

	/*++
		Routine Description:
			Register an additional message DLL. The DLL is assumed to have
			a MESSAGETABLE resource. The DLL will be registered at the
			end of the list of registered DLLs. If a message has to be 
			resolved, the list is walked head-to-tail and each DLL is
			queried for a message. If the first message is found, the
			search is completed.
			That means that any messages in the DLL that overlap with
			messages in DLLs previously registered will not be used.

		Parameters:
			This			Pointer to self.
			Name			Name of the DLL. If the flag.
							JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH is used,
							a path must be specified, otherwise the DLL
							is located using the ordinary DLL search path.
							Note that the load count of the DLL will be
							incremented.
			Flags			0 or JPDIAG_MSGRES_REGISTER_EXPLICIT_PATH
			Priority		Currently ignored, set to 0.

		Returns:
			S_OK on success
			JPDIAG_E_ALREADY_REGISTERED if already registered
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *RegisterMessageDll )(
		__in struct _JPDIAG_MESSAGE_RESOLVER *This,
		__in PCWSTR Name,
		__in DWORD Flags,
		__reserved DWORD Priority
		);

	/*++
		Routine Description:
			Unegister a prevously registered message DLL. The DLL will
			no longer be used for resoling messages. The load count
			of the DLL will be decremented, possibly leading to 
			an unload of the DLL.

		Parameters:
			This			Pointer to self.
			Name			Name of the DLL, even if the DLL has been
							loaded using a path.

		Returns:
			S_OK on success
			JPDIAG_E_DLL_NOT_REGISTERED if DLL not registered
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *UnregisterMessageDll )(
		__in struct _JPDIAG_MESSAGE_RESOLVER *This,
		__in PCWSTR Name
		);

	/*++
		Routine Description:
			Resolve a message by id. The list of registered message
			DLLs (including the system messages unless
			JPDIAG_MSGRES_NO_SYSTEM is used) is walked and each DLL is
			queried for a message in its message table for the given ID.
			If no DLL contains an appropriate message table entry and

			JPDIAG_MSGRES_FALLBACK_TO_DEFAULT is specified, a default
			string containing the message if will be returned.

			If no DLL contains an appropriate message table entry and
			JPDIAG_MSGRES_FALLBACK_TO_DEFAULT is not specified, 
			an error is returned.

			Unless JPDIAG_MSGRES_RESOLVE_IGNORE_INSERTS is specified,
			insertion strings will be applied. If InsertionStrings is
			non-NULL, the array MUST contain an entry for EACH inserion
			string embedded in the message. Note that only strings are
			supported. Refer to FormatMessage for further details.

		Parameters:
			This				Pointer to self.
			MessageId			Message id, HRESULT, NTSTATUS or Win32
								error code
			Flags				0 or a combination of 
								JPDIAG_MSGRES_RESOLVE_*, see above.
			InsertionStrings	Array of insertion strings. May be NULL.
			BufferSizeInChars	Buffer size, in chars. Must include
								space for the null terminator.
			Buffer				Result.

		Returns:
			S_OK on success
			JPDIAG_E_UNKNOWN_MESSAGE if the message could not be resolved
				and JPDIAG_MSGRES_FALLBACK_TO_DEFAULT was not specified.
			JPDIAG_E_BUFFER_TOO_SMALL if the buffer is too small.
			(any HRESULT) for unexpected errors
	--*/
	HRESULT ( JPDIAGCALLTYPE *ResolveMessage )(
		__in struct _JPDIAG_MESSAGE_RESOLVER *This,
		__in DWORD MessageId,
		__in DWORD Flags,
		__in_opt PCTSTR* InsertionStrings,
		__in SIZE_T BufferSizeInChars,
		__out_ecount(BufferSizeInChars) PWSTR Buffer
		);

	/*++
		Routine Description:
			Increment reference counter.

		Parameters:
			This				Pointer to self

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	VOID ( JPDIAGCALLTYPE *Reference )(
		__in struct _JPDIAG_MESSAGE_RESOLVER *This
		);	
	
	/*++
		Routine Description:
			Decrement reference counter.

		Parameters:
			This				Pointer to self

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	VOID ( JPDIAGCALLTYPE *Dereference )(
		__in struct _JPDIAG_MESSAGE_RESOLVER *This
		);

} JPDIAG_MESSAGE_RESOLVER, *PJPDIAG_MESSAGE_RESOLVER;

/*++
	Routine Description:
		Creates the default message resolver object. The implementation
		is threadsafe.
		The returned object has a reference count of 1.

	Parameters:
		Resolver		Resolver object. 
						Note that the resultant struct may be larger
						than JPDIAG_MESSAGE_RESOLVER.

	Returns:
		S_OK on success
		(any HRESULT) for unexpected errors
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateMessageResolver(
	__out PJPDIAG_MESSAGE_RESOLVER *Resolver
	);


/*----------------------------------------------------------------------
 *
 * Event packet
 *
 */
typedef enum _JPDIAG_EVENT_TYPE
{
	JpdiagLogEvent			= 0,
	JpdiagAssertionEvent	= 1,
	JpdiagTraceEvent		= 2,
	JpdiagTracePenterEvent	= 3,
	JpdiagTracePexitEvent	= 4,

	//
	// Custom event. CustomEvent member of EVENT_PACKET shoud be set.
	//
	JpdiagCustomEvent		= 5,
	JpdiagMaxEvent			= 5
} JPDIAG_EVENT_TYPE;

typedef enum _JTF_SEVERITY_LEVEL
{
	JpdiagTraceSeverity			= 0,
	JpdiagDebugSeverity			= 1,
	JpdiagInfoSeverity 			= 2,
	JpdiagWarningSeverity		= 3,
	JpdiagErrorSeverity			= 4,
	JpdiagFatalSeverity			= 5,
	JpdiagMaxSeverity			= 5
} JPDIAG_SEVERITY_LEVEL;

#define JPDIAG_EVPKT_SYSTEM_ALLOCATED	1

#define JPDIAG_EVENT_PACKET_MAX_INSERTION_STRINGS	16

/*++
	Structure Desciption:
		Defines debugging information. The 
		structure is of varying length and follows the following
		layout:

		  +----------------------------+
		  |     JPDIAG_DEBUG_INFO      |
		  +----------------------------+ <-- Size
		  |           offset           |
		  |    (self-relative only)    |
		  +----------------------------+ <-- TotalSize

		The structure is thus always self-relative.

--*/
typedef struct _JPDIAG_DEBUG_INFO
{
	//
	// Size of structure in bytes - set to sizeof( JPDIAG_DEBUG_INFO )
	//
	USHORT Size;

	//
	// Size of structure including data appended after end of structure
	// (i.e. data reffered to by the offset-fields)
	//
	USHORT TotalSize;

	//
	// Module basename, e.g. "foo" if event is generated by foo.dll.
	//
	DWORD ModuleOffset;

	//
	// Function name.
	//
	DWORD FunctionNameOffset;

	DWORD SourceFileOffset;
	ULONG SourceLine;
} JPDIAG_DEBUG_INFO, *PJPDIAG_DEBUG_INFO;

typedef enum _JPDIAG_PROCESSOR_MODE
{	
	JpdiagKernelMode	= 0,
	JpdiagUserMode		= 1,
	JpdiagMaxMode		= 1
} JPDIAG_PROCESSOR_MODE;

/*++
	Structure Desciption:
		Defines a packet describing a log/assert/... event. The 
		structure is of varying length and follows the following
		layout:

		  +----------------------------+
		  |    JPDIAG_EVENT_PACKET     |
		  +----------------------------+ <-- Size
		  |       Custom fields        |
		  | (*must* be self-relative)  |
		  |                            | 
		  |    Data referred to via    |
		  |           offset           |
		  |    (self-relative only)    |
		  +----------------------------+ <-- TotalSize
		
		The structure is thus always self-relative.

--*/
typedef struct _JPDIAG_EVENT_PACKET
{
	//
	// Size of structure in bytes - set to sizeof( JPDIAG_EVENT_PACKET )
	//
	USHORT Size;
	
	//
	// Size of structure in bytes including custom fields 
	// and all data refferred to via offsets.
	//
	USHORT TotalSize;

	//
	// Type of event - see JPDIAG_EVENT_TYPE.
	//
	JPDIAG_EVENT_TYPE Type;

	//
	// Event subtype if Type == JpdiagCustomEvent.
	//
	DWORD SubType;

	USHORT Flags;

	//
	// Severity - see JPDIAG_SEVERITY_LEVEL.
	//
	UCHAR Severity; 

	//
	// User/Kernel mode, see JPDIAG_PROCESSOR_MODE.
	//
	UCHAR ProcessorMode;

	//
	// Machine/Process/Thread that generated the event.
	// Use 0 for the current machine.
	//
	DWORD MachineOffset;
	DWORD ProcessId;
	DWORD ThreadId;
	
	//
	// Timstamp (UTC).
	//
	FILETIME Timestamp;

	//
	// Code (Win32 or HRESULT).
	//
	DWORD Code;

	//
	// Message - may be 0 if resolving is deferred.
	//
	DWORD MessageOffset;
	
	//
	// Offset to JPDIAG_DEBUG_INFO or 0.
	//
	DWORD DebugInfoOffset;

	//
	// Offset of custom data blob.
	//
	struct
	{
		DWORD Offset;
		DWORD Length;
	} CustomData;

	//
	// Message insertion strings, i.e. strings for use by
	// the message resolver.
	//
	struct
	{
		DWORD Count;

		//
		// Dynamic-size array - may contain up to 
		// JPDIAG_EVENT_PACKET_MAX_INSERTION_STRINGS
		// elements.
		//
		DWORD Offset[ ANYSIZE_ARRAY ];
	} MessageInsertionStrings;
} JPDIAG_EVENT_PACKET, *PJPDIAG_EVENT_PACKET;

#define JpdiagsIsValidEventPacket( pkt ) \
	( pkt != NULL && \
	  pkt->Size == sizeof( JPDIAG_EVENT_PACKET ) && \
	  pkt->TotalSize >= pkt->Size )





/*----------------------------------------------------------------------
 *
 * Formatter
 *
 */
typedef struct _JPDIAG_FORMATTER
{
	DWORD Size;

	/*++
		Routine Description:
			Create a string representation of the information 
			in the event packet.

		Parameters:
			This		- Pointer to self.
			EventPkt	- Paket to retrieve information from.
			BufferSizeInChars
			Buffer

		Return Value:
			S_OK on success
			JPDIAG_E_BUFFER_TOO_SMALL if buffer to small
	--*/
	HRESULT ( JPDIAGCALLTYPE *Format ) (
		__in struct _JPDIAG_FORMATTER *This,
		__in CONST PJPDIAG_EVENT_PACKET EventPkt,
		__in SIZE_T BufferSizeInChars,
		__out_ecount(BufferSizeInChars) PWSTR Buffer
		);

	/*++
		Routine Description:
			Increment reference counter.

		Parameters:
			This				Pointer to self

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	VOID ( JPDIAGCALLTYPE *Reference ) (
		__in struct _JPDIAG_FORMATTER *This
		);	
	
	/*++
		Routine Description:
			Decrement reference counter.

		Parameters:
			This				Pointer to self

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	VOID ( JPDIAGCALLTYPE *Dereference ) (
		__in struct _JPDIAG_FORMATTER *This
		);

} JPDIAG_FORMATTER, *PJPDIAG_FORMATTER;

/*++
	Routine Description:
		Create the default implementation of a formatter that
		formats according to a format template string.

		The template may look as follows:
			'The %Var1-brown %Fox%% %Foo (%Bar)'
		The template may contain variables, which are prefixed by a
		'%' sign and may contain any alphanumeric characters.
		'%%' is the escape sequence for '%'.

		Given the variable definition and bindings, the buffer is 
		formatted according to the Template. Provided the variables
			Name: Var1	Format: %s
			Name: Var2	Format: %s
			Name: Foo	Format: %x
		and the bindings
			"quick"
			"brown"
			42
		the function yields the string
			'The quick-brown fox% 42 ()'
		Unknown variables (%Bar) evaluate to ''.

		Valid variable names are:
			Type			
			Flags			
			Severity		
			ProcessorMode 
			Machine		
			ProcessId		
			ThreadId		
			Timestamp		
			Code			
			Message		
			Module		
			Function		
			File			
			Line			
	Parameters:
		FormatTemplate	- see discussion above.
		Resolver		- Formatter object if codes should be resolved
						  to messages. Note that resolving only
						  takes place if no message has been provided
						  in the event packet.
		ResolvingFlags  - Flags to pass to resolver. See 
						  JPDIAG_MSGRES_RESOLVE_* flags.
		Formatter		- referenced pointer to formatter object

	Return Value:
		S_OK on success
		(any HRESULT) on failure.
--*/
HRESULT JPDIAGCALLTYPE JpdiagCreateFormatter(
	__in PCWSTR FormatTemplate,
	__in_opt PJPDIAG_MESSAGE_RESOLVER Resolver,
	__in_opt DWORD ResolvingFlags,
	__out PJPDIAG_FORMATTER *Formatter
	);


/*----------------------------------------------------------------------
 *
 * Handler
 *
 */
typedef struct _JPDIAG_HANDLER
{
	/*++
		Must be set to sizeof( JPDIAG_HANDLER ).
	--*/
	DWORD Size;
	
	/*++
		Routine Description:
			Handle a packet - e.g. log it.

			Threadsafe.

		Parameters:
			This				Pointer to self.
			Packet				Packet to log.

		Return Value:
			S_OK on success
			(any HRESULT) on failure.
	--*/
	HRESULT ( JPDIAGCALLTYPE *Handle ) (
		__in struct _JPDIAG_HANDLER *This,
		__in PJPDIAG_EVENT_PACKET Packet
		);

	/*++
		Routine Description:
			Sets the next handler to forward packets to. The previous
			handler must be dereferenced, the new handler must be
			referenced. 

			Threadsafe.

		Parameters:
			This				Pointer to self.
			Handler				Next handler.

		Return Value:
			S_OK on success
			JPDIAG_E_CHAINING_NOT_SUPPORTED if handler does not
				support being chained.
			(any HRESULT) on failure.
	--*/
	HRESULT ( JPDIAGCALLTYPE *SetNextHandler ) (
		__in struct _JPDIAG_HANDLER *This,
		__in struct _JPDIAG_HANDLER *Handler
		);

	/*++
		Routine Description:
			Obtain chained handler if available. 

			Threadsafe.

		Parameters:
			This				Pointer to self.
			Handler				Next handler or NULL if not chained.

		Return Value:
			S_OK on success
			JPDIAG_E_CHAINING_NOT_SUPPORTED if handler does not
				support being chained.
			(any HRESULT) on failure.
	--*/
	HRESULT ( JPDIAGCALLTYPE *GetNextHandler ) (
		__in struct _JPDIAG_HANDLER *This,
		__out_opt struct _JPDIAG_HANDLER **Handler
		);

	/*++
		Routine Description:
			Increment reference counter.

		Parameters:
			This				Pointer to self.

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	VOID ( JPDIAGCALLTYPE *Reference ) (
		__in struct _JPDIAG_HANDLER *This
		);

	/*++
		Routine Description:
			Decrement reference counter.

		Parameters:
			This				Pointer to self.

		Returns:
			S_OK on success
			(any HRESULT) for unexpected errors
	--*/
	VOID ( JPDIAGCALLTYPE *Dereference ) (
		__in struct _JPDIAG_HANDLER *This
		);
} JPDIAG_HANDLER, *PJPDIAG_HANDLER;

typedef VOID ( CALLBACK * JPDIAG_OUTPUT_ROUTINE ) (
	__in PCWSTR Text
	);

/*++
	Routine Description:
		Create a handler that outputs information via the specified
		callback.

		N.B. kernel32!OutputDebugString can be used as callback.

	Parameters:
		Session				Session Handler is to be used in.
		Handler				Handler object.

	Returns:
		S_OK on success
		(any HRESULT) for unexpected errors
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateOutputHandler(
	__in JPDIAG_SESSION_HANDLE Session,
	__in JPDIAG_OUTPUT_ROUTINE OutputRoutine,
	__out PJPDIAG_HANDLER *Handler
	);

typedef enum 
{
	JpdiagEncodingUtf16,
	JpdiagEncodingUtf8
} JPDIAG_TEXTFILE_ENCODING;

/*++
	Routine Description:
		Create a handler that outputs information to a textfile.

	Parameters:
		Session				Session Handler is to be used in.
		FilePath			Output file.
		Handler				Handler object.

	Returns:
		S_OK on success
		(any HRESULT) for unexpected errors
--*/

JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateTextFileHandler(
	__in JPDIAG_SESSION_HANDLE Session,
	__in PCWSTR FilePath,
	__in JPDIAG_TEXTFILE_ENCODING Encoding,
	__out PJPDIAG_HANDLER *Handler
	);

///*----------------------------------------------------------------------
// *
// * Session Configuration
// *
// */
//typedef VOID ( CALLBACK * JPDIAG_SESSION_CONFIGURATION_UPDATE_CALLBACK ) (
//	__in_opt PVOID Context
//	);
//
//typedef struct _JPDIAG_SESSION_CONFIGURATION
//{
//	DWORD Size;
//
//	// Formatter text-outputter-private concept only?
//	//
//	//HRESULT ( JPDIAGCALLTYPE *GetFormatter ) (
//	//	__in struct _JPDIAG_SESSION_CONFIGURATION *This,
//	//	__out PJPDIAG_FORMATTER *Formatter
//	//	);
//
//	HRESULT ( JPDIAGCALLTYPE *GetMessageResolver ) (
//		__in struct _JPDIAG_SESSION_CONFIGURATION *This,
//		__out PJPDIAG_MESSAGE_RESOLVER *Resolver
//		);
//
//	// ?
//	//HRESULT ( JPDIAGCALLTYPE *GetHandlerStackDepth ) (
//	//	__in struct _JPDIAG_SESSION_CONFIGURATION *This,
//	//	__out PULONG Depth 
//	//	);
//
//
//	// root handler for given type?
//
//	HRESULT ( JPDIAGCALLTYPE *GetHandler ) (
//		__in struct _JPDIAG_SESSION_CONFIGURATION *This,
//		__in ULONG Position,
//		__out PJPDIAG_HANDLER *Handler 
//		);
//
//	HRESULT ( JPDIAGCALLTYPE *RegisterUpdateCallback ) (
//		__in struct _JPDIAG_SESSION_CONFIGURATION *This,
//		__in JPDIAG_SESSION_CONFIGURATION_UPDATE_CALLBACK Routine,
//		__in PVOID Context
//		);
//
//	VOID ( JPDIAGCALLTYPE *Reference ) (
//		__in struct _JPDIAG_SESSION_CONFIGURATION *This
//		);
//
//	VOID ( JPDIAGCALLTYPE *Dereference ) (
//		__in struct _JPDIAG_SESSION_CONFIGURATION *This
//		);
//} JPDIAG_SESSION_CONFIGURATION, *PJPDIAG_SESSION_CONFIGURATION;
//
//

/*----------------------------------------------------------------------
 *
 * Handler class
 *
 */

/*++
	Structure Description:
		Opaque, variable-length, self-relative structure holding
		all configuration data required to create
		a handler.
--*/
typedef struct _JPDIAG_HANDLER_CONFIGURATION
{
	//
	// Total size of structure.
	//
	DWORD Size;

} JPDIAG_HANDLER_CONFIGURATION, *PJPDIAG_HANDLER_CONFIGURATION;

typedef struct _JPDIAG_HANDLER_CLASS
{
	DWORD Size;

	HRESULT ( JPDIAGCALLTYPE * StoreConfiguration ) ( 
		__in struct _JPDIAG_HANDLER_CLASS *This,
		__in PJPDIAG_CONFIGURATION_STORE Store,
		__in PCWSTR HandlerName,
		__in PJPDIAG_HANDLER_CONFIGURATION Config
		);

	HRESULT ( JPDIAGCALLTYPE * LoadConfiguration ) ( 
		__in struct _JPDIAG_HANDLER_CLASS *This,
		__in PJPDIAG_CONFIGURATION_STORE Store,
		__in PCWSTR HandlerName,
		__out PJPDIAG_HANDLER_CONFIGURATION *Config
		);

	HRESULT ( JPDIAGCALLTYPE *DeleteConfiguration ) (
		__in struct _JPDIAG_HANDLER_CLASS *This,
		__in struct _JPDIAG_HANDLER_CONFIGURATION *Config
		);

	HRESULT ( JPDIAGCALLTYPE *CreateHandler ) (
		__in struct _JPDIAG_HANDLER_CLASS *This,
		__in struct _JPDIAG_HANDLER_CONFIGURATION *Config,
		__out PJPDIAG_HANDLER *Handler );

	HRESULT ( JPDIAGCALLTYPE *Delete ) (
		__in struct _JPDIAG_HANDLER_CLASS *This
		);
} JPDIAG_HANDLER_CLASS, *PJPDIAG_HANDLER_CLASS;

/*----------------------------------------------------------------------
 *
 * Session
 *
 */

//HRESULT JPDIAGCALLTYPE JpdiagCreateSessionIndirect(
//	__in PJPDIAG_SESSION_CONFIGURATION Config,
//	__out JPDIAG_SESSION_HANDLE *Session
//	);

/*++
	Routine Description:
		Create a new session. Handler must be added after creation.

	Parameters:
		Formatter		Formatter to use. If NULL, the default formatter
						will be used.
		Resolver		Resolver to use. If NULL, the default resolver
						will be used.
		Session			Session handle.
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagCreateSession(
	__in_opt PJPDIAG_FORMATTER Formatter,
	__in_opt PJPDIAG_MESSAGE_RESOLVER Resolver,
	__out JPDIAG_SESSION_HANDLE *Session
	);

/*++
	Routine Description:
		Handle an event.

	Parameters:
		Session			Session handle.
		Packet			Event to handle.

	Return Values:
		S_OK on success.
		S_FALSE if filtered.
		(any other failure HRESULT)
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagHandleEvent(
	__in_opt JPDIAG_SESSION_HANDLE Session,
	__in PJPDIAG_EVENT_PACKET Packet
	);


typedef enum _JPDIAG_SESSION_INFO_CLASS
{
	//
	// Default handler to use if no specific handler applies.
	//  Query/Set:
	//		EventType is ignored.
	//		Value is set to a (referenced) PJPDIAG_HANDLER instance.
	//
	JpdiagSessionDefaultHandler	= 0,
	
	//
	// Handler to use for specific EventType
	// Query/Set:
	//  	EventType must be provided.
	//  	Value is set to a (referenced) PJPDIAG_HANDLER instance.
	//
	JpdiagSessionHandler		= 1,

	//
	// Filter to use for specific EventType
	//	Query/Set:
	//  	EventType must be provided.
	//		Value is of type PDWORD.
	//
	JpdiagSessionSeverityFilter = 2,

	//
	// Resolver to use.
	//  Query/Set:
	//		EventType is ignored.
	//		Value is of type PJPDIAG_MESSAGE_RESOLVER.
	//
	JpdiagSessionResolver		= 3,

	//
	// Formatter to use.
	//  Query/Set:
	//		EventType is ignored.
	//		Value is of type PJPDIAG_FORMATTER.
	//
	JpdiagSessionFormatter		= 4,
	JpdiagSessionMaxClass		= 4
} JPDIAG_SESSION_INFO_CLASS;

/*++
	Routine Description:
		Obtain information from a session, see classes for details.

	Parameters:
		Session			Session handle.
		Class			Defines which attribute to set.
		EventType		EventType, if required.
		Value			Value.

	Return Values:
		S_OK on success.
		(any other failure HRESULT)
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagSetInformationSession(
	__in JPDIAG_SESSION_HANDLE Session,
	__in JPDIAG_SESSION_INFO_CLASS Class,
	__in DWORD EventType,
	__in PVOID Value
	);

/*++
	Routine Description:
		Obtain information from a session, see classes for details.

	Parameters:
		Session			Session handle.
		Class			Defines which attribute to query.
		EventType		EventType, if required.
		Value			Return value.

	Return Values:
		S_OK on success.
		S_FALSE if setting not available.
		(any other failure HRESULT)
--*/
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagQueryInformationSession(
	__in_opt JPDIAG_SESSION_HANDLE Session,
	__in JPDIAG_SESSION_INFO_CLASS Class,
	__in DWORD EventType,
	__out PVOID *Value
	);
		
JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagReferenceSession(
	__in JPDIAG_SESSION_HANDLE Session
	);

JPDIAGAPI HRESULT JPDIAGCALLTYPE JpdiagDereferenceSession(
	__in JPDIAG_SESSION_HANDLE Session
	);

//// non-addrefed ptr -> valid only on thread, need to addref else
//JPDIAG_SESSION_HANDLE JPDIAGCALLTYPE JpdiagGetDefaultSession();
//
//// release old session, addref new
//// also release in dllmain on thread-detach
//VOID JPDIAGCALLTYPE JpdiagSetDefaultSession( 
//	__in JPDIAG_SESSION_HANDLE Session 
//	);

#ifdef __cplusplus
} // extern "C"
#endif