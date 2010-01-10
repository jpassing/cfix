HH_HOME=c:\Program Files (x86)\HTML Help Workshop\

!IF [cscript /nologo buildnum.vbs > __buildnum.inc]
!ENDIF
!INCLUDE __buildnum.inc

all: chmdoc webdoc

chmdoc:
	cd docbook\chm
	copy ..\stylechm.css .

	xsltproc --novalid --param buildnumber $(BUILDNUMBER) ..\stylechm.xsl ..\book.xml	
	-1 "$(HH_HOME)\hhc" htmlhelp.hhp	
	move cfix.chm ..

	cd ..\..
	
	
webdoc:
	if not exist docbook\web\doc md docbook\web\doc
	cd docbook\web\doc
	copy ..\..\styleweb.css ..
	-3 robocopy ..\..\..\assets ..\assets /MIR /XD .svn /XX /NJH /NJS
	
	xsltproc --novalid --param buildnumber $(BUILDNUMBER) ..\..\styleweb.xsl ..\..\book.xml

	copy /Y ..\..\chm\*.gif .
	copy /Y ..\..\chm\*.png .
	
	if not exist images md images
	copy /Y ..\*.png .
	copy /Y ..\*.jpg .
	copy /Y ..\..\chm\images images

	cd ..\..\..
	

