if not exist release md release

rem -------------------------------------------------------------------
rem Binary release
rem -------------------------------------------------------------------
if not exist release\cfix-bin md release\cfix-bin
if not exist release\cfix-bin\bin md release\cfix-bin\bin
if not exist release\cfix-bin\bin\i386 md release\cfix-bin\bin\i386
if not exist release\cfix-bin\bin\amd64 md release\cfix-bin\bin\amd64
if not exist release\cfix-bin\lib md release\cfix-bin\lib
if not exist release\cfix-bin\lib\i386 md release\cfix-bin\lib\i386
if not exist release\cfix-bin\lib\amd64 md release\cfix-bin\lib\amd64
if not exist release\cfix-bin\include md release\cfix-bin\include
if not exist release\cfix-bin\doc md release\cfix-bin\doc

copy /Y bin\fre\i386\cfix*.exe release\cfix-bin\bin\i386\
copy /Y bin\fre\i386\cfix.dll release\cfix-bin\bin\i386\
copy /Y bin\fre\i386\cfix*.pdb release\cfix-bin\bin\i386\
copy /Y bin\fre\i386\jpdiag.dll release\cfix-bin\bin\i386\
copy /Y bin\fre\i386\jpdiag.pdb release\cfix-bin\bin\i386\
copy /Y bin\fre\i386\cfix.lib release\cfix-bin\lib\i386\
               
copy /Y bin\fre\amd64\cfix*.exe release\cfix-bin\bin\amd64\
copy /Y bin\fre\amd64\cfix.dll release\cfix-bin\bin\amd64\
copy /Y bin\fre\amd64\cfix*.pdb release\cfix-bin\bin\amd64\
copy /Y bin\fre\amd64\jpdiag.dll release\cfix-bin\bin\amd64\
copy /Y bin\fre\amd64\jpdiag.pdb release\cfix-bin\bin\amd64\
copy /Y bin\fre\amd64\cfix.lib release\cfix-bin\lib\amd64\

copy /Y include\*.h release\cfix-bin\include

copy /Y doc\docbook\chm\cfix.chm release\cfix-bin\doc\
xcopy /S /Y /I  doc\samples\VsSample release\cfix-bin\doc\SampleProject

copy /Y COPYING release\cfix-bin\

rem -------------------------------------------------------------------
rem Source release
rem -------------------------------------------------------------------
if not exist release\cfix-src md release\cfix-src
if not exist release\cfix-src\cfix md release\cfix-src\cfix
if not exist release\cfix-src\cfix\cfix md release\cfix-src\cfix\cfix
if not exist release\cfix-src\cfix\jpdiag md release\cfix-src\cfix\jpdiag
if not exist release\cfix-bin\doc md release\cfix-src\doc

xcopy /S /Y /I include release\cfix-src\cfix\include
xcopy /S /Y /I cfix release\cfix-src\cfix\cfix
xcopy /S /Y /I jpdiag release\cfix-src\cfix\jpdiag
xcopy /S /Y /I ..\jpht release\cfix-src\jpht

copy /Y DIRS release\cfix-src\cfix\
copy /Y *.cmd release\cfix-src\cfix\

copy /Y doc\docbook\chm\cfix.chm release\cfix-src\doc\
xcopy /S /Y /I  doc\samples\VsSample release\cfix-src\doc\SampleProject

copy /Y COPYING release\cfix-src\

pushd release\cfix-src\jpht
cmd.exe /C clean.cmd
popd
pushd release\cfix-src\cfix
cmd.exe /C clean.cmd
popd
