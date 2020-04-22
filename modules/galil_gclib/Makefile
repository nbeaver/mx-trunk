#
# Linux and Win32 drivers for Galil motor controllers using the gclib library.
#

all: galil_gclib.mxo

#--------------------------------------------------------------------------

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

NEWPORT_XPS_OBJS = galil_gclib.$(OBJ) \
		i_galil_gclib.$(OBJ) d_galil_gclib.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

XPS_C8_DRIVERS_FLAGS = -g -fPIC

galil_gclib.mxo: $(NEWPORT_XPS_OBJS)
	$(CXX) -shared -rdynamic -Wl,-soname,galil_gclib.mxo \
		-o galil_gclib.mxo $(NEWPORT_XPS_OBJS) \
		../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

XPS_C8_DRIVERS_FLAGS = -D_WIN32

CXX = cl

galil_gclib.mxo: $(NEWPORT_XPS_OBJS) $(VENDOR_OBJS)
	link /dll /debug /nologo /out:galil_gclib.mxo \
		/nodefaultlib:libc /nodefaultlib:libcmt \
		$(NEWPORT_XPS_OBJS)
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

endif

#--------------------------------------------------------------------------

galil_gclib.$(OBJ): galil_gclib.c i_galil_gclib.h d_galil_gclib.h
	$(COMPILE) $(CFLAGS) galil_gclib.c

i_galil_gclib.$(OBJ): i_galil_gclib.c i_galil_gclib.h
	$(COMPILE) $(CFLAGS) i_galil_gclib.c

d_galil_gclib.$(OBJ): d_galil_gclib.c d_galil_gclib.h i_galil_gclib.h
	$(COMPILE) $(CFLAGS) d_galil_gclib.c

install: galil_gclib.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp galil_gclib.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest tmp.*

#----

distclean: clean

#----

depend:

#--------------------------------------------------------------------------

