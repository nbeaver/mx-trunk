@echo off

call c:\opt\mx\etc\version

c:\cygwin\bin\tclsh84 %MXDIR%/bin/mxget %1 %2 %3 %4 %5 %6 %7 %8 %9

