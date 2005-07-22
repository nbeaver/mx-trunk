$!
$! Example motor script for VMS.
$!
$ motorbin := $dua0:[users.lavender.mxtest.bin]motorbin.exe
$!
$ motorbin "-F" dua0:[users.lavender.mxtest.etc]motor.dat "-s" dua0:[users.lavender]scan.dat
