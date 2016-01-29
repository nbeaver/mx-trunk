$!
$! This script is used to create the [.mx.libMx]mx_private_version.h
$! header file on OpenVMS.
$!
$! The script uses relative paths, since I do not want to hard code
$! a location for the MX source code here.  It is intended that this
$! script be run from the [.mx.tools] directory and it will not work
$! correctly if run from any other directory.
$!
$define/user sys$output [-.libMx]mx_private_version.h
$!
$run mx_private_version
