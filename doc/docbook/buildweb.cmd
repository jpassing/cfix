@echo off

set "HH_HOME=c:\Program Files (x86)\HTML Help Workshop\"

if not exist web\doc md web\doc
pushd web\doc
xsltproc ..\..\styleweb.xsl ..\..\book.xml
if ERRORLEVEL 1 goto Exit

copy /Y ..\..\chm\*.gif .
copy /Y ..\..\chm\*.png .

md images
copy /Y ..\*.png .
copy /Y ..\..\chm\images images
:Exit
popd
