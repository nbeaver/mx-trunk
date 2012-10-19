#
# Supported values of MX_ARCH: linux
#

ifndef MX_ARCH
    MX_ARCH = linux
endif

ifeq ($(MX_ARCH),win32)
	LIBUSB-0.1_DIR = c:\\docume~1\\lavender\\archives\\photron\\PFV2452\\SDK\\VC
	CFLAGS += -EHsc
	CFLAGS += -I$(LIBUSB-0.1_DIR)\\Include
endif

all: libusb-0.1.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

LIBUSB-0.1_OBJS = libusb-0.1.$(OBJ) i_libusb-0.1.$(OBJ) 

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

libusb-0.1.mxo: $(LIBUSB-0.1_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,libusb-0.1.mxo \
		-o libusb-0.1.mxo $(LIBUSB-0.1_OBJS) -lusb

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

libusb-0.1.mxo: $(LIBUSB-0.1_OBJS)
	link /dll /debug /nologo /out:libusb-0.1.mxo /nodefaultlib:libc \
		$(LIBUSB-0.1_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(LIBUSB-0.1_DIR)\\Lib\\PccLib.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f libusb-0.1.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:libusb-0.1.mxo\;2 \
			-manifest libusb-0.1.mxo.manifest ; \
		rm -f libusb-0.1.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

libusb-0.1.$(OBJ): libusb-0.1.c i_libusb-0.1.h
	$(COMPILE) $(CFLAGS) libusb-0.1.c

i_libusb-0.1.$(OBJ): i_libusb-0.1.c i_libusb-0.1.h
	$(COMPILE) $(CFLAGS) i_libusb-0.1.c

install: libusb-0.1.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libusb-0.1.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


