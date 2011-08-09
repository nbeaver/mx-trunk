#
# Warning: This makefile assumes that Gnu Make is being used to build
#          the modules.  Other versions of Make will probably fail.
#          It also assumes the presence of a Unix-style Bash or Bourne
#          shell.
#
# You enable the building of each module by uncommenting the "MODULES +="
# line for that module below.
#
# You configure each module by editing the Makefile in its subdirectory.
#

all: build

#-------------------------------------------------------------------
# Multiplatform modules.

#MODULES += daqmx_base		# linux, win32

#MODULES += epix_xclib		# linux, win32

#-------------------------------------------------------------------
# Win32-only modules.

#MODULES += bnc725_lib

#MODULES += pleora_iport

#MODULES += radicon_helios

#-------------------------------------------------------------------
# EPICS modules.

MODULES += epics

#MODULES += epics_aps

#MODULES += epics_biocat

#MODULES += epics_mbc

#MODULES += epics_pmac_tc

#---

MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH) MX_INSTALL_DIR=$(MX_INSTALL_DIR)

build:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) ) \
	done
	
clean:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) clean ) \
	done
	
distclean:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) distclean ) \
	done
	
depend:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) depend ) \
	done
	
install:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) install ) \
	done
	
