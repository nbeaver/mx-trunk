@echo off

set MX_VERSION=mx-2.1.1-2017-08-07

set MX_PYTHON_DIR=c:\WinPython-64bit-2.7.10.2\python-2.7.10.amd64

rem =======================================================================

set MXDIR=c:/opt/%MX_VERSION%
set MXWDIR=c:\opt\%MX_VERSION%

set PATH=c:\opt\local\bin;c:\opt\mx\bin;%MXWDIR%\bin;%MXWDIR%\lib\modules;%MX_PYTHON_DIR%;%MX_PYTHON_DIR%\scripts;%PATH%;c:\cygwin64\bin;c:\opt\tiff-4.0.6\libtiff;c:\Program Files (x86)\Sysinternals

set PYTHONPATH=%MXWDIR%\lib\mp;%PYTHONPATH%

