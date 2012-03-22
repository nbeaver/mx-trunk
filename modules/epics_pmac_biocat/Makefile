

ifndef MX_ARCH
    MX_ARCH = linux
endif

all: epics_pmac_biocat.mxo

#
# Customization of the EPICS configuration is performed in the
# Makehead.config file of the 'epics' module.
#

include ../epics/Makefile.config

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += $(EPICS_INCLUDES)
CFLAGS += -I../epics/

EPICS_TC_OBJS = epics_pmac_biocat.$(OBJ) d_epics_pmac_biocat.$(OBJ)

#--------------------------------------------------------------------------

# Cygwin

ifeq ($(MX_ARCH),cygwin)

epics_pmac_biocat.mxo: $(EPICS_TC_OBJS)
	dllwrap -o epics_pmac_biocat.mxo \
		$(EPICS_TC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

epics_pmac_biocat.mxo: $(EPICS_TC_OBJS)
	gcc -shared -rdynamic -Wl,-soname,epics_pmac_biocat.mxo \
		-o epics_pmac_biocat.mxo \
		$(EPICS_TC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# MacOS X

ifeq ($(MX_ARCH),macosx)

epics_pmac_biocat.mxo: $(EPICS_TC_OBJS)
	gcc -dynamiclib -o epics_pmac_biocat.mxo \
		$(EPICS_TC_OBJS) ../../libMx/libMx.dylib ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES)
endif

#--------------------------------------------------------------------------

# Solaris

ifeq ($(MX_ARCH),solaris)

epics_pmac_biocat.mxo: $(EPICS_TC_OBJS)
	ld -G -o epics_pmac_biocat.mxo \
		$(EPICS_TC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Solaris GCC

ifeq ($(MX_ARCH),solaris-gcc)

epics_pmac_biocat.mxo: $(EPICS_TC_OBJS)
	ld -G -o epics_pmac_biocat.mxo \
		$(EPICS_TC_OBJS) ../../libMx/libMx.so ../epics/epics.mxo \
		$(EPICS_LIB_DIRS) $(EPICS_LIBRARIES) -lc
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

epics_pmac_biocat.mxo: $(EPICS_TC_OBJS)
	link /dll /debug /nologo /out:epics_pmac_biocat.mxo /nodefaultlib:libc \
	    $(EPICS_TC_OBJS) ..\\epics\\epics.lib ..\\..\\libMx/libMx.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f epics_pmac_biocat.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:epics_pmac_biocat.mxo\;2 \
			-manifest epics_pmac_biocat.mxo.manifest; \
		rm -f epics_pmac_biocat.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

epics_pmac_biocat.$(OBJ): epics_pmac_biocat.c d_epics_pmac_biocat.h
	$(COMPILE) $(CFLAGS) epics_pmac_biocat.c

d_epics_pmac_biocat.$(OBJ): d_epics_pmac_biocat.c d_epics_pmac_biocat.h
	$(COMPILE) $(CFLAGS) d_epics_pmac_biocat.c

#--------------------------------------------------------------------------

install: epics_pmac_biocat.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epics_pmac_biocat.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

