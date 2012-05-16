@echo off

set MXDIR=c:/opt/mx-1.5.5-2012_05_16

set MXWDIR=c:\opt\mx-1.5.5-2012_05_16

set PATH=c:\opt\mx\bin;%MXWDIR%\bin;%MXWDIR%\lib\modules;c:\python27;c:\python27\scripts;%PATH%;c:\cygwin\bin

set EPICSBASE=c:\opt\epics\base-3.14.12.2

set PATH=%PATH%;%EPICSBASE%\bin\win32-x86

set PYTHONPATH=%MXWDIR%\lib\mp;%PYTHONPATH%

