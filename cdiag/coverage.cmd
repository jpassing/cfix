@echo off
if "x%VSINSTALLDIR%x" == "xx" (
	echo Run vcvars32 first!
	goto Exit
)

echo on
set "PATH=%PATH%;%VSINSTALLDIR%\Team Tools\Performance Tools"
vsinstr bin\chk\i386\jpdiag.dll -coverage -verbose 
start vsperfmon -coverage -output:jpdiag.coverage
bin\chk\i386\unittest.exe
vsperfcmd -shutdown

:Exit