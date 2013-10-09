@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor -F mlabjack.dat -s slabjack.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

