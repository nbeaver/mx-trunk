@echo off

set DANTE_DLL_DIR=c:\Users\mrcat\Desktop\DANTE\Library\Microsoft Visual C++ 2017 x64\2019-06-24 17.07-3.2.4.3-Callback-x64\Release

set path=..;%DANTE_DLL_DIR%;%path%

..\..\..\motor\mxmotor -Fmdante.dat -s sdante.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

