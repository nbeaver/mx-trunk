@echo off

%MXWDIR%\sbin\mxserver -p %MXPORT% -f %MXDIR%/etc/mxserver.dat -C %MXDIR%/etc/mxserver.acl %1 %2 %3 %4 %5 %6 %7 %8 %9

