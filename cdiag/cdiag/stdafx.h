#pragma once

/*----------------------------------------------------------------------
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#define WINVER         0x0500
#define _WIN32_WINNT   0x0500
#define _WIN32_WINDOWS 0x0500
#define _WIN32_IE      0x0500
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <strsafe.h>
#include <crtdbg.h>
#include <stdlib.h>

#pragma warning( push )
#pragma warning( disable : 4995 )
#include <shlwapi.h>
#pragma warning( pop ) 
