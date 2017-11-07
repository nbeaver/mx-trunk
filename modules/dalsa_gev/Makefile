#
# This module is only for the 'linux' platform.
#

DALSA_GEV_INCLUDES += -I/usr/dalsa/GigeV/include
DALSA_GEV_INCLUDES += -I/opt/genicam_v3_0/library/CPP/include

DALSA_GEV_LIBRARIES = -L/usr/dalsa/GigeV/lib -lGevApi -lCorW32

#--------------------------------------------------------------------------

all: dalsa_gev.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS_MODULE = -I../../libMx -D__MX_LIBRARY__ $(DALSA_GEV_INCLUDES)

DALSA_GEV_OBJS = dalsa_gev.$(OBJ) i_dalsa_gev.$(OBJ) d_dalsa_gev_camera.$(OBJ)

#--------------------------------------------------------------------------

#
# The vendor include files make a lot of use of functions that are forbidden
# in the core of MX like sprintf(), strcpy(), and so forth.  We must define
# MX_NO_POISON to relax that restriction.
#

GCC = gcc
GPP = g++

CFLAGS = -g -fPIC $(GCCFLAGS) -DOS_LINUX -DDEBUG -DMX_NO_POISON $(CFLAGS_MODULE)

#
# DALSA redefineds a bunch of macros like INT8_MAX, etc., so we define the
# following include file directory as 'system' to suppress the warnings.
#

CFLAGS += -isystem /usr/dalsa/GigeV/include

# Suppress GCC complaints about Visual Studio-specific pragmas.

CFLAGS += -Wno-unknown-pragmas

#--------------------------------------------------------------------------

dalsa_gev.mxo: $(DALSA_GEV_OBJS)
	gcc -shared -rdynamic -Wl,-soname,dalsa_gev.mxo \
		-o dalsa_gev.mxo $(DALSA_GEV_OBJS) \
		$(DALSA_GEV_LIBRARIES)

#--------------------------------------------------------------------------

dalsa_gev.$(OBJ): dalsa_gev.c i_dalsa_gev.h d_dalsa_gev_camera.h
	$(GCC) -c $(CFLAGS) dalsa_gev.c

i_dalsa_gev.$(OBJ): i_dalsa_gev.c i_dalsa_gev.h
	$(GPP) -c $(CFLAGS) i_dalsa_gev.c

d_dalsa_gev_camera.$(OBJ): d_dalsa_gev_camera.c \
				d_dalsa_gev_camera.h i_dalsa_gev.h
	$(GPP) -c $(CFLAGS) d_dalsa_gev_camera.c

install: dalsa_gev.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp dalsa_gev.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


