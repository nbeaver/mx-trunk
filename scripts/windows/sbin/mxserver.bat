@echo off

call c:\opt\mx\etc\version

%MXWDIR%\sbin\mxserver -p 9727 -f %MXDIR%/etc/mxserver.dat -C %MXDIR%/etc/mxserver.acl -c -M 0.01 %1 %2 %3 %4 %5 %6 %7 %8 %9

