@echo off

set path=..\..\..\libMx;%path%

..\..\..\motor\mxmotor.static -i -F matv_vimba.dat -s satv_vimba.dat

