#
# Module: winusb
#
# This module supports the WinUSB driver for Microsoft Windows implemented
# by the Windows driver Winusb.sys.
#
# Supported values of MX_ARCH: win32
#

ifndef MX_ARCH
    MX_ARCH = win32
endif

WINUSB_DIR = c:\\docume~1\\lavender\\winusb
CFLAGS += -EHsc
CFLAGS += -I$(WINUSB_DIR)\\Include

all: winusb.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

WINUSB_OBJS = winusb.$(OBJ) i_winusb.$(OBJ) 

#--------------------------------------------------------------------------

winusb.mxo: $(WINUSB_OBJS)
	link /dll /debug /nologo /out:winusb.mxo /nodefaultlib:libc \
		$(WINUSB_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME)

#		$(WINUSB_DIR)\\Lib\\PccLib.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f winusb.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:winusb.mxo\;2 \
			-manifest winusb.mxo.manifest ; \
		rm -f winusb.mxo.manifest; \
	fi

#--------------------------------------------------------------------------

winusb.$(OBJ): winusb.c i_winusb.h
	$(COMPILE) $(CFLAGS) winusb.c

i_winusb.$(OBJ): i_winusb.c i_winusb.h
	$(COMPILE) $(CFLAGS) i_winusb.c

install: winusb.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp winusb.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.o *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


