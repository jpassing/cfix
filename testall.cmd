@echo off

rem just in case cfix is in the path...
set PATH=

pushd bin\chk\i386
echo i386 checked
cfix32 -f -z test*a*.dll
if ERRORLEVEL 2 (
	echo.
	echo Errors occured [i386 checked]
	echo.
	popd
	goto Exit
)
popd

pushd bin\fre\i386
echo i386 free
cfix32 -f -z test*a*.dll
if ERRORLEVEL 2 (
	echo.
	echo Errors occured [i386 free]
	echo.
	popd
	goto Exit
)
popd

pushd bin\chk\amd64
echo amd64 checked
cfix64 -f -z test*a*.dll
if ERRORLEVEL 2 (
	echo.
	echo Errors occured [amd64 checked]
	echo.
	popd
	goto Exit
)
popd

pushd bin\fre\amd64
echo amd64 free
cfix64 -f -z test*a*.dll
if ERRORLEVEL 2 (
	echo.
	echo Errors occured [amd64 free]
	echo.
	popd
	goto Exit
)
popd

:Exit