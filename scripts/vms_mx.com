$!
$! Example MX server startup script for VMS.
$!
$ mxserver := $dua0:[users.lavender.mxtest.sbin]mxserver.exe
$!
$ mxserver "-k" "-p" 9727 "-f" dua0:[users.lavender.mxtest.etc]mxserver.dat "-C" dua0:[users.lavender.mxtest.etc]mxserver.acl
