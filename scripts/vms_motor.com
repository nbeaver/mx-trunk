$!
$! Example motor script for VMS.
$!
$ motor := $user3:[lavender.mxtest.bin]motor.exe
$!
$ motor "-F" user3:[lavender.mxtest.etc]motor.dat "-s" user3:[lavender]scan.dat
