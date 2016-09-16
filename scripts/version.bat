@echo off
rem An example Win32 batch file for setting up the environment variables
rem needed to use an installed version of MX.

set MX_VERSION=2.1.1-2016_09_15

set EPICS_VERSION=3.14.12.3

set EPICS_HOST_ARCH=windows-x64

set WINPYDIR=C:\WinPython-64bit-2.7.10.2\python-2.7.10.amd64

set WINPYVER=2.7.10.2

rem The MX 'libtiff' module needs to be able to find 'libtiff.dll'.

set LIBTIFF_DIR=C:\opt\tiff-4.0.6

set PATH=%PATH%;%LIBTIFF_DIR%\libtiff;%LIBTIFF_DIR%\tools

rem =======================================================================

set MXDIR=c:/opt/mx-%MX_VERSION%
set MXWDIR=c:\opt\mx-%MX_VERSION%

set MX_ARCH=win32
set MX_INSTALL_DIR=%MXDIR%

set mx_path_for_python=%WINPYDIR%\Lib\site-packages\PyQt5;%WINPYDIR%\Lib\site-packages-PyQt4;%WINPYDIR%\;%WINPYDIR%\DLLs;%WINPYDIR%\Scripts;%WINPYDIR%\..\tools;%WINPYDIR%\..\tools\mingw32\bin;%WINPYDIR%\..\tools\R\bin\x64;%WINPYDIR%\..\tools\Julia\bin

set PATH=c:\opt\mx\bin;%MXWDIR%\bin;%MXWDIR%\lib\modules;%mx_path_for_python%;%PATH%;c:\cygwin64\bin

set PYTHONPATH=%MXWDIR%\lib\mp;%PYTHONPATH%

set EPICSBASE=c:\opt\epics\base-%EPICS_VERSION%

set PATH=%PATH%;%EPICSBASE%\bin\%EPICS_HOST_ARCH%

set mx_path_for_python=

call c:\opt\mx\etc\mx_msdev_dir

set PATH=%PATH%;C:\Program Files (x86)\Sysinternals

