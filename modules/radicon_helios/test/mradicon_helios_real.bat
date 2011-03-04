@echo off

call d:/opt/mx/etc/version.bat

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F %MXDIR%/etc/mxserver.dat -s sradicon_helios.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

