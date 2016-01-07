#
# This modules is only for the 'win32' and 'linux' platforms.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifeq ($(MX_ARCH),linux)
	DALSA_GEV_INCLUDES += -I/usr/dalsa/GigeV/include

	DALSA_GEV_LIBRARIES = -L/usr/dalsa/GigeV/lib -lGevApi
	DALSA_GEV_LIBRARIES += -L/usr/local/lib -lCorW32
endif

ifeq ($(MX_ARCH),win32)
	#
	# Note: The directory name DALSA_GEV_DIR must _not_ have any spaces
	# in it.  This means names like "C:\\Program Files\\Teledyne DALSA"
	# are not acceptable.
	#

	DALSA_GEV_DIR = c:\\progra~1\\teledy~1\\GigeV

	DALSA_GEV_INCLUDES += -I$(DALSA_GEV_DIR)\\Include

	DALSA_GEV_LIBRARIES = $(DALSA_GEV_DIR)\\Lib\\Win64\\aaaaaa.lib
endif

#--------------------------------------------------------------------------

all: dalsa_gev.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS_MODULE = -I../../libMx -D__MX_LIBRARY__ $(DALSA_GEV_INCLUDES)

DALSA_GEV_OBJS = dalsa_gev.$(OBJ) i_dalsa_gev.$(OBJ) d_dalsa_gev_camera.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

#
# The vendor include files make a lot of use of functions that are forbidden
# in the core of MX like sprintf(), strcpy(), and so forth.  We must define
# MX_NO_POISON to relax that restriction.
#
# POSIX_HOSTPC must be defined for Linux builds.  If it is not, then the
# GigeV include files appear to assume that we are compiling on Windows
# and start decorating everything with __declspec() attributes.  This
# behavior of the header files does not seem to be documented anywhere.
#

COMPILE = gcc -c

CFLAGS = -g -fPIC $(GCCFLAGS) -DOS_LINUX -DDEBUG -DMX_NO_POISON $(CFLAGS_MODULE)

dalsa_gev.mxo: $(DALSA_GEV_OBJS)
	gcc -shared -rdynamic -Wl,-soname,dalsa_gev.mxo \
		-o dalsa_gev.mxo $(DALSA_GEV_OBJS) \
		$(DALSA_GEV_LIBRARIES)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

CFLAGS += -EHsc
CFLAGS += $(CFLAGS_MODULE)

dalsa_gev.mxo: $(DALSA_GEV_OBJS)
	link /dll /debug /nologo /out:dalsa_gev.mxo /nodefaultlib:libc \
		$(DALSA_GEV_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(DALSA_GEV_LIBRARIES)

	# For VC2005 or later, embed the manifest in the module.

	if test -f dalsa_gev.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:dalsa_gev.mxo\;2 \
			-manifest dalsa_gev.mxo.manifest ; \
		rm -f dalsa_gev.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

dalsa_gev.$(OBJ): dalsa_gev.c i_dalsa_gev.h d_dalsa_gev_camera.h
	$(COMPILE) $(CFLAGS) dalsa_gev.c

i_dalsa_gev.$(OBJ): i_dalsa_gev.c i_dalsa_gev.h
	$(COMPILE) $(CFLAGS) i_dalsa_gev.c

d_dalsa_gev_camera.$(OBJ): d_dalsa_gev_camera.c \
				d_dalsa_gev_camera.h i_dalsa_gev.h
	$(COMPILE) $(CFLAGS) d_dalsa_gev_camera.c

install: dalsa_gev.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp dalsa_gev.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


