#
# This module contains drivers for the PvAPI C API used by
# cameras from Allied Vision Technologies.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

AVT_PVAPI_DIR = "/opt/AVT GigE SDK"

AVT_PVAPI_PLATFORM = x86
AVT_PVAPI_RELEASE  = 4.5

#--------------------------------------------------------------------------

all: avt_pvapi.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += -I$(AVT_PVAPI_DIR)/inc-pc

AVT_PVAPI_OBJS = avt_pvapi.$(OBJ) i_avt_pvapi.$(OBJ) d_avt_pvapi.$(OBJ)

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

#CFLAGS += -Wno-strict-prototypes

CFLAGS += -D_LINUX

avt_pvapi.mxo: $(AVT_PVAPI_OBJS)
	g++ -shared -rdynamic -Wl,-zmuldefs \
		-Wl,-soname,avt_pvapi.mxo -o avt_pvapi.mxo \
		$(AVT_PVAPI_OBJS) ../../libMx/libMx.so \
	  -L$(AVT_PVAPI_DIR)/lib-pc/$(AVT_PVAPI_PLATFORM)/$(AVT_PVAPI_RELEASE) \
		-lPvAPI
endif

#--------------------------------------------------------------------------

avt_pvapi.$(OBJ): avt_pvapi.c i_avt_pvapi.h d_avt_pvapi.h 
	$(COMPILE) $(CFLAGS) avt_pvapi.c

i_avt_pvapi.$(OBJ): i_avt_pvapi.c i_avt_pvapi.h
	$(COMPILE) $(CFLAGS) i_avt_pvapi.c

d_avt_pvapi.$(OBJ): d_avt_pvapi.c d_avt_pvapi.h i_avt_pvapi.h
	$(COMPILE) $(CFLAGS) d_avt_pvapi.c

install: avt_pvapi.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp avt_pvapi.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

