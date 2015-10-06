#
# This module can be used on the 'linux' and 'win32' platforms.
#

#---------------------------------------------------------------------------
# If MX_ARCH is not defined, figure it out from the output of "uname".
#

os := $(shell uname -o)

ifndef MX_ARCH

ifeq ($(os),GNU/Linux)
MX_ARCH = linux
endif

#----------------------
# Note that the win32 platform is built from within Cygwin but does not
# link to Cygwin.  Instead, it links to MSVCRT.
#

ifeq ($(os),Cygwin)
MX_ARCH = win32
endif

endif

#---------------------------------------------------------------------------
# If EPIX_ARCH is not defined, figure it out from the output of "uname".
#

ifndef EPIX_ARCH

ifeq ($(MX_ARCH),linux)

machine := $(shell uname -m)

ifeq ($(machine),i686)
EPIX_ARCH = i386
endif
ifeq ($(machine),x86_64)
EPIX_ARCH = x86_64_pic
endif

#----------------------

ifeq ($(MX_ARCH),win32)

kernel := $(shell uname -s | cut -f3 -d'-' )

ifeq ($(kernel),WOW64)

endif

#EPIX_ARCH = i386
EPIX_ARCH = x86_64_pic
#EPIX_ARCH = w64

EPIX_XCLIB_DIR = /usr/local/xclib

#EPIX_XCLIB_DIR = c:\\progra~1\\EPIX\\XCLIB

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

