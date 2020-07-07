@echo off

set DANTE_DLL_DIR=c:\opt\DANTE\3.2.4.3\Library\Callback-x64\Release
rem set DANTE_DLL_DIR=c:\opt\DANTE\3.4.0.3\Library\Callback-x64\Release

set path=..;%DANTE_DLL_DIR%;%path%

..\..\..\motor\mxmotor -Fmdante.dat -s sdante.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

