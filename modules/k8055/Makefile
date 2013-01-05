#
# This module supports the Velleman K8055 USB Experiment Interface Board.
#
# The module supports the following software interfaces:
#
# 1.  K8055D.DLL
#       This is the DLL that is distributed by Velleman for the board.
#       Bear in mind that the DLL is 32-bit only.  Version 4 of the
#       DLL was used.
#
# 2.  libk8055
#       This package found at http://libk8055.sourceforge.net/ provides
#       Linux support.
#

#
# USE_LIBK8055 is used on Win32 to decide whether or not to use the
# vendor-provided or the libk8055 library ported to libusb-win32.
# USE_LIBK8055 only has an effect for the 'win32' build target.
#

USE_LIBK8055 = TRUE

#
# Supported values of MX_ARCH:  linux win32
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

ifeq ($(MX_ARCH),linux)
	LIBK8055_DIR = /usr/local

	CFLAGS += -I$(LIBK8055_DIR)/include
endif

ifeq ($(MX_ARCH),win32)
	K8055_DIR = c:\\progra~1\\velleman

	K8055_LIBRARY = $(K8055_DIR)\\k8055.lib
endif

#--------------------------------------------------------------------------

all: k8055.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

K8055_OBJS = k8055.$(OBJ) i_k8055.$(OBJ) \
		d_k8055_ainput.$(OBJ) d_k8055_aoutput.$(OBJ) \
		d_k8055_dinput.$(OBJ) d_k8055_doutput.$(OBJ) \
		d_k8055_scaler.$(OBJ) d_k8055_timer.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

k8055.mxo: $(K8055_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,k8055.mxo \
		-o k8055.mxo $(K8055_OBJS) -L$(LIBK8055_DIR)/lib -lk8055
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

k8055.mxo: $(K8055_OBJS)
	link /dll /debug /nologo /out:k8055.mxo /nodefaultlib:libc \
		$(K8055_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(K8055_LIBRARY)

endif

#--------------------------------------------------------------------------

k8055.$(OBJ): k8055.c i_k8055.h \
			d_k8055_ainput.h d_k8055_aoutput.h \
			d_k8055_dinput.h d_k8055_doutput.h \
			d_k8055_scaler.h d_k8055_timer.h
	$(COMPILE) $(CFLAGS) k8055.c

i_k8055.$(OBJ): i_k8055.c i_k8055.h

d_k8055_ainput.$(OBJ): d_k8055_ainput.c i_k8055.h d_k8055_ainput.h

d_k8055_aoutput.$(OBJ): d_k8055_aoutput.c i_k8055.h d_k8055_aoutput.h

d_k8055_dinput.$(OBJ): d_k8055_dinput.c i_k8055.h d_k8055_dinput.h

d_k8055_doutput.$(OBJ): d_k8055_doutput.c i_k8055.h d_k8055_doutput.h

d_k8055_scaler.$(OBJ): d_k8055_scaler.c i_k8055.h d_k8055_scaler.h

d_k8055_timer.$(OBJ): d_k8055_timer.c i_k8055.h d_k8055_timer.h

install: k8055.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp k8055.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

