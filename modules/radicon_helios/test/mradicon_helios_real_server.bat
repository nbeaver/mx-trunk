@echo off

call d:/opt/mx/etc/version.bat

set path=..\..\..\libMx;%path%

..\..\..\server\mxserver -c -p 9727 -f %MXDIR%/etc/mxserver.dat -C %MXDIR%/etc/mxserver.acl %1 %2 %3 %4 %5 %6 %7 %8 %9

