@echo off

set MX_VERSION=mx-2.1.1-2017_04_14

set MX_PYTHON_VERSION=27

rem =======================================================================

set MXDIR=c:/opt/%MX_VERSION%
set MXWDIR=c:\opt\%MX_VERSION%

set EPICSBASE=c:\opt\epics\base-%EPICS_VERSION%

set PATH=c:\opt\mx\bin;%MXWDIR%\bin;%MXWDIR%\lib\modules;c:\python%MX_PYTHON_VERSION%;c:\python%MX_PYTHON_VERSION%\scripts;%PATH%;c:\cygwin\bin

set PYTHONPATH=%MXWDIR%\lib\mp;%PYTHONPATH%

