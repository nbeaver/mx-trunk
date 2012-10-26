#
# This module can either be compiled to produce an 'ni488' module that uses
# the National Instruments GPIB libraries, or to produce a 'linux_gpib'
# module that uses the Linux GPIB package that can be downloaded from
# http://linux-gpib.sourceforge.net/.  You cannot simultaneously build
# it for both.
#
# The National Instruments GPIB drivers are cross platform, but they have
# only been tested with MX under Linux.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

#BUILD_TARGET = ni488

BUILD_TARGET = linux_gpib

#--------------------------------------------------------------------------

ifeq ($(BUILD_TARGET),ni488)

    CFLAGS += -DHAVE_NI488

    ifeq ($(MX_ARCH),linux)
	NI488_DIR = /usr/local
	#
	NI488_INCLUDES = $(NI488_DIR)/include
	NI488_LIB_DIR  = $(NI488_DIR)/lib
	#
	# Note: If you use the Ethernet-based GPIB interface, then you
	#       should link using -lgpibenet rather than -lgpib.
	#
	NI488_LIBRARY = -lgpib
	#NI488_LIBRARY = -lgpibenet
    endif
endif

ifeq ($(BUILD_TARGET),linux_gpib)

	CFLAGS += -DHAVE_LINUX_GPIB

	NI488_DIR      = /usr/local/mx
	#
	NI488_INCLUDES = $(NI488_DIR)/include
	NI488_LIB_DIR  = $(NI488_DIR)/lib
	NI488_LIBRARY  = -lgpib
	
endif

#--------------------------------------------------------------------------

all: $(BUILD_TARGET).mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += -I$(NI488_INCLUDES)

NI488_OBJS = ni488.$(OBJ) i_ni488.$(OBJ)

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

$(BUILD_TARGET).mxo: $(NI488_OBJS)
	gcc -shared -rdynamic -Wl,-soname,$(BUILD_TARGET).mxo -o ni488.mxo \
		$(NI488_OBJS) ../../libMx/libMx.so \
		-L$(NI488_LIB_DIR) $(NI488_LIBRARY)
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

ni488.mxo: $(NI488_OBJS)
	link /dll /debug /nologo /out:ni488.mxo /nodefaultlib:libc \
		$(NI488_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(NI488_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f ni488.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:ni488.mxo\;2 \
			-manifest ni488.mxo.manifest ; \
		rm -f ni488.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ni488.$(OBJ): ni488.c i_ni488.h
	$(COMPILE) $(CFLAGS) ni488.c

i_ni488.$(OBJ): i_ni488.c i_ni488.h
	$(COMPILE) $(CFLAGS) i_ni488.c

install: $(MODULE_MXO)
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp $(MODULE_MXO) $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

