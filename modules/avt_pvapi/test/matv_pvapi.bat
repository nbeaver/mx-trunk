@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor.static -i -F matv_pvapi.dat -s satv_pvapi.dat

