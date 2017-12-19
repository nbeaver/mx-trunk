

ifndef MX_ARCH
    MX_ARCH = linux
endif

all: epics_aps.mxo

#
# Customization of the EPICS configuration is performed in the
# Makehead.config file of the 'epics' module.
#

include ../epics/Makefile.config

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += $(EPICS_INCLUDES)
CFLAGS += -I../epics/

EPICS_APS_OBJS = epics_aps.$(OBJ) \
	d_aps_gap.$(OBJ) d_aps_quadem_amplifier.$(OBJ) \
	sq_aps_id.$(OBJ) v_aps_topup.$(OBJ)

#--------------------------------------------------------------------------

# Cygwin

ifeq ($(MX_ARCH),cygwin)

epics_aps.mxo: $(EPICS_APS_OBJS)
	dllwrap -o epics_aps.mxo \
		$(EPICS_APS_OBJS) ../../libMx/libMx.dll.a ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(use_linux),TRUE)

epics_aps.mxo: $(EPICS_APS_OBJS)
	$(CC) $(SANITIZE) -shared -rdynamic -Wl,-soname,epics_aps.mxo \
		-o epics_aps.mxo \
		$(EPICS_APS_OBJS) -L../../libMx -lMx ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# MacOS X

ifeq ($(MX_ARCH),macosx)

epics_aps.mxo: $(EPICS_APS_OBJS)
	gcc -dynamiclib -o epics_aps.mxo \
		$(EPICS_APS_OBJS) ../../libMx/libMx.dylib ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# MacOS X (with Clang)

ifeq ($(MX_ARCH),macosx-clang)

epics_aps.mxo: $(EPICS_APS_OBJS)
	clang -dynamiclib -o epics_aps.mxo \
		$(EPICS_APS_OBJS) ../../libMx/libMx.dylib ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Solaris

ifeq ($(MX_ARCH),solaris)

epics_aps.mxo: $(EPICS_APS_OBJS)
	ld -G -o epics_aps.mxo \
		$(EPICS_APS_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Solaris GCC

ifeq ($(MX_ARCH),solaris-gcc)

epics_aps.mxo: $(EPICS_APS_OBJS)
	ld -G -o epics_aps.mxo \
		$(EPICS_APS_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

epics_aps.mxo: $(EPICS_APS_OBJS)
	link /dll /debug /nologo /out:epics_aps.mxo /nodefaultlib:libc \
		$(EPICS_APS_OBJS) ..\\epics\\epics.lib ..\\..\\libMx/libMx.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f epics_aps.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:epics_aps.mxo\;2 \
			-manifest epics_aps.mxo.manifest; \
		rm -f epics_aps.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

epics_aps.$(OBJ): epics_aps.c 
	$(COMPILE) $(CFLAGS) epics_aps.c

d_aps_gap.$(OBJ): d_aps_gap.c
	$(COMPILE) $(CFLAGS) d_aps_gap.c

d_aps_quadem_amplifier.$(OBJ): d_aps_quadem_amplifier.c
	$(COMPILE) $(CFLAGS) d_aps_quadem_amplifier.c

sq_aps_id.$(OBJ): sq_aps_id.c
	$(COMPILE) $(CFLAGS) sq_aps_id.c

v_aps_topup.$(OBJ): v_aps_topup.c
	$(COMPILE) $(CFLAGS) v_aps_topup.c

#--------------------------------------------------------------------------

INSTALLED_MODULE_NAME=$(MX_INSTALL_DIR)/lib/modules/epics_aps.mxo

ifeq ($(MX_ARCH),macosx-clang)

install: epics_aps.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epics_aps.mxo $(INSTALLED_MODULE_NAME)
	/usr/bin/install_name_tool -id "$(INSTALLED_MODULE_NAME)" \
				$(INSTALLED_MODULE_NAME)
	mx_mac_fix_lib_path $(INSTALLED_MODULE_NAME) \
			$(MX_INSTALL_DIR)/lib/libMx.dylib
	mx_mac_fix_lib_path $(INSTALLED_MODULE_NAME) \
			$(MX_INSTALL_DIR)/lib/modules/epics.mxo

else

install: epics_aps.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epics_aps.mxo $(INSTALLED_MODULE_NAME)

endif

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

