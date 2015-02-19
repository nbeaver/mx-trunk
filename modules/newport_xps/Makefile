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

all: newport_xps.mxo

NEWPORT_XPS_SOURCE = ftp://download.newport.com/MotionControl/Current/MotionControllers/XPS-C/Software/Drivers/DLL/

VENDOR_OBJS = XPS_C8_drivers.$(OBJ)

#
# If NEWPORT_XPS_DISTCLEAN_DELETE is set to "true", then invoking 
#
#    make modules-distclean
#
# from the top level MX makefile will cause the distclean to delete the
# source code that was downloaded from Newport's FTP site.  If this 
# variable is set to anything other than "true" then the Newport source
# code will not be deleted.  Normally, you should leave this variable
# set to "false".
#

NEWPORT_XPS_DISTCLEAN_DELETE = false

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

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

NEWPORT_XPS_OBJS = newport_xps.$(OBJ) \
		i_newport_xps.$(OBJ) d_newport_xps.$(OBJ) \
		Socket_MX.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

XPS_C8_DRIVERS_FLAGS = -g -fPIC

newport_xps.mxo: vendor_downloads $(NEWPORT_XPS_OBJS) $(VENDOR_OBJS)
	$(CXX) -shared -rdynamic -Wl,-soname,newport_xps.mxo \
		-o newport_xps.mxo $(NEWPORT_XPS_OBJS) $(VENDOR_OBJS) \
		../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

newport_xps.$(OBJ): newport_xps.c i_newport_xps.h d_newport_xps.h
	$(COMPILE) $(CFLAGS) newport_xps.c

i_newport_xps.$(OBJ): i_newport_xps.c i_newport_xps.h
	$(COMPILE) $(CFLAGS) i_newport_xps.c

d_newport_xps.$(OBJ): d_newport_xps.c d_newport_xps.h i_newport_xps.h
	$(COMPILE) $(CFLAGS) d_newport_xps.c

install: newport_xps.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp newport_xps.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest tmp.*

#----

ifeq ($(NEWPORT_XPS_DISTCLEAN_DELETE),true)

distclean: clean
	echo "*** Deleting downloaded Newport source code ***"
	-$(RM) XPS_C8_drivers.cpp
	-$(RM) XPS_C8_drivers.h
	-$(RM) xps_c8_drivers.h
	-$(RM) Socket.cpp
	-$(RM) Socket.h
	-$(RM) strtok_r.cpp
	-$(RM) strtok_r.h
	-$(RM) *.orig

else

distclean: clean

endif

#----

depend:

#--------------------------------------------------------------------------

vendor_downloads: Socket.h XPS_C8_drivers.h XPS_C8_errors.h XPS_C8_drivers.cpp

Socket.h:
	../../scripts/download $(NEWPORT_XPS_SOURCE)/Socket.h

XPS_C8_drivers.h:
	../../scripts/download $(NEWPORT_XPS_SOURCE)/XPS_C8_drivers.h 

XPS_C8_errors.h:
	../../scripts/download $(NEWPORT_XPS_SOURCE)/XPS_C8_errors.h 

XPS_C8_drivers.cpp:
	../../scripts/download $(NEWPORT_XPS_SOURCE)/XPS_C8_drivers.cpp 

#--------------------------------------------------------------------------

XPS_C8_drivers.$(OBJ): XPS_C8_drivers.cpp XPS_C8_drivers.h Socket.h
	$(CXX) $(XPS_C8_DRIVERS_FLAGS) -c XPS_C8_drivers.cpp

Socket_MX.$(OBJ): Socket_MX.c Socket.h
	$(COMPILE) $(CFLAGS) Socket_MX.c

