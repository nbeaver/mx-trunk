@echo off

set path=c:\users\lavender\software\nuvant\cetrdll_lp\ezware~1;%path%

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor -F mnuvant.dat -s snuvant.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

