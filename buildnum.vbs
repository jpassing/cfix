Set objWMIService = GetObject("winmgmts:{impersonationLevel=impersonate}!\\.\root\cimv2")

Set colOperatingSystems = objWMIService.ExecQuery("Select * from Win32_OperatingSystem")
For Each objOperatingSystem in colOperatingSystems
    Wscript.Echo "NTBUILDNUMBER = " & objOperatingSystem.BuildNumber
Next

wscript.echo "BUILDNUMBER = " & datediff("d", #2000/1/1#, now()) & vbcrlf & vbcrlf

Set objProcessor = objWMIService.Get("win32_Processor='CPU0'")
If objProcessor.Architecture = 0 Then
	Wscript.Echo "ARCH = i386"
ElseIf objProcessor.Architecture = 9 Then
    Wscript.Echo "ARCH = AMD64"
End If