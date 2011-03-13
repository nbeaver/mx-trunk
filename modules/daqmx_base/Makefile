#
# Supported values of MX_ARCH:  win32
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

ifeq ($(MX_ARCH),win32)
	DAQMX_BASE_DIR = c:\\progra~1\\nation~1\\ni-daq~1

	CFLAGS += -DWIN32
endif

#--------------------------------------------------------------------------

all: daqmx_base.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(DAQMX_BASE_DIR)\\Include

DAQMX_BASE_OBJS = daqmx_base.$(OBJ) i_daqmx_base.$(OBJ) \
		d_daqmx_base_dinput.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

daqmx_base.mxo: $(DAQMX_BASE_OBJS)
	link /dll /debug /nologo /out:daqmx_base.mxo /nodefaultlib:libc \
		$(DAQMX_BASE_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(DAQMX_BASE_DIR)\\Lib\\nidaqmxbase.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f daqmx_base.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:daqmx_base.mxo\;2 \
			-manifest daqmx_base.mxo.manifest ; \
		rm -f daqmx_base.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

daqmx_base.$(OBJ): daqmx_base.c i_daqmx_base.h \
			d_daqmx_base_dinput.h
	$(COMPILE) $(CFLAGS) daqmx_base.c

i_daqmx_base.$(OBJ): i_daqmx_base.c i_daqmx_base.h
	$(COMPILE) $(CFLAGS) i_daqmx_base.c

d_daqmx_base_dinput.$(OBJ): d_daqmx_base_dinput.c i_daqmx_base.h \
			d_daqmx_base_dinput.h
	$(COMPILE) $(CFLAGS) d_daqmx_base_dinput.c

install: daqmx_base.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp daqmx_base.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

