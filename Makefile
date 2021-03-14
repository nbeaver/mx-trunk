#
# Change the value of the MX_ARCH variable below to match the platform you
# are compiling the MX system for.  The available platforms are:
#
#   bsd           - FreeBSD, NetBSD, or OpenBSD
#   bsd-clang     - FreeBSD with Clang 3.3 or above
#   cygwin        - Cygwin 1.5 or 1.7
#   djgpp         - DOS extender with DJGPP 2.0.3 or above
#   hpux          - HP 9000/8xx with HP-UX 11v2
#   hpux-gcc      - HP 9000/8xx with HP-UX 11v2 using GCC
#   hurd          - GNU/Hurd
#   irix          - SGI with Irix 6.3 or above
#   irix-gcc      - SGI with Irix 6.5 using GCC
#   linux         - Linux 2.2 or above using GCC
#   linux-icc     - Linux 2.6 or above using Intel C++.
#   linux-clang   - Linux 3.12 or above using Clang
#   macosx        - x86/powerpc with MacOS X using GCC
#   macosx-clang  - x86 with MacOS X using Clang
#   minix         - Minix 3.3.0 using Clang
#   qnx           - x86 with QNX Neutrino 6.x
#   rtems         - x86/m68k/powerpc with RTEMS
#   solaris       - sparc/x86 with Solaris 8 or above
#   solaris-gcc   - sparc/x86 with Solaris 8 or above using GCC
#   termux        - Android 7.1 Bionic with Termux Clang
#   tru64         - alpha with HP Tru64 Unix 5.1b
#   tru64-gcc     - alpha with HP Tru64 Unix 5.1b using GCC
#   unixware      - UnixWare 7
#   vms           - alpha/vax/ia64 with OpenVMS 7.3.1 or 8.x
#   vms-gnv       - ia64 with OpenVMS 8.3 using GNV 1.6-4
#   vxworks       - VxWorks Tornado 2.0 using GCC
#   win32         - Microsoft Win32 with Visual C++ 4 or above
#   win32-borland - Microsoft Win32 with Borland C++ Builder 5.5.1
#   win32-mingw   - Microsoft Win32 with MinGW-w64
#
#------------------------------------------------------------------------------
#
#   WARNING: MX makefiles require GNU Make 3.80 or above.
#
#   BSD make, Microsoft NMAKE, Solaris make, and the like are not supported.
#
#------------------------------------------------------------------------------
#
#   On Microsoft Windows, you must have a Unix-like shell environment with a
#   shell compatible with sh/ksh/bash to build MX.  This can typically be done
#   by installing Cygwin or possibly Msys.  If you are using the Visual C++
#   compiler with Cygwin, then you must also source a compiler setup script
#   for your version of the compiler from the ones found in the directory
#   mx/scripts/windows/visual_studio.
#
#   The Unix-like shell environment must supply the following commands:
#     awk, cp, cut, echo, head, make, rm, sh, test, touch, tr
#
#   Performing 'make depend' also requires that the shell environment
#   provide a 'makedepend' command.
#
#------------------------------------------------------------------------------
#
# Set MX_INSTALL_DIR to the directory that you want to install MX in.
# Then, do the command "make depend" followed by "make".  If MX builds
# correctly, do "make install" to install the binaries in the requested
# location.  The directory $(MX_INSTALL_DIR) must exist # before you do
# "make install".
#
# On 'win32', $(MX_INSTALL_DIR) must use forward slashes like / rather
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

#
# Attempt to automatically determine the values of MX_ARCH and MX_INSTALL_DIR.
#
# The logic works as follows:
#
# 1. If MX_ARCH and/or MX_INSTALL_DIR are defined in your shell's environment
#    variables, then they will automatically take precedence over the 
#    definitions below.
#
# 2. If they are not defined, but the 'mx/tools/mx_config' program was built by
#    a previous 'make' command, then it will be interrogated for the values.
#
#    This has to be done for things like 'sudo make install' to work correctly
#    on non-Linux platforms where 'sudo' strips out variables like MX_ARCH
#    and MX_INSTALL_DIR from the root environment.
#
# 3. If none of the above is done, then the defaults are 'linux' and '/opt/mx'.
#
# Note: You can always ignore the above and explicitly set values for MX_ARCH
# and MX_INSTALL_DIR below.
#

#MX_ARCH=linux
#MX_INSTALL_DIR=/opt/mx
#MX_INSTALL_DIR = c:/opt/mx-2.1.3-2017_12_07

#--------

ifndef MX_ARCH
  ifneq ($(wildcard tools/mx_config),)
    MX_ARCH=$(shell tools/mx_config mx_arch)
  else
    MX_ARCH = linux
  endif
endif

ifndef MX_INSTALL_DIR
  ifneq ($(wildcard tools/mx_config),)
    MX_INSTALL_DIR=$(shell tools/mx_config mx_install_dir)
  else
    MX_INSTALL_DIR = /opt/mx
  endif
endif

$(info MX_ARCH is [${MX_ARCH}])
$(info MX_INSTALL_DIR is [${MX_INSTALL_DIR}])

#------------------------------------------------------------------------------

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

