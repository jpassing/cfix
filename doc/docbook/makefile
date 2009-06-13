HH_HOME=c:\Program Files (x86)\HTML Help Workshop\

all: chmdoc webdoc

chmdoc:
	cd chm

	xsltproc ..\stylechm.xsl ..\book.xml	
	-1 "$(HH_HOME)\hhc" htmlhelp.hhp	
	move cfix.chm ..

	cd ..
	
	
webdoc:
	if not exist web\doc md web\doc
	cd web\doc
	xsltproc ..\..\styleweb.xsl ..\..\book.xml

	copy /Y ..\..\chm\*.gif .
	copy /Y ..\..\chm\*.png .
	
	if not exist images md images
	copy /Y ..\*.png .
	copy /Y ..\*.jpg .
	copy /Y ..\..\chm\images images

	cd ..
	

