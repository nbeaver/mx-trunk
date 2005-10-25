#
# Change the value of the MX_ARCH variable below to match the platform you
# are compiling the MX system for.  The available platforms are:
#
#   bsd           - Compile for FreeBSD 5.4, NetBSD 2.0.2, or OpenBSD 3.3
#   cygwin        - Compile for Cygwin 1.5.x
#   djgpp         - Compile for DOS extender with DJGPP 2.0.3
#   hpux          - Compile for HP 9000/7xx under HP-UX 10.20
#   irix          - Compile for SGI under Irix 6.3 or 6.5
#   irix-gcc      - Compile for SGI under Irix 6.5 using GCC
#   linux         - Compile for i386/m68k/sparc under Linux 2.2, 2.4, or 2.6
#   linux-icc     - Compile for i386 under Linux 2.4 using Intel C++ 7.1
#   macosx        - Compile for PPC Macintosh under MacOS X
#   qnx           - Compile for i386 under QNX Neutrino 6.3
#   rtems         - Compile for i386 under RTEMS 4.6.5
#   solaris       - Compile for Sun Sparc under Solaris 2.6
#   solaris-gcc   - Compile for Sun Sparc under Solaris 8 using GCC
#   vms           - Compile for OpenVMS 7.3.1
#   vxworks       - Compile for VxWorks Tornado 2.0 using GCC
#   win32         - Compile for Microsoft Win32 with Visual C++ 4, 5, 6, or 7
#   win32-borland - Compile for Microsoft Win32 with Borland C++ Builder 5.5.1
#   win32-mingw   - Compile for Microsoft Win32 with MinGW 4.1.1 and MSYS 1.0.10
#
# Set MX_INSTALL_DIR to the directory that you want to install MX in.
# Next, check "mx/libMx/mxconfig.h" to see if any of the defines need to
# be changed.   Then do the command "make depend" followed by "make".
# If MX builds correctly, do "make install" to install the binaries
# in the requested location.  The directory $(MX_INSTALL_DIR) must exist
# before you do "make install".
#
# On 'win32', $(MX_INSTALL_DIR) should use forward slashes like / rather
# than backslashes.
#
# For 'djgpp' and 'vms' platforms, the 'scripts' subdirectory contains
# modified versions of the top level MX Makefile which, for those plaforms,
# should be used instead of this file.
#
# More detailed instructions may be found in the file "mx/README.install".
#

MX_ARCH = linux

MX_INSTALL_DIR = /opt/mx

#MX_INSTALL_DIR = $(HOME)/mxtest

#MX_INSTALL_DIR = c:/docume~1/lavender/mxtest

#MX_INSTALL_DIR = c:/docume~1/lavender/mxtest_cygwin

#MX_INSTALL_DIR = c:/opt/mx

MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH) MX_INSTALL_DIR=$(MX_INSTALL_DIR)

all: build

build: depend_files
	( cd tools ; $(MAKECMD) -f Makefile.$(MX_ARCH) )
	( cd libMx ; $(MAKECMD) )
	( cd motor ; $(MAKECMD) )
	( cd server ; $(MAKECMD) )
	( cd update ; $(MAKECMD) )
	( cd update_old ; $(MAKECMD) )
	( cd util ; $(MAKECMD) )

clean: depend_files
	( cd tools ; $(MAKECMD) -f Makefile.$(MX_ARCH) mx_clean )
	( cd libMx ; $(MAKECMD) mx_clean )
	( cd motor ; $(MAKECMD) mx_clean )
	( cd server ; $(MAKECMD) mx_clean )
	( cd update ; $(MAKECMD) mx_clean )
	( cd update_old ; $(MAKECMD) mx_clean )
	( cd util ; $(MAKECMD) mx_clean )

distclean: depend_files
	( cd tools ; $(MAKECMD) -f Makefile.$(MX_ARCH) mx_clean )
	( cd libMx ; $(MAKECMD) mx_distclean )
	( cd motor ; $(MAKECMD) mx_distclean )
	( cd server ; $(MAKECMD) mx_distclean )
	( cd update ; $(MAKECMD) mx_distclean )
	( cd update_old ; $(MAKECMD) mx_distclean )
	( cd util ; $(MAKECMD) mx_distclean )

depend: depend_files
	( cd libMx ; $(MAKECMD) mx_depend )
	( cd motor ; $(MAKECMD) mx_depend )
	( cd server ; $(MAKECMD) mx_depend )
	( cd update ; $(MAKECMD) mx_depend )
	( cd update_old ; $(MAKECMD) mx_depend )
	( cd util ; $(MAKECMD) mx_depend )

install: depend_files
	( cd libMx ; $(MAKECMD) mx_install )
	( cd motor ; $(MAKECMD) mx_install )
	( cd server ; $(MAKECMD) mx_install )
	( cd update ; $(MAKECMD) mx_install )
###	( cd update_old ; $(MAKECMD) mx_install )
	( cd util ; $(MAKECMD) mx_install )

#------------------------------------------------------------------------------

MD = Makefile.depend

depend_files: libMx/$(MD) motor/$(MD) server/$(MD) update/$(MD) \
					update_old/$(MD) util/$(MD)

libMx/$(MD):
	touch libMx/$(MD)

motor/$(MD):
	touch motor/$(MD)

server/$(MD):
	touch server/$(MD)

update/$(MD):
	touch update/$(MD)

update_old/$(MD):
	touch update_old/$(MD)

util/$(MD):
	touch util/$(MD)

