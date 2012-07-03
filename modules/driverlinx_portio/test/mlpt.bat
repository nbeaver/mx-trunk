@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mlpt.dat -s slpt.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

