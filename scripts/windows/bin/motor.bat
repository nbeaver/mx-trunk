@echo off

call c:\opt\mx\etc\version

%MXWDIR%\bin\motor -F %MXDIR%/etc/motor.dat -s "%HOMEDRIVE%%HOMEPATH%\scan.dat" %1 %2 %3 %4 %5 %6 %7 %8 %9

