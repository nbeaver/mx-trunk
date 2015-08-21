@echo off

set MX_VERSION=mx-1.5.7-2015_08_21

set MX_PYTHON_VERSION=27

set EPICS_VERSION=3.14.12.3

set EPICS_HOST_ARCH=windows-x64

rem =======================================================================

set MXDIR=c:/opt/%MX_VERSION%
set MXWDIR=c:\opt\%MX_VERSION%

set EPICSBASE=c:\opt\epics\base-%EPICS_VERSION%

set PATH=c:\opt\mx\bin;%MXWDIR%\bin;%MXWDIR%\lib\modules;c:\python%MX_PYTHON_VERSION%;c:\python%MX_PYTHON_VERSION%\scripts;%PATH%;c:\cygwin\bin

set PATH=%PATH%;%EPICSBASE%\bin\%EPICS_HOST_ARCH%

set PYTHONPATH=%MXWDIR%\lib\mp;%PYTHONPATH%

