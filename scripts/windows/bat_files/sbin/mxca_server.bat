@echo off

call c:\opt\mx\etc\version

%MXWDIR%\sbin\mxca_server -l %1 %2 %3 %4 %5 %6 %7 %8 %9 %MXDIR%/etc/mxca_server.dat %MXDIR%/etc/mxca_server.acl

