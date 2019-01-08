@echo off

set path=..\..\..\libMx;%path%

set path=C:\Program Files\XIA\xManager 1.1;%path%

..\..\..\motor\mxmotor -F mxia_handel.dat -s sxia_handel.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

