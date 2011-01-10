@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mpleora_iport.dat -s spleora_iport.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

