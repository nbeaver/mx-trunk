#
# Linux and Win32 drivers for Galil motor controllers using the gclib library.
#

all: galil_gclib.mxo

#--------------------------------------------------------------------------

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

GALIL_GCLIB_OBJS = galil_gclib.$(OBJ) \
		i_galil_gclib.$(OBJ) d_galil_gclib_motor.$(OBJ) \
		d_galil_gclib_ainput.$(OBJ) d_galil_gclib_aoutput.$(OBJ) \
		d_galil_gclib_dinput.$(OBJ) d_galil_gclib_doutput.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

galil_gclib.mxo: $(GALIL_GCLIB_OBJS)
	$(CXX) -shared -rdynamic -Wl,-soname,galil_gclib.mxo \
		-o galil_gclib.mxo $(GALIL_GCLIB_OBJS) \
		-lgclibo -lgclib ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

CXX = cl

galil_gclib.mxo: $(GALIL_GCLIB_OBJS) $(VENDOR_OBJS)
	link /dll /debug /nologo /out:galil_gclib.mxo \
		/nodefaultlib:libc /nodefaultlib:libcmt \
		$(GALIL_GCLIB_OBJS) \
		gclibo.lib gclib.lib \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

endif

#--------------------------------------------------------------------------

galil_gclib.$(OBJ): galil_gclib.c i_galil_gclib.h d_galil_gclib_motor.h \
			d_galil_gclib_ainput.h d_galil_gclib_aoutput.h \
			d_galil_gclib_dinput.h d_galil_gclib_doutput.h
	$(COMPILE) $(CFLAGS) galil_gclib.c

i_galil_gclib.$(OBJ): i_galil_gclib.c i_galil_gclib.h
	$(COMPILE) $(CFLAGS) i_galil_gclib.c

d_galil_gclib_motor.$(OBJ): d_galil_gclib_motor.h i_galil_gclib.h
	$(COMPILE) $(CFLAGS) d_galil_gclib_motor.c

d_galil_gclib_ainput.$(OBJ): d_galil_gclib_ainput.c i_galil_gclib.h
	$(COMPILE) $(CFLAGS) d_galil_gclib_ainput.c

d_galil_gclib_aoutput.$(OBJ): d_galil_gclib_aoutput.c i_galil_gclib.h
	$(COMPILE) $(CFLAGS) d_galil_gclib_aoutput.c

d_galil_gclib_dinput.$(OBJ): d_galil_gclib_dinput.c i_galil_gclib.h
	$(COMPILE) $(CFLAGS) d_galil_gclib_dinput.c

d_galil_gclib_doutput.$(OBJ): d_galil_gclib_doutput.c i_galil_gclib.h
	$(COMPILE) $(CFLAGS) d_galil_gclib_doutput.c

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

