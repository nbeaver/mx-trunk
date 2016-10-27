@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mbnc725_lib.dat -s sbnc725_lib.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

