@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor.static -i -F mavt_vimba.dat -s satv_vimba.dat

