#
# Warning: This makefile assumes that Gnu Make is being used to build
#          the modules.  Other versions of Make will probably fail.
#          It also assumes the presence of a Unix-style Bash or Bourne
#          shell.
#
# You configure the building of each module by editing the Makefile
# in its subdirectory.
#

all: build

# Win32-only modules.

MODULES += pleora_iport

MODULES += radicon_helios

#MODULES += bnc725_lib

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
	
