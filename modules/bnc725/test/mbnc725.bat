@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mbnc725.dat -s sbnc725.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

