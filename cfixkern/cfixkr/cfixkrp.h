#pragma once

/*----------------------------------------------------------------------
 * Purpose:
 *		Internal header file. 
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

#include <cfixkr.h>
#include <cfixpe.h>

#define CFIXKR_POOL_TAG 'xifC'

/*----------------------------------------------------------------------
 *
 * Utility routines.
 *
 */

/*++
	Routine Description:
		Helper routine calling IoCompleteRequest.
--*/
NTSTATUS CfixkrpCompleteRequest(
	__in PIRP Irp,
	__in NTSTATUS Status,
	__in ULONG_PTR Information,
	__in CCHAR PriorityBoost
	);

PVOID CfixkrpAllocatePagedHashtableMemory(
	__in SIZE_T Size 
	);

PVOID CfixkrpAllocateNonpagedHashtableMemory(
	__in SIZE_T Size 
	);

VOID CfixkrpFreeHashtableMemory(
	__in PVOID Mem
	);


/*----------------------------------------------------------------------
 *
 * Driver Connection object.
 *
 */

struct _CFIXKRP_DRIVER_CONNECTION;
typedef struct _CFIXKRP_DRIVER_CONNECTION 
CFIXKRP_DRIVER_CONNECTION, *PCFIXKRP_DRIVER_CONNECTION;

#define CFIXKRP_REPORT_CHANNEL_SIGNATURE 'nnhC'

/*++
	Structure Description:
		Provides the link between caller and test code. All 
		events generated by the test code are funneled through
		the channel. Moreover, the channel specifies the dispositions
		to use in the case of test failures.

		Note that this structure is used thread-locally only.
		Therefore, no synchronization is required.
--*/
typedef struct _CFIXKRP_REPORT_CHANNEL
{
	ULONG Signature;

	/*++
		Routine Description:
			Called to report any type of test event - failed assertions,
			exceptions etc.
	--*/
	//CFIX_REPORT_DISPOSITION ( * Report )(
	//	__in struct _CFIXKRP_REPORT_CHANNEL *This,
	//	__in PCFIX_TESTCASE_EXECUTION_EVENT Event 
	//	);

	struct
	{
		//
		// [in] Disposition tto used when a failed asserion occurs.
		//
		CFIX_REPORT_DISPOSITION FailedAssertion;
		
		//
		// [in] Disposition tto used when an unhandled exception occurs.
		//
		CFIX_REPORT_DISPOSITION UnhandledException;
	} Dispositions;

	//
	// N.B. The buffer must only be written to using the Report
	// method.
	//
	struct
	{
		//
		// [in] Total size of buffer, in bytes.
		//
		ULONG BufferSize;

		//
		// [in, out] Buffer in which a sequence of CFIXKR_EXECUTION_EVENT
		// (each being of variable length) is written.
		//
		PUCHAR Buffer;

		//
		// [out] Number of CFIXKR_EXECUTION_EVENT strcutures written
		// to buffer.
		//
		ULONG EventCount;

		//
		// [out] Number of bytes in buffer that have been written.
		//
		ULONG BufferLength;

		//
		// [out] TRUE iff more events have been generated than the buffer
		// is able to hold. The remaining events have been dropped.
		//
		BOOLEAN BufferTruncated;
	} EventBuffer;
} CFIXKRP_REPORT_CHANNEL, *PCFIXKRP_REPORT_CHANNEL;


/*++
	Routine Description:
		Create and register a driver connection object (from NPP).

		Only to be called (indirectly) from test driver during registration. 
		As a consequence, the test driver is guaranteed to be locked 
		into memory during execution of this routine.

		Callable at IRQL == PASSIVE_LEVEL.
--*/
NTSTATUS CfixkrpCreateAndRegisterDriverConnection(
	__in ULONGLONG DriverLoadAddress,
	__out PCFIXKRP_DRIVER_CONNECTION *Connection
	);

/*++
	Routine Description:
		Initialite the interface. 

		Callable at IRQL < DISPATCH_LEVEL.
--*/
VOID CfixkrpQuerySinkInterfaceDriverConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__out PCFIXKR_REPORT_SINK_INTERFACE Interface
	);


/*++
	Routine Description:
		Calls a routine of the driver. The routine is expected
		to have a signature compatible to CFIX_PE_TESTCASE_ROUTINE.

		Callable at IRQL == PASSIVE_LEVEL.

	Parameters:
		Channel					Channel to receive any test events.
		RoutineRanToCompletion	Indicates whether the routine ran to
								completion or was aborted prematurely 
								due to failed assertions etc.
		AbortRun				Indicates whether (based on the
								decision of the channel) the testrun
								is to be aborted.
	Return Value:
		STATUS_SUCCESS on successful call.
		STATUS_DRIVER_ENTRYPOINT_NOT_FOUND if routine not available.
		STATUS_DRIVER_UNABLE_TO_LOAD if the driver is not available any
			more.

		The return value only indicates whether the call was successful,
		not whether the actual routine completed successfully.
--*/
NTSTATUS CfixkrpCallRoutineDriverConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__in USHORT FixtureKey,
	__in USHORT RoutineKey,
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__out BOOLEAN *RoutineRanToCompletion,
	__out BOOLEAN *AbortRun
	);

/*++
	Routine Description:
		Callable at IRQL < DISPATCH_LEVEL.
--*/
VOID CfixkrpReferenceConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Conn
	);

/*++
	Routine Description:
		Callable at IRQL < DISPATCH_LEVEL.
--*/
VOID CfixkrpDereferenceConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Conn
	);

/*++
	Routine Description:
		Get the channel attached to the currently executing thread.

		This routine is only to be used by the reports stub
		routines.

		Callable at any IRQL.
--*/
PCFIXKRP_REPORT_CHANNEL CfixkrpGetReportChannelCurrentThread(
	__in PCFIXKRP_DRIVER_CONNECTION Connection
	);

/*++
	Routine Description:
		Report information about an uncaught exception thrown by 
		a test routine.

		Only to be called from Driver Connection.

		Callable at any IRQL.
--*/
CFIX_REPORT_DISPOSITION CfixkrpReportUnhandledException(
	__in PCFIXKRP_REPORT_CHANNEL Channel,
	__in PEXCEPTION_POINTERS ExceptionPointers
	);

/*++
	Routine Description:
		Populate the struct with the pointers of the stub routines.
--*/
VOID CfixkrpGetReportSinkStubs(
	__out PCFIXKR_REPORT_SINK_METHODS Stubs
	);

/*----------------------------------------------------------------------
 *
 * Driver Connection Registry.
 *
 */

/*++
	Routine Description:
		Initialize the registry. Must be called at driver load time.

		Callable at IRQL < DISPATCH_LEVEL.
--*/
NTSTATUS CfixkrpInitializeDriverConnectionRegistry();

/*++
	Routine Description:
		Teardown the registry. Must be called at driver unload time.

		Callable at IRQL < DISPATCH_LEVEL.
--*/
VOID CfixkrpTeardownDriverConnectionRegistry();

/*++
	Routine Description:
		Register a connection. May only be called by connection
		object implementation.

		Callable at IRQL < DISPATCH_LEVEL.
--*/
NTSTATUS CfixkrpRegisterDriverConnection(
	__in ULONGLONG LoadAddress,
	__in PCFIXKRP_DRIVER_CONNECTION Connection
	);

/*++
	Routine Description:
		Unregister a connection. May only be called by connection
		object implementation.

		Callable at IRQL < DISPATCH_LEVEL.
--*/
NTSTATUS CfixkrpUnregisterDriverConnection(
	__in ULONGLONG LoadAddress
	);

/*++
	Routine Description:
		Returns a (referenced) connection object corresponding to the
		given load address.
		
		Callable at IRQL < DISPATCH_LEVEL.

	Return Value:
		STATUS_SUCCESS on success 
		STATUS_NOT_FOUND if no such driver connection
--*/
NTSTATUS CfixkrpLookupDriverConnection(
	__in ULONGLONG DriverLoadAddress,
	__out PCFIXKRP_DRIVER_CONNECTION *Connection
	);

/*++
	Routine Description:
		Fill a PCFIXKRIO_TEST_MODULE structure.

		Callable at IRQL == PASSIVE_LEVEL.

	Return Value:
		STATUS_BUFFER_OVERFLOW if MaximumBufferSize too small.
			BufferSize is set to required size.
--*/
NTSTATUS CfixkrpQueryModuleDriverConnection(
	__in PCFIXKRP_DRIVER_CONNECTION Connection,
	__in ULONG MaximumBufferSize,
	__out PUCHAR IoBuffer,
	__out PULONG BufferSize
	);

/*++
	Routine Description:
		Get list of connections, i.e. list of load addresses.

		Callable at IRQL < DISPATCH_LEVEL.

	Return Value:
		STATUS_BUFFER_OVERFLOW if array was too small. The array will
			contain as many addresses as it can hold. ElementsAvailable
			will contain the total amount of connections available.
		STATUS_SUCCESS if array was large enough.
			ElementsAvailable == ElementsWritten.
--*/
NTSTATUS CfixkrpGetDriverConnections(
	__in ULONG LoadAddressesCapacity,
	__out_ecount(LoadAddressesCapacity) ULONGLONG *DriverLoadAddresses,
	__out PULONG ElementsWritten,
	__out PULONG ElementsAvailable 
	);

/*----------------------------------------------------------------------
 *
 * Adapter object.
 *
 */

struct _CFIXKRP_TEST_ADAPTER;
typedef struct _CFIXKRP_TEST_ADAPTER 
CFIXKRP_TEST_ADAPTER, *PCFIXKRP_TEST_ADAPTER;

/*++
	Routine Description:
		Create a test adapter for the given driver. The driver must be
		locked in memory for the duration of the call.

		Callable at IRQL == PASSIVE_LEVEL.
--*/
NTSTATUS CfixkrpCreateTestAdapter(
	__in ULONGLONG DriverLoadAddress,
	__out PCFIXKRP_TEST_ADAPTER *Adapter
	);

/*++
	Routine Description:
		Delete a test adapter.

		Callable at IRQL < DISPATCH_LEVEL.
--*/
VOID CfixkrpDeleteTestAdapter(
	__in PCFIXKRP_TEST_ADAPTER Adapter
	);


/*++
	Routine Description:
		Returns the routine identified by the given keys. 

		Note: Before calling the returned routine, the caller must
		ensure that the driver is locked in memory for the duration of 
		the call.
		
		Callable at IRQL < DISPATCH_LEVEL.

	Return Value:
		STATUS_SUCCESS if routine found.
		STATUS_DRIVER_ENTRYPOINT_NOT_FOUND if routine not available.
--*/
NTSTATUS CfixkrpGetRoutineTestAdapter(
	__in PCFIXKRP_TEST_ADAPTER Adapter,
	__in USHORT FixtureKey,
	__in USHORT RoutineKey,
	__out CFIX_PE_TESTCASE_ROUTINE *Routine
	);

/*++
	Routine Description:
		Fill a PCFIXKRIO_TEST_MODULE structure.

		Only cached information is requested; module need not be locked.

		Callable at IRQL == PASSIVE_LEVEL.

	Return Value:
		STATUS_BUFFER_OVERFLOW if MaximumBufferSize too small.
			BufferSize is set to required size.
--*/
NTSTATUS CfixkrpQueryModuleTestAdapter(
	__in PCFIXKRP_TEST_ADAPTER Adapter,
	__in ULONG MaximumBufferSize,
	__out PUCHAR IoBuffer,
	__out PULONG BufferSize
	);

/*----------------------------------------------------------------------
 *
 * Stack Trace Capture.
 *
 */

#define CFIXKRP_MAX_STACKFRAMES 32

/*++
	Routine Description:
		Capture the stack trace based on the given CONTEXT.

	Parameters:
		Context		Thread context. If NULL, the current context is
					used.
		Event		Event structure to populate.
		MaxFrames	Max number of frames the structure can hold.
--*/
NTSTATUS CfixkrpCaptureStackTrace(
	__in_opt CONST PCONTEXT Context,
	__in PCFIX_STACKTRACE StackTrace,
	__in ULONG MaxFrames 
	);