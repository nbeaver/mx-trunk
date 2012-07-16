@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mdaqmx_base.dat -s sdaqmx_base.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

