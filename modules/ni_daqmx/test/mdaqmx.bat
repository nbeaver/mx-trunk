@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor -F mdaqmx.dat -s sdaqmx.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

