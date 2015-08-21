This directory contains some sample Windows batch files for starting
MX programs on a Windows system.  These files are just examples and
should be customized for your environment.  By default, these are 
intended to be installed in the c:\opt\mx directory, with subdirectories
named c:\opt\mx\bin, c:\opt\mx\etc, c:\opt\mx\run, and c:\opt\mx\sbin.

The Windows batch files are designed to make it easy to switch between
installed versions of MX.  For example, if your c:\opt directory looks
like this

    c:\opt\mx\
    c:\opt\mx\mx-1.5.7-2015_03_12
    c:\opt\mx\mx-1.5.7-2015_04_10
    c:\opt\mx\mx-1.5.7-2015_05_16

then you can switch between the different versions of MX by editing the
single file c:\opt\mx\etc\version.bat.  A typical example of version.bat
looks like this

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

Changing the installed version of MX is then just a simple matter of
changing the definitions of MXDIR and MXWDIR to match.  Note that the
difference between the two variables is that MXDIR uses Unix-style
forward slashes, while MXWDIR uses Windows-style backslashes.  Both
variables need to exist for this setup to work.

The bin, etc, and sbin directories all contain useful scripts that emulate
a Unix-style MX installation as much as is practical or useful.  You will
also note the presence of an empty run directory.  Even though the directory
is empty, you should create a c:\opt\mx\run directory, since that is the
directory that the MX server uses as its current directory while running.

Another thing to note is that the above script makes allowances for using
the Cygwin, Win32 Python, and EPICS packages.  If you do not have some of
these packages, then you can either delete or ignore references to them.

Bill Lavender (August 21, 2015)

