#
# This module is only for the 'linux' platform.
#
# WARNING: When building the DALSA GeV libraries for version 2.0.x, you
# _MUST_ make a change to the file DALSA/src/gevlib/Makefile.
#
# As said by Hugh Parliament of Teledyne DALSA:
#
#----------------------------------------------------------------------------
# Look in the file DALSA/src/gevlib/Makefile
#
# At the bottom there is a line specific to the source file "gev_xml_utils.c"
#
# Replace it with the one below - it adds in the "LEGACY_XML_FIXUP_SUPPORT" definition.
#
# # Specific file handling..... (to quiet warnings etc....)
# gev_xml_utils.o: gev_xml_utils.c
# 	$(CC) -I. $(INC_PATH) $(C_COMPILE_OPTIONS) -Wno-attributes -DUSE_MINIZ -DLEGACY_XML_FIXUP_SUPPORT $(BASE_OPTIONS) $(TARGET_OPTIONS) $(ARCH_OPTIONS) -c $< -o $@
# 		
# 		Then do a reinstall / remake of the library.
#
# 		I had removed this definition because it had unintended consequences on another camera. It does remove the NULL label definition but is uses a techniques that affects the XML definitions used for the CDATA conditional constructs. AFAIK - these are not used by the Xray panels so you can re-enable it for your case.
#
#----------------------------------------------------------------------------

DALSA_GEV_INCLUDES += -I/usr/dalsa/GigeV/include
DALSA_GEV_INCLUDES += -I/opt/genicam_v3_0/library/CPP/include

DALSA_GEV_LIBRARIES = -L/usr/dalsa/GigeV/lib -lGevApi -lCorW32

#--------------------------------------------------------------------------

all: dalsa_gev.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS_MODULE = -I../../libMx -D__MX_LIBRARY__ $(DALSA_GEV_INCLUDES)

# DALSA provided files
#DALSA_GEV_OBJS += dynaqueue.o
#DALSA_GEV_OBJS += gev_get_next_image.o

DALSA_GEV_OBJS += dalsa_gev.$(OBJ) i_dalsa_gev.$(OBJ) d_dalsa_gev_camera.$(OBJ)


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

i_dalsa_gev.$(OBJ): i_dalsa_gev.cpp i_dalsa_gev.h
	$(GPP) -c $(CFLAGS) i_dalsa_gev.cpp

d_dalsa_gev_camera.$(OBJ): d_dalsa_gev_camera.cpp \
				d_dalsa_gev_camera.h i_dalsa_gev.h
	$(GPP) -c $(CFLAGS) d_dalsa_gev_camera.cpp

gev_get_next_image.$(OBJ): gev_get_next_image.cpp
	$(GPP) -c $(CFLAGS) gev_get_next_image.cpp

dynaqueue.$(OBJ): dynaqueue.c
	$(GCC) -c $(CFLAGS) -DPOSIX_HOSTPC -DHOSTPC \
		-DUSE_MONOTONIC_CLOCK -Dx86_64 -D_REENTRANT \
		dynaqueue.c

install: dalsa_gev.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp dalsa_gev.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


