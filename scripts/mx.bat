@echo off

set MXDIR=c:/opt/mx

set MXWDIR=c:\opt\mx

set path=%MXWDIR%\bin;%PATH%

%MXWDIR%\sbin\mxserver -p 9727 -f %MXDIR%/etc/mxserver.dat -C %MXDIR%/etc/mxserver.acl %1 %2 %3 %4 %5 %6 %7 %8 %9

