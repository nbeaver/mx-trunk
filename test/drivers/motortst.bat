@echo off

set path=..\..\libMx;%path%

..\..\motor\mxmotor -F motortst.dat -s scantst.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

