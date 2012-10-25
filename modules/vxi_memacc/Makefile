#
# Driver for the NI-VISA VXI/VME access via the MEMACC resource.
#
# The vendor software for this device must be purchased from 
# National Instruments.
#
# Thus far, only the Linux version of this driver has been tested.
#
#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)
	VXI_MEMACC_DIR       = /usr/local/vxipnp/linux
	VXI_MEMACC_INCLUDES  = $(VXI_MEMACC_DIR)/include
	VXI_MEMACC_LIB_DIRS  = $(VXI_MEMACC_DIR)/lib
	VXI_MEMACC_LIBRARIES = -l_vxi_memacc
endif

#--------------------------------------------------------------------------

all: vxi_memacc.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(VXI_MEMACC_INCLUDES)

VXI_MEMACC_OBJS = vxi_memacc.$(OBJ) i_vxi_memacc.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

vxi_memacc.mxo: $(VXI_MEMACC_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,vxi_memacc.mxo \
		-o vxi_memacc.mxo $(VXI_MEMACC_OBJS) \
		-L$(VXI_MEMACC_LIB_DIRS) -l_vxi_memacc

endif

#--------------------------------------------------------------------------

vxi_memacc.$(OBJ): vxi_memacc.c i_vxi_memacc.h
	$(COMPILE) $(CFLAGS) vxi_memacc.c

i_vxi_memacc.$(OBJ): i_vxi_memacc.c i_vxi_memacc.h
	$(COMPILE) $(CFLAGS) -DIS_MX_DRIVER i_vxi_memacc.c

install: vxi_memacc.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp vxi_memacc.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

