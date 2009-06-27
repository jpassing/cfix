#
# 7z a -tzip src.zip cfix-src-1.1.3059 -xr!*.ncb -xr!*.suo
#

WIXBINPATH=D:\prog\wix\wix-2.0.5805.0-binaries

!IF [cscript /nologo buildnum.vbs > __buildnum.inc]
!ENDIF
!INCLUDE __buildnum.inc

VERSION = 1.5.0.$(BUILDNUMBER)

#----------------------------------------------------------------------
#
# Build.
#
#----------------------------------------------------------------------

cfix.msi: cfix.wixobj ui.wixobj dialogs.wixlib
	if not exist wixca.dll copy /Y $(WIXBINPATH)\wixca.dll .
	light -b .. -nologo -w0 -loc dialogs\WixUI_en-us.wxl -loc cfix_en-us.wxl dialogs.wixlib cfix.wixobj ui.wixobj -out cfix_$(VERSION).msi

cfix.wixobj: cfix.wxs
	candle -dCFIX_TREE=. -dProductCode=$(PRODUCTCODE) -dPackageCode=$(PACKAGECODE) -dProductVersion=$(VERSION) -nologo cfix.wxs

ui.wixobj: ui.wxi
	candle -nologo ui.wxi
	
dialogs.wixlib:
	cd dialogs
	candle -nologo *.wxs
	lit -out ..\dialogs.wixlib *.wixobj
	cd ..
	
clean:
	del dialogs\*.wixobj
	del *.wixobj
	del *.wixlib
	del cfix.msi
	
	
#----------------------------------------------------------------------
#
# Installer tests on test machines.
#
#----------------------------------------------------------------------
testinstallerlocal: 
	if not exist c:\dev\cfix md c:\dev\cfix
	
	msiexec /i cfix_$(VERSION).msi /qb!
	copy /Y makefile c:\dev\cfix\makefile
	cmd /C "cd /d $(APPDATA)\cfix && $(MAKE) -f c:\dev\cfix\makefile testkernelsample testvssample"
	msiexec /x cfix_$(VERSION).msi /qb!
		
#----------------------------------------------------------------------
#
# Installer tests. Should be run in DDK/WDK environment.
#
#----------------------------------------------------------------------
testkernelsample:
	cd doc\examples\KernelMode
	echo Home: $(CFIX_HOME)
	build -cZ
	cd ..\..\..
	
testvssample:
	cd doc\examples\UserMode
!if ( "$(VS80COMNTOOLS)" != "" )
	start /wait devenv ExampleVs2005.sln /build "Release|Win32"
!elseif ( "$(VS71COMNTOOLS)" != "" )
	start /wait devenv ExampleVs2003.sln /build "Release|Win32"
!else
!error Unrecognized VisualStudio version
!endif
	cd ..\..\..