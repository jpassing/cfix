Set objWMIService = GetObject("winmgmts:{impersonationLevel=impersonate}!\\.\root\cimv2")

Set colOperatingSystems = objWMIService.ExecQuery("Select * from Win32_OperatingSystem")
For Each objOperatingSystem in colOperatingSystems
    Wscript.Echo "NTBUILDNUMBER = " & objOperatingSystem.BuildNumber
	
	if objOperatingSystem.BuildNumber >= 6000 then
		Wscript.Echo "UAC = 1"
	else
		Wscript.Echo "UAC = 0"
	end if
Next

Set colCompSys = objWMIService.ExecQuery("Select * from Win32_ComputerSystem")
for each objCompSys in colCompSys
	if instr(1, objCompSys.SystemType, "x64") then
		Wscript.Echo "ARCH = AMD64"
	else
		Wscript.Echo "ARCH = i386"
	end if
next

wscript.echo "BUILDNUMBER = " & datediff("d", #2000/1/1#, now()) & vbcrlf & vbcrlf

wscript.echo "NOW_DAY = " & day(date())
wscript.echo "NOW_MONTH = " & month(date())
wscript.echo "NOW_YEAR = " & year(date())
