@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor -F mk8055.dat -s sk8055.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

