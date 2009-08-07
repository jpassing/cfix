@echo off
 
rem
rem    This file is part of cfix.
rem
rem    Copyright:
rem        2007-2009 Johannes Passing (passing at users.sourceforge.net)
rem
rem    cfix is free software: you can redistribute it and/or modify
rem    it under the terms of the GNU Lesser General Public License as published by
rem    the Free Software Foundation, either version 3 of the License, or
rem    (at your option) any later version.
rem
rem    cfix is distributed in the hope that it will be useful,
rem    but WITHOUT ANY WARRANTY; without even the implied warranty of
rem    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem    GNU Lesser General Public License for more details.
rem    
rem    You should have received a copy of the GNU Lesser General Public License
rem    along with cfix.  If not, see <http://www.gnu.org/licenses/>.
rem

if "%CFIX_HOME%" == "" goto nohome

if "%1" == ""      goto usage
if /i %1 == i386   goto i386
if /i %1 == x86    goto i386
if /i %1 == amd64  goto amd64
if /i %1 == x64    goto amd64

echo.
echo Unknown architecture "%1%"
echo.
goto usage

:i386
echo Setting environment for cfix (i386).

set INCLUDE=%CFIX_HOME%\include;%INCLUDE%
set LIB=%CFIX_HOME%\lib\i386;%LIB%;
set PATH=%CFIX_HOME%\bin\i386;%PATH%

goto :eof

:amd64
echo Setting environment for cfix (amd64).

set INCLUDE=%CFIX_HOME%\include;%INCLUDE%
set LIB=%CFIX_HOME%\lib\amd64;%LIB%;
set PATH=%CFIX_HOME%\bin\amd64;%PATH%

goto :eof

:nohome
echo Error: Environment variable CFIX_HOME not set.
goto :eof

:usage
echo Usage:
echo   %0 i386 ^| amd64 ^| x86 ^| x64
echo.
echo Example:
echo   %0 x86 - sets environment variables for building 32-bit cfix tests
echo   %0 x64 - sets environment variables for building 64-bit cfix tests