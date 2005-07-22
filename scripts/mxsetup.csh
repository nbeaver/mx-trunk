# MX example setup script for tcsh.  It must be sourced by the current shell
# using the 'source' command.

setenv MXDIR /opt/mx

setenv MXPORT 9727

set path=( ${MXDIR}/bin $path )

setenv PYTHONPATH "${MXDIR}/lib/mp:$PYTHONPATH"

setenv LD_LIBRARY_PATH "${MXDIR}/lib:$LD_LIBRARY_PATH"

