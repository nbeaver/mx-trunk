@echo off

rem This script can be used to run the 'motor' program using the MX server's
rem database file.  This is mostly useful for testing MX drivers in a simpler
rem single process environment.

call c:\opt\mx\etc\version

%MXWDIR%\bin\motor -F %MXDIR%/etc/mxserver.dat -s nul %1 %2 %3 %4 %5 %6 %7 %8 %9

