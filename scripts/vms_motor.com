$!
$! Example motor script for VMS.
$!
$ motorbin := $user3:[lavender.mxtest.bin]motorbin.exe
$!
$ motorbin "-F" user3:[lavender.mxtest.etc]motor.dat "-s" user3:[lavender]scan.dat
