@echo off
if "x%VSINSTALLDIR%x" == "xx" (
	echo Run vcvars32 first!
	goto Exit
)

echo on
set "PATH=%PATH%;%VSINSTALLDIR%\Team Tools\Performance Tools"
vsinstr bin\chk\i386\cfix32.exe -coverage -verbose 
vsinstr bin\chk\i386\cfix.dll -coverage -verbose 
vsinstr bin\chk\i386\testapi.dll -coverage -verbose 
start vsperfmon -coverage -output:cfix.coverage
sleep 2
bin\chk\i386\cfix32.exe -z bin\chk\i386\testapi.dll
vsperfcmd -shutdown

:Exit