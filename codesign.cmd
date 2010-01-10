@echo off

if '%1%' EQU '' goto usage
if '%2%' EQU '' goto usage

if '%CODESIGNINGCERT%' EQU '' goto missingcertpath

set DESCRIPTION=%1%
set FILENAME=%2%

echo Description: %DESCRIPTION%
echo File: %FILENAME%

set /P PASSWORD=Enter Password for code signing certificate:

:sign
echo Signing %FILENAME%...
%SDKBASE%\bin\signtool sign /f %CODESIGNINGCERT% /p %PASSWORD% /t http://timestamp.comodoca.com/authenticode /d %DESCRIPTION% %FILENAME%

shift
set FILENAME=%2%
if '%FILENAME%' NEQ '' goto sign

goto exit

:usage
echo.
echo Usage:
echo   %0 description files...
echo.
goto exit

:missingcertpath
echo.
echo CODESIGNINGCERT not set - must hold the path to the PFX file.
echo.
:exit