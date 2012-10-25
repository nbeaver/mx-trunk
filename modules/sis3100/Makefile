#
# Driver for the Struck SIS1100/3100 PCI-to-VME bus interface.
#
# The vendor software for this device can be downloaded from here:
#   http://www.struck.de/pcivme.htm
#
# Thus far, only the Linux version of this driver has been tested.
#
#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)
	SIS3100_DIR       = /usr/local/sis1100-2.13-6/sis3100_calls
	SIS3100_INCLUDES  = $(SIS3100_DIR)
	SIS3100_LIB_DIRS  = $(SIS3100_DIR)
	SIS3100_LIBRARIES = -l_sis3100
endif

#--------------------------------------------------------------------------

all: sis3100.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

SIS3100_OBJS = sis3100.$(OBJ) i_sis3100.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

sis3100.mxo: $(SIS3100_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,sis3100.mxo \
		-o sis3100.mxo $(SIS3100_OBJS)

endif

#--------------------------------------------------------------------------

sis3100.$(OBJ): sis3100.c i_sis3100.h
	$(COMPILE) $(CFLAGS) sis3100.c

i_sis3100.$(OBJ): i_sis3100.c i_sis3100.h
	$(COMPILE) $(CFLAGS) i_sis3100.c

install: sis3100.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp sis3100.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

