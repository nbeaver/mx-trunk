@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\motor -F mfastcam_pcclib.dat -s sfastcam_pcclib.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

