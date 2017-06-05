@echo off

call c:\opt\mx\etc\version

%MXWDIR%\sbin\mxautosave %1 %2 %3 %4 %5 %6 %7 %8 %9 %MXDIR%/etc/mxautosave.dat %MXDIR%/state/mxsave.1 %MXDIR%/state/mxsave.2

