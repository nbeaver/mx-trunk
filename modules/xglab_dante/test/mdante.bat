@echo off

rem set DANTE_VER=3.2.4.3
rem set DANTE_VER=3.4.0.3

set DANTE_VER=3.4.0.3

echo ""
echo DANTE_VER = %DANTE_VER%

set DANTE_DLL_DIR=c:\opt\DANTE\%DANTE_VER%\Library\Callback-x64\Release

set path=..;%DANTE_DLL_DIR%;%path%

..\..\..\motor\mxmotor -Fmdante.dat -s sdante.dat %1 %2 %3 %4 %5 %6 %7 %8 %9

