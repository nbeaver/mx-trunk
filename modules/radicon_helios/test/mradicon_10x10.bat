@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mradicon_10x10.dat -s sradicon_10x10.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

