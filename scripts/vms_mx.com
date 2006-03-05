$!
$! Example MX server startup script for VMS.
$!
$ mxserver := $user3:[lavender.mxtest.sbin]mxserver.exe
$!
$ mxserver "-p" 9727 "-f" user3:[lavender.mxtest.etc]mxserver.dat "-C" user3:[lavender.mxtest.etc]mxserver.acl
