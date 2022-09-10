@echo off

set path=..\..\libMx;%path%

..\..\motor\mxmotor -F mwin32clock.dat -s swin32clock.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

