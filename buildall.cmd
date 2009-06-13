@echo off

if x%SDKBASE%x == xx (
	echo.
	echo SDKBASE not set. Set to base directory of Windows SDK.
	echo Note that the path must not contain any spaces - required by build.exe
	echo.
	goto exit
)

if NOT x%DDKBUILDENV%x == xx (
	echo.
	echo Build environment found. Execute this command in a normal shell, not in a WDK shell.
	echo.
	goto exit
)

rcstamp cfix\Cfix\cfix.rc *.*.*.+
if ERRORLEVEL 1 (
	echo rcstamp failed
	goto exit
)

rcstamp cfix\cfixcmd\runtest.rc *.*.*.+
if ERRORLEVEL 1 (
	echo rcstamp failed
	goto exit
)

rcstamp jpdiag\jpdiag\jpdiag.rc *.*.*.+
if ERRORLEVEL 1 (
	echo rcstamp failed
	goto exit
)

rcstamp cfixkern\Cfixkl\cfixkl.rc *.*.*.+
if ERRORLEVEL 1 (
	echo rcstamp failed
	goto exit
)

rcstamp cfixkern\Cfixkr\cfixkr.rc *.*.*.+
if ERRORLEVEL 1 (
	echo rcstamp failed
	goto exit
)

cmd /C ddkbuild -WLHXP checked . -cZ
cmd /C ddkbuild -WLHXP free . -cZ
cmd /C ddkbuild -WLHNETA64 checked . -cZ
cmd /C ddkbuild -WLHNETA64 free . -cZ

:exit