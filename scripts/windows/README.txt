This directory contains some sample Windows batch files for starting
MX programs on a Windows system.  These files are just examples and
should be customized for your environment.  By default, these are 
intended to be installed in the c:\opt\mx directory, with subdirectories
named c:\opt\mx\bin, c:\opt\mx\etc, c:\opt\mx\run, and c:\opt\mx\sbin.

These batch files are designed to make it easy to switch between installed
versions of MX.  For example, if your c:\opt directory looks like this

    c:\opt\mx\
    c:\opt\mx\mx-1.5.5-2012_03_12
    c:\opt\mx\mx-1.5.5-2012_04_10
    c:\opt\mx\mx-1.5.5-2012_05_16

then you can switch between the different versions of MX by editing the
single file c:\opt\mx\etc\version.bat.  A typical example of version.bat
looks like this

    @echo off

    set MXDIR=c:/opt/mx-1.5.5-2012_05_16

    set MXWDIR=c:\opt\mx-1.5.5-2012_05_16

    set PATH=c:\opt\mx\bin;%MXWDIR%\bin;%MXWDIR%\lib\modules;c:\python27;c:\python27\scripts;%PATH%;c:\cygwin\bin

    set EPICSBASE=c:\opt\epics\base-3.14.12

    set PATH=%PATH%;%EPICSBASE%\bin\win32-x86

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

Bill Lavender (May 16, 2012)

