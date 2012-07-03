@echo off

set path=..\..\..\libMx;c:\Ortec\Umcbi;%path%

..\..\..\motor\motor -F mtrump.dat -s strump.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

