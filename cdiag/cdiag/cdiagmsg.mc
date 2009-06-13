;// Copyright:
;//		2007-2009 Johannes Passing (passing at users.sourceforge.net)
;//
;// This file is part of cfix.
;//
;// cfix is free software: you can redistribute it and/or modify
;// it under the terms of the GNU Lesser General Public License as published by
;// the Free Software Foundation, either version 3 of the License, or
;// (at your option) any later version.
;//
;// cfix is distributed in the hope that it will be useful,
;// but WITHOUT ANY WARRANTY; without even the implied warranty of
;// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;// GNU Lesser General Public License for more details.
;// 
;// You should have received a copy of the GNU Lesser General Public License
;// along with cfix.  If not, see <http://www.gnu.org/licenses/>.
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
MessageId		= 0x8100
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_SETTING_NOT_FOUND
Language		= English
The specified setting could not be found
.

MessageId		= 0x8101
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_BUFFER_TOO_SMALL
Language		= English
The specified buffer is too small
.

MessageId		= 0x8102
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_SETTING_MISMATCH
Language		= English
The setting refers to data not matching the requested data type
.

MessageId		= 0x8103
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_DLL_NOT_REGISTERED
Language		= English
DLL has not been registered to the message resolver
.

MessageId		= 0x8104
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_UNKNOWN_MESSAGE
Language		= English
Unknown message code
.

MessageId		= 0x8105
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_ALREADY_REGISTERED
Language		= English
DLL has already been registered to the message resolver
.

MessageId		= 0x8106
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_CHAINING_NOT_SUPPORTED
Language		= English
This handler does not support hander chaining
.

MessageId		= 0x8107
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_NO_VERSION_INFO
Language		= English
The module does not contain any version information
.

MessageId		= 0x81ff
Severity		= Warning
Facility		= Interface
SymbolicName	= CDIAG_E_TEST
Language		= English
Test=%1,%2
.
