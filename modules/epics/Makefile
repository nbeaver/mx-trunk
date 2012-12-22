

ifndef MX_ARCH
    MX_ARCH = linux
endif

all: epics.mxo

#
# Customization of the EPICS configuration is performed in Makehead.config
#

include Makefile.config

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += $(EPICS_INCLUDES)

EPICS_OBJS = epics.$(OBJ) \
	d_epics_aio.$(OBJ) d_epics_area_detector.$(OBJ) d_epics_ccd.$(OBJ) \
	d_epics_dio.$(OBJ) d_epics_mca.$(OBJ) d_epics_mcs.$(OBJ) \
	d_epics_motor.$(OBJ) d_epics_scaler.$(OBJ) d_epics_timer.$(OBJ) \
	i_epics_camac.$(OBJ) i_epics_gpib.$(OBJ) \
	i_epics_rs232.$(OBJ) i_epics_vme.$(OBJ) \
	mx_epics.$(OBJ) mx_vepics.$(OBJ) sq_joerger.$(OBJ) \
	v_epics_timeout.$(OBJ)

#--------------------------------------------------------------------------

# Cygwin

ifeq ($(MX_ARCH),cygwin)

epics.mxo: $(EPICS_OBJS)
	dllwrap -o epics.mxo \
		$(EPICS_OBJS) ../../libMx/libMx.so \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

epics.mxo: $(EPICS_OBJS)
	gcc -shared -rdynamic -Wl,-soname,epics.mxo -o epics.mxo \
		$(EPICS_OBJS) ../../libMx/libMx.so \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# MacOS X

ifeq ($(MX_ARCH),macosx)

epics.mxo: $(EPICS_OBJS)
	gcc -dynamiclib -o epics.mxo \
		$(EPICS_OBJS) ../../libMx/libMx.dylib \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Solaris

ifeq ($(MX_ARCH),solaris)

epics.mxo: $(EPICS_OBJS)
	ld -G -o epics.mxo \
		$(EPICS_OBJS) ../../libMx/libMx.so \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Solaris GCC

ifeq ($(MX_ARCH),solaris-gcc)

epics.mxo: $(EPICS_OBJS)
	ld -G -o epics.mxo \
		$(EPICS_OBJS) ../../libMx/libMx.so \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

epics.mxo: $(EPICS_OBJS)
	link /dll /debug /nologo /out:epics.mxo /nodefaultlib:libc \
		$(EPICS_OBJS) ..\\..\\libMx/libMx.lib $(EPICS_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f epics.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:epics.mxo\;2 \
			-manifest epics.mxo.manifest; \
		rm -f epics.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

epics.$(OBJ): epics.c 
	$(COMPILE) $(CFLAGS) epics.c

d_epics_aio.$(OBJ): d_epics_aio.c
	$(COMPILE) $(CFLAGS) d_epics_aio.c

d_epics_area_detector.$(OBJ): d_epics_area_detector.c
	$(COMPILE) $(CFLAGS) d_epics_area_detector.c

d_epics_ccd.$(OBJ): d_epics_ccd.c
	$(COMPILE) $(CFLAGS) d_epics_ccd.c

d_epics_dio.$(OBJ): d_epics_dio.c
	$(COMPILE) $(CFLAGS) d_epics_dio.c

d_epics_mca.$(OBJ): d_epics_mca.c
	$(COMPILE) $(CFLAGS) d_epics_mca.c

d_epics_mcs.$(OBJ): d_epics_mcs.c
	$(COMPILE) $(CFLAGS) d_epics_mcs.c

d_epics_motor.$(OBJ): d_epics_motor.c
	$(COMPILE) $(CFLAGS) d_epics_motor.c

d_epics_scaler.$(OBJ): d_epics_scaler.c
	$(COMPILE) $(CFLAGS) d_epics_scaler.c

d_epics_timer.$(OBJ): d_epics_timer.c
	$(COMPILE) $(CFLAGS) d_epics_timer.c

i_epics_gpib.$(OBJ): i_epics_gpib.c
	$(COMPILE) $(CFLAGS) i_epics_gpib.c

i_epics_rs232.$(OBJ): i_epics_rs232.c
	$(COMPILE) $(CFLAGS) i_epics_rs232.c

i_epics_vme.$(OBJ): i_epics_vme.c
	$(COMPILE) $(CFLAGS) i_epics_vme.c

mx_epics.$(OBJ): mx_epics.c
	$(COMPILE) $(MX_EPICS_CFLAGS) mx_epics.c

mx_vepics.$(OBJ): mx_vepics.c
	$(COMPILE) $(CFLAGS) mx_vepics.c

sq_joerger.$(OBJ): sq_joerger.c
	$(COMPILE) $(CFLAGS) sq_joerger.c

#--------------------------------------------------------------------------

install: epics.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epics.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

