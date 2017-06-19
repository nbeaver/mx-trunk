$ !
$ ! This script implements an extremely limited subset of the functionality
$ ! of the Unix "touch" command.  The script creates the specified file
$ ! if and only if it does not exist.  If the file already exists, the
$ ! script does nothing.
$ !
$ file = f$search("''p1'")
$ if file .eqs. ""
$ then
$    write sys$output "Creating file ''p1'"
$    create 'p1'
$ else
$    write sys$output "File ''p1' already exists."
$ endif
$ exit
