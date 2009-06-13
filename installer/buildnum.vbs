wscript.echo "BUILDNUMBER = " & datediff("d", #2000/1/1#, now()) & vbcrlf & vbcrlf
wscript.echo "PRODUCTCODE = " & Left(CreateObject("scriptlet.typelib").guid, 38)
wscript.echo "PACKAGECODE = " & Left(CreateObject("scriptlet.typelib").guid, 38)