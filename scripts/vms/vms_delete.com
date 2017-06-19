$ !
$ ! This script appends a wildcard version number to the end of delete
$ ! commands used by the MX Makefiles.  This is done so that the MX
$ ! Makefiles do not have to explicitly include the VMS version number
$ ! in delete commands.
$ !
$ filename = "''p1';*"
$ ! write sys$output "filename = ''filename'"
$ delete/log 'filename'
$ exit
