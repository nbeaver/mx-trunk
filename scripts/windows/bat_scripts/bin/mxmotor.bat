@echo off

call c:\opt\mx\etc\version

%MXWDIR%\bin\mxmotor -F %MXDIR%/etc/mxmotor.dat -s "%HOMEDRIVE%%HOMEPATH%\scan.dat" %1 %2 %3 %4 %5 %6 %7 %8 %9

