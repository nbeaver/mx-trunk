

ifndef MX_ARCH
    MX_ARCH = linux
endif

all: epics_mbc.mxo

#
# Customization of the EPICS configuration is performed in the
# Makehead.config file of the 'epics' module.
#

include ../epics/Makefile.config

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += $(EPICS_INCLUDES)
CFLAGS += -I../epics/

EPICS_MBC_OBJS = epics_mbc.$(OBJ) \
	d_mbc_gsc_trigger.$(OBJ) d_mbc_noir.$(OBJ) d_mbc_noir_trigger.$(OBJ)

#--------------------------------------------------------------------------

# Cygwin

ifeq ($(MX_ARCH),cygwin)

epics_mbc.mxo: $(EPICS_MBC_OBJS)
	dllwrap -o epics_mbc.mxo \
		$(EPICS_MBC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

epics_mbc.mxo: $(EPICS_MBC_OBJS)
	gcc -shared -o epics_mbc.mxo \
		$(EPICS_MBC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# MacOS X

ifeq ($(MX_ARCH),macosx)

epics_mbc.mxo: $(EPICS_MBC_OBJS)
	gcc -dynamiclib -o epics_mbc.mxo \
		$(EPICS_MBC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Solaris

ifeq ($(MX_ARCH),solaris)

epics_mbc.mxo: $(EPICS_MBC_OBJS)
	ld -G -o epics_mbc.mxo \
		$(EPICS_MBC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Solaris GCC

ifeq ($(MX_ARCH),solaris-gcc)

epics_mbc.mxo: $(EPICS_MBC_OBJS)
	ld -G -o epics_mbc.mxo \
		$(EPICS_MBC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

epics_mbc.mxo: $(EPICS_MBC_OBJS)
	link /dll /debug /nologo /out:epics_mbc.mxo /nodefaultlib:libc \
	    $(EPICS_MBC_OBJS) ..\\epics\\epics.lib ..\\..\\libMx/libMx.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f epics_mbc.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:epics_mbc.mxo\;2 \
			-manifest epics_mbc.mxo.manifest; \
		rm -f epics_mbc.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

epics_mbc.$(OBJ): epics_mbc.c 
	$(COMPILE) $(CFLAGS) epics_mbc.c

d_mbc_gsc_trigger.$(OBJ): d_mbc_gsc_trigger.c
	$(COMPILE) $(CFLAGS) d_mbc_gsc_trigger.c

d_mbc_noir.$(OBJ): d_mbc_noir.c
	$(COMPILE) $(CFLAGS) d_mbc_noir.c

d_mbc_noir_trigger.$(OBJ): d_mbc_noir_trigger.c
	$(COMPILE) $(CFLAGS) d_mbc_noir_trigger.c

#--------------------------------------------------------------------------

install: epics_mbc.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epics_mbc.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

