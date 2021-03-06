#
# This module can be used on the 'linux' and 'win32' platforms.
#
#---------------------------------------------------------------------------
#
# If you need to override the values of MX_ARCH, EPIX_ARCH, or EPIX_XCLIB_DIR,
# then do so following this comment here.
#

#---------------------------------------------------------------------------
# If MX_ARCH is not defined, get it from 'mx_config'.
#

ifndef MX_ARCH
MX_ARCH := $(shell ../../tools/mx_config mx_arch)
endif

#---------------------------------------------------------------------------
# If EPIX_ARCH is not defined, figure it out from the output of 'mx_config'.
#

ifndef EPIX_ARCH

wordsize := $(shell ../../tools/mx_config wordsize)

#----------------------

ifeq ($(MX_ARCH),linux)

ifeq ($(wordsize),32)
EPIX_ARCH = i386
endif
ifeq ($(wordsize),64)
EPIX_ARCH = x86_64_pic
endif

endif

#----------------------

ifeq ($(MX_ARCH),win32)

ifeq ($(wordsize),32)
EPIX_ARCH = w32
endif
ifeq ($(wordsize),64)
EPIX_ARCH = w64
endif

endif

#----------------------

endif

#---------------------------------------------------------------------------
# If EPIX_XCLIB_DIR is not defined, figure from variables defined earlier
# in this Makefile.
#

ifndef EPIX_XCLIB_DIR

ifeq ($(MX_ARCH),linux)
EPIX_XCLIB_DIR = /usr/local/xclib
endif
ifeq ($(MX_ARCH),win32)
EPIX_XCLIB_DIR = c:\\progra~1\\EPIX\\XCLIB
endif

endif

#--------------------------------------------------------------------------

all: epix_xclib.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += -I$(EPIX_XCLIB_DIR)

EPIX_XCLIB_OBJS = epix_xclib.$(OBJ) \
		i_epix_xclib.$(OBJ) i_epix_rs232.$(OBJ) \
		i_epix_camera_link.$(OBJ) \
		d_epix_xclib.$(OBJ) d_epix_xclib_dio.$(OBJ)

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

CFLAGS += -Wno-strict-prototypes -Wno-shadow -Wno-unused-variable

epix_xclib.mxo: $(EPIX_XCLIB_OBJS)
	pwd
	gcc -shared -rdynamic -Wl,-soname,epix_xclib.mxo -o epix_xclib.mxo \
		$(EPIX_XCLIB_OBJS) ../../libMx/libMx.so \
		$(EPIX_XCLIB_DIR)/xclib_$(EPIX_ARCH).a
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

epix_xclib.mxo: $(EPIX_XCLIB_OBJS)
	link /dll /debug /nologo /out:epix_xclib.mxo /nodefaultlib:libc \
		$(EPIX_XCLIB_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(EPIX_XCLIB_DIR)\\xclib$(EPIX_ARCH).lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f epix_xclib.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:epix_xclib.mxo\;2 \
			-manifest epix_xclib.mxo.manifest ; \
		rm -f epix_xclib.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

epix_xclib.$(OBJ): epix_xclib.c \
			i_epix_xclib.h i_epix_rs232.h i_epix_camera_link.h \
			d_epix_xclib.h d_epix_xclib_dio.h
	$(COMPILE) $(CFLAGS) epix_xclib.c

i_epix_xclib.$(OBJ): i_epix_xclib.c i_epix_xclib.h
	$(COMPILE) $(CFLAGS) i_epix_xclib.c

i_epix_xclib_rs232.$(OBJ): i_epix_xclib_rs232.c \
			i_epix_xclib_rs232.h i_epix_xclib.h
	$(COMPILE) $(CFLAGS) i_epix_xclib_rs232.c

i_epix_xclib_camera_link.$(OBJ): i_epix_xclib_camera_link.c \
			i_epix_xclib_camera_link.h i_epix_xclib.h
	$(COMPILE) $(CFLAGS) i_epix_xclib_camera_link.c

d_epix_xclib.$(OBJ): d_epix_xclib.c d_epix_xclib.h \
			i_epix_xclib.h i_epix_camera_link.h
	$(COMPILE) $(CFLAGS) d_epix_xclib.c

d_epix_xclib_dio.$(OBJ): d_epix_xclib_dio.c d_epix_xclib_dio.h \
			i_epix_xclib.h d_epix_xclib.h
	$(COMPILE) $(CFLAGS) d_epix_xclib_dio.c

install: epix_xclib.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp epix_xclib.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

