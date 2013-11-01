cfix
====

cfix is an xUnit testing framework for C/C++, specialized for unmanaged Windows development (32/64 bit). cfix supports development of both user and kernel mode unit tests. 

cifx unit tests are compiled and linked into a DLL. The testrunner application provided by cfix allows selectively running tests of one or more of such test-DLLs. Execution and behaviour in case of failing testcases can be highly customized. Moreover, cfix has been designed to work well in conjunction with the Windows Debuggers (Visual Studio, WinDBG). 

As of cfix 1.3, the framework is source-compatible to WinUnit, i.e. a WinUnit test suite can be recompiled into a full-fledged cfix test suite.

See http://www.cfix-testing.org/ for more details.