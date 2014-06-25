#
# This module contains drivers for the Vimba C API used by
# cameras from Allied Vision Technologies.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

AVT_VIMBA_DIR = /opt/VimbaSDK

#AVT_VIMBA_WORDLENGTH = 64
AVT_VIMBA_WORDLENGTH = 32

#--------------------------------------------------------------------------

all: avt_vimba.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += -I$(AVT_VIMBA_DIR)/VimbaC/Include

AVT_VIMBA_OBJS = avt_vimba.$(OBJ) i_avt_vimba.$(OBJ) d_avt_vimba.$(OBJ)

#--------------------------------------------------------------------------

# Linux
#
# FIXME: It should not be necessary to specify the -Wl,-zmuldefs option
# to the linker below.  The problem is that the VimbaC.h header file
# written by Allied Vision Technologies contains a line like this
#
#   const VmbHandle_t  gVimbaHandle = (VmbHandle_t)1;
#
# where gVimbaHandle is a global variable.
# 
# The only situation in which this is valid C is if the VimbaC.h 
# header file is only included by _one_ file of C code.  If it is
# included by two or more C code files, you will get errors like
# this when you try to link the resulting object files
#
#   i_avt_vimba.o:(.rodata+0x0): multiple definition of `gVimbaHandle'
#   avt_vimba.o:(.rodata+0x0): first defined here
#   d_avt_vimba.o:(.rodata+0x0): multiple definition of `gVimbaHandle'
#   avt_vimba.o:(.rodata+0x0): first defined here
#
# The general consensus of StackOverflow and the internet community
# is that you should _never_ _ever_ initialize the value of a 
# global variable in a header file that may be #included by more
# than one C source code file.  Unfortunately, AVT has done this
# very thing, so I have to use the workaround of passing the
# option -zmuldefs to the Gnu linker.  Ick!
#

ifeq ($(MX_ARCH),linux)

CFLAGS += -Wno-strict-prototypes

avt_vimba.mxo: $(AVT_VIMBA_OBJS)
	gcc -shared -rdynamic -Wl,-zmuldefs \
		-Wl,-soname,avt_vimba.mxo -o avt_vimba.mxo \
		$(AVT_VIMBA_OBJS) ../../libMx/libMx.so \
	  -L$(AVT_VIMBA_DIR)/VimbaC/DynamicLib/x86_$(AVT_VIMBA_WORDLENGTH)bit \
		-lVimbaC
endif

#--------------------------------------------------------------------------

avt_vimba.$(OBJ): avt_vimba.c i_avt_vimba.h d_avt_vimba.h 
	$(COMPILE) $(CFLAGS) avt_vimba.c

i_avt_vimba.$(OBJ): i_avt_vimba.c i_avt_vimba.h
	$(COMPILE) $(CFLAGS) i_avt_vimba.c

d_avt_vimba.$(OBJ): d_avt_vimba.c d_avt_vimba.h i_avt_vimba.h
	$(COMPILE) $(CFLAGS) d_avt_vimba.c

install: avt_vimba.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp avt_vimba.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

