#
# mxsetup.sh - MX example setup script for 'bash' and 'sh'.
#
# This is an example script for setting up the environment variables that
# MX client programs need.  It must be sourced by the current shell using
# the '.' command.  Ideally, it should do everything that an average MX
# installation should need, but you should customize it if necessary.
#

# MX files all live in the $MXDIR directory tree.

if [ "x${MXDIR}" = x ]; then
	MXDIR=/opt/mx
fi

# User applications should go in ${MXDIR}/bin.

if [ "x${PATH}" = x ]; then
	PATH="${MXDIR}/bin"
else
	PATH="${MXDIR}/bin:${PATH}"
fi

# Used by Python to find loadable modules.

if [ "x${PYTHONPATH}" = x ]; then
	PYTHONPATH="${MXDIR}/lib/mp"
else
	PYTHONPATH="${MXDIR}/lib/mp:${PYTHONPATH}"
fi

# Used by Tcl to find loadable packages.

if [ "x${TCLLIBPATH}" = x ]; then
	TCLLIBPATH="${MXDIR}/lib/mxtcl"
else
	TCLLIBPATH="${MXDIR}/lib/mxtcl ${TCLLIBPATH}"
fi

# Setup the appropriate shared library path for this operating system.

osname=`uname`

case $osname in
    Darwin )
	# Shared library path for MacOS X.

	if [ "x${DYLD_LIBRARY_PATH}" = x ]; then
		DYLD_LIBRARY_PATH="${MXDIR}/lib"
	else
		DYLD_LIBRARY_PATH="${MXDIR}/lib:${DYLD_LIBRARY_PATH}"
	fi

	export DYLD_LIBRARY_PATH
	;;
    HP-UX )
	# Shared library path for HP-UX.

	if [ "x${SHLIB_PATH}" = x ]; then
		SHLIB_PATH="${MXDIR}/lib"
	else
		SHLIB_PATH="${MXDIR}/lib:${SHLIB_PATH}"
	fi

	export SHLIB_PATH
	;;
    * )
	# Shared library path for other versions of Unix ( Linux, Solaris, etc.)
	#
	# WARNING: If the startup of your X11 window session uses setuid/setgid
	# binaries, then you may find that LD_LIBRARY_PATH is removed from your
	# environment.  This is done to avoid some security exploits that 
	# depend on a custom LD_LIBRARY_PATH.  If this happens to you, then
	# your best alternative is to add $(MXDIR)/lib to your /etc/ld.so.conf
	# file (or whatever serves that function on your platform) and run
	# /sbin/ldconfig.
	#

	if [ "x${LD_LIBRARY_PATH}" = x ]; then
		LD_LIBRARY_PATH="${MXDIR}/lib:${MXDIR}/lib/modules:${MXDIR}/lib/mp"
	else
		LD_LIBRARY_PATH="${MXDIR}/lib:${MXDIR}/lib/modules:${MXDIR}/lib/mp:${LD_LIBRARY_PATH}"
	fi

	export LD_LIBRARY_PATH
	;;
esac

export MXDIR PATH PYTHONPATH

# If present, run a local setup script.

if [ -f $MXDIR/etc/mxlocal.sh ]; then
	source $MXDIR/etc/mxlocal.sh
fi

