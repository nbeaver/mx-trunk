$!
$! Example mxmotor script for VMS.
$!
$ mxmotor := $dua0:[users.lavender.mxtest.bin]mxmotor.exe
$!
$ mxmotor "-F" dua0:[users.lavender.mxtest.etc]mxmotor.dat "-s" dua0:[users.lavender]mxscan.dat
