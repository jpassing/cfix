// Copyright:
//		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
//
// This file is part of cfix.
//
// cfix is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// cfix is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with cfix.  If not, see <http://www.gnu.org/licenses/>.
//--------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------
//--------------------------------------------------------------------
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: JPDIAG_E_SETTING_NOT_FOUND
//
// MessageText:
//
// The specified setting could not be found
//
#define JPDIAG_E_SETTING_NOT_FOUND       ((HRESULT)0x80048100L)

//
// MessageId: JPDIAG_E_BUFFER_TOO_SMALL
//
// MessageText:
//
// The specified buffer is too small
//
#define JPDIAG_E_BUFFER_TOO_SMALL        ((HRESULT)0x80048101L)

//
// MessageId: JPDIAG_E_SETTING_MISMATCH
//
// MessageText:
//
// The setting refers to data not matching the requested data type
//
#define JPDIAG_E_SETTING_MISMATCH        ((HRESULT)0x80048102L)

//
// MessageId: JPDIAG_E_DLL_NOT_REGISTERED
//
// MessageText:
//
// DLL has not been registered to the message resolver
//
#define JPDIAG_E_DLL_NOT_REGISTERED      ((HRESULT)0x80048103L)

//
// MessageId: JPDIAG_E_UNKNOWN_MESSAGE
//
// MessageText:
//
// Unknown message code
//
#define JPDIAG_E_UNKNOWN_MESSAGE         ((HRESULT)0x80048104L)

//
// MessageId: JPDIAG_E_ALREADY_REGISTERED
//
// MessageText:
//
// DLL has already been registered to the message resolver
//
#define JPDIAG_E_ALREADY_REGISTERED      ((HRESULT)0x80048105L)

//
// MessageId: JPDIAG_E_CHAINING_NOT_SUPPORTED
//
// MessageText:
//
// This handler does not support hander chaining
//
#define JPDIAG_E_CHAINING_NOT_SUPPORTED  ((HRESULT)0x80048106L)

//
// MessageId: JPDIAG_E_NO_VERSION_INFO
//
// MessageText:
//
// The module does not contain any version information
//
#define JPDIAG_E_NO_VERSION_INFO         ((HRESULT)0x80048107L)

