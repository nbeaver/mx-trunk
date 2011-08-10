

ifndef MX_ARCH
    MX_ARCH = linux
endif

all: epics_biocat.mxo

#
# Customization of the EPICS configuration is performed in the
# Makehead.config file of the 'epics' module.
#

include ../epics/Makefile.config

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += $(EPICS_INCLUDES)
CFLAGS += -I../epics/

EPICS_BIOCAT_OBJS = epics_biocat.$(OBJ) \
		d_aps_18id.$(OBJ)

#--------------------------------------------------------------------------

# Cygwin

ifeq ($(MX_ARCH),cygwin)

epics_biocat.mxo: $(EPICS_BIOCAT_OBJS)
	dllwrap -o epics_biocat.mxo \
		$(EPICS_BIOCAT_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

epics_biocat.mxo: $(EPICS_BIOCAT_OBJS)
	gcc -shared -o epics_biocat.mxo \
		$(EPICS_BIOCAT_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# MacOS X

ifeq ($(MX_ARCH),macosx)

epics_biocat.mxo: $(EPICS_BIOCAT_OBJS)
	gcc -dynamiclib -o epics_biocat.mxo \
		$(EPICS_BIOCAT_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Solaris

ifeq ($(MX_ARCH),solaris)

epics_biocat.mxo: $(EPICS_BIOCAT_OBJS)
	ld -G -o epics_biocat.mxo \
		$(EPICS_BIOCAT_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Solaris GCC

ifeq ($(MX_ARCH),solaris-gcc)

epics_biocat.mxo: $(EPICS_BIOCAT_OBJS)
	ld -G -o epics_biocat.mxo \
		$(EPICS_BIOCAT_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

epics_biocat.mxo: $(EPICS_BIOCAT_OBJS)
	link /dll /debug /nologo /out:epics_biocat.mxo /nodefaultlib:libc \
	    $(EPICS_BIOCAT_OBJS) ..\\epics\\epics.lib ..\\..\\libMx/libMx.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f epics_biocat.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:epics_biocat.mxo\;2 \
			-manifest epics_biocat.mxo.manifest; \
		rm -f epics_biocat.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

epics_biocat.$(OBJ): epics_biocat.c 
	$(COMPILE) $(CFLAGS) epics_biocat.c

d_aps_18id.$(OBJ): d_aps_18id.c
	$(COMPILE) $(CFLAGS) d_aps_18id.c

#--------------------------------------------------------------------------

install: epics_biocat.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epics_biocat.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

