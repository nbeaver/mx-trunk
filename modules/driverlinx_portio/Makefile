#
# The DriverLINX Port I/O driver allows a user mode program running under
# Windows NT to access Intel x86 I/O ports from 0x0100 to 0xFFFF.  This
# driver was written by Scientific Software Tools, Inc. and may be downloaded
# from http://www.sstnet.com/dnload/dnload.htm.  This driver also works
# under Windows 95/98, but on those machines it is probably simpler to
# just use the 'dos_portio' driver.
#
# The 'driverlinx_portio' module is only for the 'win32' platform.
#

MX_ARCH = win32

DRIVERLINX_DIR      = c:\\progra~1\\DLPortIO
DRIVERLINX_INCLUDES = -I$(DRIVERLINX_DIR)\\API
DRIVERLINX_LIBS     = $(DRIVERLINX_DIR)\\API\\DLPORTIO.lib

all: driverlinx_portio.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += $(DRIVERLINX_INCLUDES)

ORTEC_DRIVERLINX_OBJS = driverlinx_portio.$(OBJ) i_driverlinx_portio.$(OBJ)

driverlinx_portio.mxo: $(ORTEC_DRIVERLINX_OBJS)
	link /dll /debug /nologo /out:driverlinx_portio.mxo /nodefaultlib:libc \
		$(ORTEC_DRIVERLINX_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(DRIVERLINX_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f driverlinx_portio.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:driverlinx_portio.mxo\;2 \
			-manifest driverlinx_portio.mxo.manifest ; \
		rm -f driverlinx_portio.mxo.manifest; \
	fi

driverlinx_portio.$(OBJ): driverlinx_portio.c
	$(COMPILE) $(CFLAGS) driverlinx_portio.c

i_driverlinx_portio.$(OBJ): i_driverlinx_portio.c i_driverlinx_portio.h
	$(COMPILE) $(CFLAGS) i_driverlinx_portio.c

install: driverlinx_portio.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp driverlinx_portio.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

