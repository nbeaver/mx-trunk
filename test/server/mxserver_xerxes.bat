@echo off

set etcdir=c:/opt/mx/etc

..\..\server\mxserver -p 9727 -f %etcdir%/mxserver_med_xafs.dat -C %etcdir%/mxserver.acl %1 %2 %3 %4 %5 %6 %7 %8 %9

