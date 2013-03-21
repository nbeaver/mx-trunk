#
# Change the value of the MX_ARCH variable below to match the platform you
# are compiling the MX system for.  The available platforms are:
#
#   bsd           - Compile for FreeBSD, NetBSD, or OpenBSD
#   cygwin        - Compile for Cygwin 1.5 or 1.7
#   djgpp         - Compile for DOS extender with DJGPP 2.0.3 or above
#   ecos          - Compile for x86 with eCos
#   hpux          - Compile for HP 9000/8xx with HP-UX 11v2
#   hpux-gcc      - Compile for HP 9000/8xx with HP-UX 11v2 using GCC
#   hurd          - Compile for GNU/Hurd
#   irix          - Compile for SGI with Irix 6.3 or above
#   irix-gcc      - Compile for SGI with Irix 6.5 using GCC
#   linux         - Compile for Linux 2.2 or above using GCC
#   linux-icc     - Compile for Linux 2.6 or above using Intel C++.
#   macosx        - Compile for x86/powerpc with MacOS X
#   qnx           - Compile for x86 with QNX Neutrino
#   rtems         - Compile for x86/m68k/powerpc with RTEMS
#   solaris       - Compile for sparc/x86 with Solaris 8 or above
#   solaris-gcc   - Compile for sparc/x86 with Solaris 8 or above using GCC
#   tru64         - Compile for alpha with HP Tru64 Unix 5.1b
#   tru64-gcc     - Compile for alpha with HP Tru64 Unix 5.1b using GCC
#   vms           - Compile for alpha/vax/ia64 with OpenVMS 7.3.1 or 8.x
#   vms-gnv       - Compile for ia64 with OpenVMS 8.3 using GNV 1.6-4
#   vxworks       - Compile for VxWorks Tornado 2.0 using GCC
#   win32         - Compile for Microsoft Win32 with Visual C++ 4 or above
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
# Some platform specific instructions and configuration variables can be
# found in the mx/libMx/Makehead.$(MX_ARCH) file, where MX_ARCH is the
# variable that you set below.
#
# Optional dynamically loaded MX modules are found in the mx/modules
# directory tree.  You select the modules to compile by editing the file
# mx/modules/Makefile.  The individual modules are configured by editing
# the makefiles named mx/module/*/Makefile.  If you are using it, the
# 'epics' module is a special case and requires some configuration in
# mx/module/epics/Makefile.config.
#

MX_ARCH = win32

#MX_INSTALL_DIR = /opt/mx
#MX_INSTALL_DIR = c:/opt/mx
#MX_INSTALL_DIR = /mnt/mx
#MX_INSTALL_DIR = /mnt/sdcard/opt/mx
#MX_INSTALL_DIR = $(HOME)/local/mx
#MX_INSTALL_DIR = $(HOME)/mxtest
#MX_INSTALL_DIR = c:/docume~1/lavender/mxtest
MX_INSTALL_DIR = c:/opt/mx-1.5.5-2013_03_20

MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH) MX_INSTALL_DIR=$(MX_INSTALL_DIR)

all: build

build: depend_files
	( cd tools ; $(MAKECMD) -f Makefile.$(MX_ARCH) )
	( cd libMx ; $(MAKECMD) )
	( cd motor ; $(MAKECMD) )
	( cd server ; $(MAKECMD) )
	( cd autosave ; $(MAKECMD) )
	( cd util ; $(MAKECMD) )

clean: depend_files
	( cd tools ; $(MAKECMD) -f Makefile.$(MX_ARCH) mx_clean )
	( cd libMx ; $(MAKECMD) mx_clean )
	( cd motor ; $(MAKECMD) mx_clean )
	( cd server ; $(MAKECMD) mx_clean )
	( cd autosave ; $(MAKECMD) mx_clean )
	( cd util ; $(MAKECMD) mx_clean )

distclean: depend_files
	( cd tools ; $(MAKECMD) -f Makefile.$(MX_ARCH) mx_clean )
	( cd libMx ; $(MAKECMD) mx_distclean )
	( cd motor ; $(MAKECMD) mx_distclean )
	( cd server ; $(MAKECMD) mx_distclean )
	( cd autosave ; $(MAKECMD) mx_distclean )
	( cd util ; $(MAKECMD) mx_distclean )

depend: depend_files
	( cd libMx ; $(MAKECMD) mx_depend )
	( cd motor ; $(MAKECMD) mx_depend )
	( cd server ; $(MAKECMD) mx_depend )
	( cd autosave ; $(MAKECMD) mx_depend )
	( cd util ; $(MAKECMD) mx_depend )

install: depend_files
	( cd libMx ; $(MAKECMD) mx_install )
	( cd motor ; $(MAKECMD) mx_install )
	( cd server ; $(MAKECMD) mx_install )
	( cd autosave ; $(MAKECMD) mx_install )
	( cd util ; $(MAKECMD) mx_install )

#------------------------------------------------------------------------------

modules: modules-build

modules-build:
	( cd modules ; $(MAKECMD) build )

modules-clean:
	( cd modules ; $(MAKECMD) clean )

modules-distclean:
	( cd modules ; $(MAKECMD) distclean )

modules-depend:
	( cd modules ; $(MAKECMD) depend )

modules-install:
	( cd modules ; $(MAKECMD) install )

#------------------------------------------------------------------------------

MD = Makefile.depend

depend_files: libMx/$(MD) motor/$(MD) server/$(MD) autosave/$(MD) util/$(MD)

libMx/$(MD):
	touch libMx/$(MD)

motor/$(MD):
	touch motor/$(MD)

server/$(MD):
	touch server/$(MD)

autosave/$(MD):
	touch autosave/$(MD)

util/$(MD):
	touch util/$(MD)

