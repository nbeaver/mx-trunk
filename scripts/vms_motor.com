$!
$! Example motor script for VMS.
$!
$ motor := $dua0:[users.lavender.mxtest.bin]motor.exe
$!
$ motor "-F" dua0:[users.lavender.mxtest.etc]motor.dat "-s" dua0:[users.lavender]scan.dat
