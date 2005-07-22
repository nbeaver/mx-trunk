@echo off

rem Example setup script for MX development under Windows using Visual C++

doskey

set msdevdir=c:\progra~1\micros~1\common\msdev98
set msvcdir=c:\progra~1\micros~1\vc98

set path=c:\progra~1\micros~1\common\tools;%path%
set path=c:\progra~1\micros~1\common\tools\win95;%path%
set path=c:\progra~1\micros~1\vc98\bin;%path%
set path=c:\progra~1\micros~1\common\msdev98\bin;%path%
set path=c:\util;c:\python22;c:\tcl\bin;c:\perl\bin;c:\cygwin\bin;%path%

set include=c:\progra~1\micros~1\vc98\mfc\include;
set include=c:\progra~1\micros~1\vc98\include;%include%
set include=c:\progra~1\micros~1\vc98\atl\include;%include%

set lib=c:\progra~1\micros~1\vc98\lib;c:\progra~1\micros~1\vc98\mfc\lib;

set home=c:\users\lavender

set dircmd=/o /l

set user=lavender

