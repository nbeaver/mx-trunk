#
# This module supports Labjack U3, U6, and U9 devices.
#
# On Windows, this module uses the Labjack-provided UD drivers. 
# On Linux and Mac OS X, the module uses the Labjack-provided
# "exodriver".
#
# Supported values of MX_ARCH:  linux macosx win32
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

ifeq ($(MX_ARCH),win32)
	LABJACK_UX_DIR =

	CFLAGS += -I$(LABJACK_UX_DIR)/include

	LABJACK_UX_LIBRARY = 
else
	LABJACK_UX_DIR = /usr/local

	CFLAGS += -I$(LABJACK_UX_DIR)/include

	LABJACK_UX_LIBRARY = -L $(LABJACK_UX_DIR)/lib -llabjackusb
endif

#--------------------------------------------------------------------------

all: labjack_ux.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

#LABJACK_UX_OBJS = labjack_ux.$(OBJ) i_labjack_ux.$(OBJ) \
#		d_labjack_ux_ainput.$(OBJ) d_labjack_ux_aoutput.$(OBJ) \
#		d_labjack_ux_dinput.$(OBJ) d_labjack_ux_doutput.$(OBJ) \
#		d_labjack_ux_scaler.$(OBJ) d_labjack_ux_timer.$(OBJ)

LABJACK_UX_OBJS = labjack_ux.$(OBJ) i_labjack_ux.$(OBJ) \
		d_labjack_ux_dinput.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

labjack_ux.mxo: $(LABJACK_UX_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,labjack_ux.mxo \
		-o labjack_ux.mxo $(LABJACK_UX_OBJS) $(LABJACK_UX_LIBRARY)
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

labjack_ux.mxo: $(LABJACK_UX_OBJS)
	link /dll /debug /nologo /out:labjack_ux.mxo /nodefaultlib:libc \
		$(LABJACK_UX_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(LABJACK_UX_LIBRARY)

endif

#--------------------------------------------------------------------------

#labjack_ux.$(OBJ): labjack_ux.c i_labjack_ux.h \
#			d_labjack_ux_ainput.h d_labjack_ux_aoutput.h \
#			d_labjack_ux_dinput.h d_labjack_ux_doutput.h \
#			d_labjack_ux_scaler.h d_labjack_ux_timer.h
labjack_ux.$(OBJ): labjack_ux.c i_labjack_ux.h
	$(COMPILE) $(CFLAGS) labjack_ux.c

i_labjack_ux.$(OBJ): i_labjack_ux.c i_labjack_ux.h

d_labjack_ux_ainput.$(OBJ): d_labjack_ux_ainput.c \
				i_labjack_ux.h d_labjack_ux_ainput.h

d_labjack_ux_aoutput.$(OBJ): d_labjack_ux_aoutput.c \
				i_labjack_ux.h d_labjack_ux_aoutput.h

d_labjack_ux_dinput.$(OBJ): d_labjack_ux_dinput.c \
				i_labjack_ux.h d_labjack_ux_dinput.h

d_labjack_ux_doutput.$(OBJ): d_labjack_ux_doutput.c \
				i_labjack_ux.h d_labjack_ux_doutput.h

d_labjack_ux_scaler.$(OBJ): d_labjack_ux_scaler.c \
				i_labjack_ux.h d_labjack_ux_scaler.h

d_labjack_ux_timer.$(OBJ): d_labjack_ux_timer.c \
				i_labjack_ux.h d_labjack_ux_timer.h

install: labjack_ux.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp labjack_ux.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

