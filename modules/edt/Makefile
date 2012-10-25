#
# This module contains drivers for the EDT family of video capture cards
# that are found here:
#
#   http://www.edt.com/
#
# Note: Although EDT distributes driver software for a variety of different
#       operating systems, the MX driver (so far) has only been tested on
#       Linux.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

EDT_DIR = /opt/EDTpdv

#--------------------------------------------------------------------------

all: edt.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += -I$(EDT_DIR)

EDT_OBJS = edt.$(OBJ) i_edt.$(OBJ) i_edt_rs232.$(OBJ) d_edt.$(OBJ)

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

CFLAGS += -Wno-strict-prototypes

edt.mxo: $(EDT_OBJS)
	gcc -shared -rdynamic -Wl,-soname,edt.mxo -o edt.mxo \
		$(EDT_OBJS) ../../libMx/libMx.so \
		-L$(EDT_DIR) -lpdv
endif

#--------------------------------------------------------------------------

edt.$(OBJ): edt.c i_edt.h i_edt_rs232.h d_edt.h 
	$(COMPILE) $(CFLAGS) edt.c

i_edt.$(OBJ): i_edt.c i_edt.h
	$(COMPILE) $(CFLAGS) i_edt.c

i_edt_rs232.$(OBJ): i_edt_rs232.c i_edt_rs232.h i_edt.h
	$(COMPILE) $(CFLAGS) i_edt_rs232.c

d_edt.$(OBJ): d_edt.c d_edt.h i_edt.h
	$(COMPILE) $(CFLAGS) d_edt.c

install: edt.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp edt.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

