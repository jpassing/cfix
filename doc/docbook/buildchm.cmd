@echo off

set "HH_HOME=c:\Program Files (x86)\HTML Help Workshop\"

pushd chm
xsltproc ..\stylechm.xsl ..\book.xml
if ERRORLEVEL 1 goto Exit

"%HH_HOME%\hhc" htmlhelp.hhp
if ERRORLEVEL 1 goto Exit

:Exit
popd
