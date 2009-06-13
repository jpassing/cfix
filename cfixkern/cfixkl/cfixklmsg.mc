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
MessageId		= 0xa000
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIXKL_E_CFIXKR_NOT_FOUND
Language		= English
The cfix kernel reflector driver (cfixkr) could not be found.
.

MessageId		= 0xa001
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIXKL_E_CFIXKR_START_FAILED
Language		= English
The cfix kernel reflector driver (cfixkr) could not be started.
.

MessageId		= 0xa002
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIXKL_E_CFIXKR_START_DENIED
Language		= English
The cfix kernel reflector driver (cfixkr) could not be started due to lacking privileges.
.

MessageId		= 0xa003
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIXKL_E_UNKNOWN_LOAD_ADDRESS
Language		= English
The driver load address cannot be determined.
.

MessageId		= 0xa004
Severity		= Warning
Facility		= Interface
SymbolicName	= CFIXKL_E_INVALID_REFLECTOR_RESPONSE
Language		= English
Invalid reflector response received.
.
