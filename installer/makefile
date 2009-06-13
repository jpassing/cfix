#
# 7z a -tzip src.zip cfix-src-1.1.3059 -xr!*.ncb -xr!*.suo
#

WIXBINPATH=D:\prog\wix\wix-2.0.5805.0-binaries

!IF [cscript /nologo buildnum.vbs > __buildnum.inc]
!ENDIF
!INCLUDE __buildnum.inc

VERSION = 1.2.0.$(BUILDNUMBER)

cfix.msi: cfix.wixobj ui.wixobj dialogs.wixlib
	if not exist wixca.dll copy /Y $(WIXBINPATH)\wixca.dll .
	light -b .. -nologo -w0 -loc dialogs\WixUI_en-us.wxl -loc cfix_en-us.wxl dialogs.wixlib cfix.wixobj ui.wixobj -out cfix_$(VERSION).msi

cfix.wixobj: cfix.wxs
	candle -dProductCode=$(PRODUCTCODE) -dPackageCode=$(PACKAGECODE) -dProductVersion=$(VERSION) -nologo cfix.wxs

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