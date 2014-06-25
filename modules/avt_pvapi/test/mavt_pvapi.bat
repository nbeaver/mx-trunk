@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor.static -i -F mavt_pvapi.dat -s satv_pvapi.dat

