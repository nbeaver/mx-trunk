@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mradicon_helios.dat -s sradicon_helios.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

