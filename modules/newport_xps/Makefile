#
# Linux and Win32 drivers for the Newport XPS-C and XPS-Q series of motion
# controllers.
#
# The MX 'newport_xps' module makes use of C code written by Newport that
# is downloadable from their FTP site.  The C code does not have any stated
# license, so it is not possible for license reasons to directly include it
# in the MX Subversion repository.  Instead, what we do is to have the
# makefile for this module automatically download the needed C code from
# Newport's FTP site to this directory.  The macro NEWPORT_XPS_SOURCE
# used below specifies the URL for the directory on Newport's FTP server
# that contains the needed C code.
#

NEWPORT_XPS_SOURCE = ftp://download.newport.com/MotionControl/Current/MotionControllers/XPS-C/Software/Drivers/DLL/

VENDOR_OBJS = XPS_C8_drivers.$(OBJ)

#
# Tested values of MX_ARCH:  linux
#
# Note that the Newport C code seems fairly portable, so it can probably
# be compiled for other MX build architectures besides the ones listed
# above.

ifndef MX_ARCH
  MX_ARCH = linux
endif

#--------------------------------------------------------------------------

all: newport_xps.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

NEWPORT_XPS_OBJS = newport_xps.$(OBJ) i_newport_xps.$(OBJ) d_newport_xps.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

newport_xps.mxo: vendor_downloads $(NEWPORT_XPS_OBJS) $(VENDOR_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,newport_xps.mxo \
		-o newport_xps.mxo $(NEWPORT_XPS_OBJS)

endif

#--------------------------------------------------------------------------

newport_xps.$(OBJ): newport_xps.c
	$(COMPILE) $(CFLAGS) newport_xps.c

install: newport_xps.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp newport_xps.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest tmp.*

distclean: clean
	-$(RM) xps_c8_drivers.*
	-$(RM) XPS_C8_drivers.*
	-$(RM) Socket.*
	-$(RM) strtok_r.*

depend:

#--------------------------------------------------------------------------

vendor_downloads:
	../../scripts/download $(NEWPORT_XPS_SOURCE)/Socket.cpp 
	../../scripts/download $(NEWPORT_XPS_SOURCE)/Socket.h 
	../../scripts/download $(NEWPORT_XPS_SOURCE)/XPS_C8_drivers.cpp 
	../../scripts/download $(NEWPORT_XPS_SOURCE)/XPS_C8_drivers.h 
	#
	cat XPS_C8_drivers.cpp | sed s/^#typedef/typedef/ > tmp.sed
	mv tmp.sed XPS_C8_drivers.cpp
	#
	cat XPS_C8_drivers.h | sed s/^#typedef/typedef/ > tmp.sed
	mv tmp.sed XPS_C8_drivers.h
	#
	ln -sf XPS_C8_drivers.h xps_c8_drivers.h

XPS_C8_drivers.$(OBJ):
	$(COMPILE) XPS_C8_drivers.cpp

