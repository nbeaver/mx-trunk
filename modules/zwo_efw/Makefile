#
# Linux, MacOS, and Win32 drivers for ZWO Electronic Filter Wheels.
#

all: zwo_efw.mxo

#--------------------------------------------------------------------------

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

ZWO_EFW_OBJS = zwo_efw.$(OBJ) d_zwo_efw_motor.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)
	ZWO_EFW_DIR = /opt/libEFWFilter
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)
	ZWO_EFW_DIR = zzzz
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

COMPILE = $(CC) -c

CFLAGS += -I$(ZWO_EFW_DIR)/include

zwo_efw.mxo: $(ZWO_EFW_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,zwo_efw.mxo \
		-o zwo_efw.mxo $(ZWO_EFW_OBJS) \
		-L$(ZWO_EFW_DIR)/lib/Linux/x64 -lEFWFilter ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

COMPILE = cl

zwo_efw.mxo: $(ZWO_EFW_OBJS) $(VENDOR_OBJS)
	link /dll /debug /nologo /out:zwo_efw.mxo \
		/nodefaultlib:libc /nodefaultlib:libcmt \
		$(ZWO_EFW_OBJS) \
		$(ZWO_EFW_DIR\\zzzz\\efw_filter.lib \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

endif

#--------------------------------------------------------------------------

zwo_efw.$(OBJ): zwo_efw.c d_zwo_efw_motor.h
	$(COMPILE) $(CFLAGS) zwo_efw.c

d_zwo_efw_motor.$(OBJ): d_zwo_efw_motor.h
	$(COMPILE) $(CFLAGS) d_zwo_efw_motor.c

install: zwo_efw.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp zwo_efw.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest tmp.*

#----

distclean: clean

#----

depend:

#--------------------------------------------------------------------------

