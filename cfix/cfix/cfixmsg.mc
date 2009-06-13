;// Copyright:
;//		2008, Johannes Passing (passing at users.sourceforge.net)
;//
;// This file is part of cfix.
;//
;// cfix is free software: you can redistribute it and/or modify
;// it under the terms of the GNU General Public License as published by
;// the Free Software Foundation, either version 3 of the License, or
;// (at your option) any later version.
;//
;// cfix is distributed in the hope that it will be useful,
;// but WITHOUT ANY WARRANTY; without even the implied warranty of
;// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;// GNU General Public License for more details.
;// 
;// You should have received a copy of the GNU General Public License
;// along with cfix.  If not, see <http://www.gnu.org/licenses/>.
;//
;//--------------------------------------------------------------------
;// Definitions
;//--------------------------------------------------------------------

MessageIdTypedef=HRESULT

SeverityNames=(
  Success=0x0
  Informational=0x1
  Warning=0x2
  Error=0x3
)

FacilityNames=(
  Interface=4
)

LanguageNames=(English=0x409:MSG00409)


;//--------------------------------------------------------------------
MessageId		= 0x8000
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_MISBEHAVIOUD_GETTC_ROUTINE
Language		= English
The routine exported by the testmodule did not provide the expected data
.

MessageId		= 0x8001
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_UNSUPPORTED_VERSION
Language		= English
The API version used by the testmodule is not supported by this release
.

MessageId		= 0x8002
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_DUP_SETUP_ROUTINE
Language		= English
The fixture defines more than one setup routine
.

MessageId		= 0x8003
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_DUP_TEARDOWN_ROUTINE
Language		= English
The fixture defines more than one teardown routine
.

MessageId		= 0x8004
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_UNKNOWN_ENTRY_TYPE
Language		= English
The test fixture definition exportted by the testmodule contains
unrecognized entry types.
.

MessageId		= 0x8005
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_FIXTURE_NAME_TOO_LONG
Language		= English
The fixture name is too long
.







MessageId		= 0x8008
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_TESTRUN_ABORTED
Language		= English
The testrun has been aborted
.

MessageId		= 0x8009
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_UNKNOWN_THREAD
Language		= English
A call has been performed on an unknown thread. Register the thread
appropriately before using any of the framework's APIs in a testcase
.

MessageId		= 0x800a
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_THREAD_ABORTED
Language		= English
The thread has been aborted
.

MessageId		= 0x800b
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_SETUP_ROUTINE_FAILED
Language		= English
The setup routine has failed.
.

MessageId		= 0x800c
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_TEARDOWN_ROUTINE_FAILED
Language		= English
The teardown routine has failed.
.

MessageId		= 0x800d
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIX_E_STACKTRACE_CAPTURE_FAILED
Language		= English
Capturing the stack trace failed.
.
