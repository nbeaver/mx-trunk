@echo off

call c:\opt\mx\etc\version

call "c:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\Bin\amd64\vcvars64"

color 07

set path=c:\opt\local\bin;%path%;c:\cygwin\bin;c:\opt\epics\base-3.14.12.2\bin\windows-x64;c:\Program Files (x86)\Sysinternals

