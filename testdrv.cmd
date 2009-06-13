if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (set BITNESS=64) else (set BITNESS=32)

sc stop cfixkr
sc stop cfixkr_testklib0
sc stop cfixkr_testklib1
sc stop cfixkr_testklib2
sc stop cfixkr_testklib3
sc stop cfixkr_testklib4
sc stop cfixkr_testklib5
sc stop cfixkr_testklib6
sc stop cfixkr_testklib7

if "%PROCESSOR_ARCHITECTURE%" == "AMD64" (
	cmd.exe /C copydrv64.cmd
	pushd %SystemDrive%\drv\amd64\
	cfix64 -f  testkr.dll
	popd
	
	if ERRORLEVEL 1 ( goto exit )
)

cmd.exe /C copydrv32.cmd
pushd %SystemDrive%\drv\i386\
cfix32 -f  testkr.dll
popd

:exit