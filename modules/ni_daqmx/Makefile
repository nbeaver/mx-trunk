#
# If you are building the software for National Instruments daqMx Base,
# then set USE_DAQMX_BASE to TRUE below.  Otherwise, set it to FALSE.
#

USE_DAQMX_BASE = FALSE

#
# Supported values of MX_ARCH:  linux win32
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

ifeq ($(MX_ARCH),linux)
	NI_DAQMX_DIR = /usr/local/natinst/nidaqmxbase

	CFLAGS += -I$(NI_DAQMX_DIR)/include
endif

ifeq ($(MX_ARCH),win32)
	NI_DAQMX_DIR = c:\\progra~1\\nation~1\\ni-daq~1

	CFLAGS += -DWIN32
	CFLAGS += -I$(NI_DAQMX_DIR)\\include
endif

ifeq ($(USE_DAQMX_BASE),TRUE)
	CFLAGS += -DUSE_DAQMX_BASE
endif

#--------------------------------------------------------------------------

all: ni_daqmx.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

NI_DAQMX_OBJS = ni_daqmx.$(OBJ) i_ni_daqmx.$(OBJ) \
		d_ni_daqmx_ainput.$(OBJ) d_ni_daqmx_aoutput.$(OBJ) \
		d_ni_daqmx_dinput.$(OBJ) d_ni_daqmx_doutput.$(OBJ) \
		d_ni_daqmx_thermocouple.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

#
# Read http://decibel.ni.com/content/thread/5902?decorator=print&displayFullThread=true
# for an explanation of why the line starting with -Wl,... must be present
# on Linux distributions derived from Red Hat Enterprise Linux 5.
#

ni_daqmx.mxo: $(NI_DAQMX_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,ni_daqmx.mxo \
		-o ni_daqmx.mxo $(NI_DAQMX_OBJS) \
		-lnidaqmxbase -lnidaqmxbaselv \
		/usr/local/lib/liblvrtdark.so.10.0 \
		/usr/local/lib/LabVIEW-2010/libniCPULib.so \
	-Wl,-rpath,'$$ORIGIN',-rpath,'/usr/local/lib/LabVIEW-2010/patchlib'

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

ni_daqmx.mxo: $(NI_DAQMX_OBJS)
	link /dll /debug /nologo /out:ni_daqmx.mxo /nodefaultlib:libc \
		$(NI_DAQMX_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(NI_DAQMX_DIR)\\Lib\\nidaqmxbase.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f ni_daqmx.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:ni_daqmx.mxo\;2 \
			-manifest ni_daqmx.mxo.manifest ; \
		rm -f ni_daqmx.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

ni_daqmx.$(OBJ): ni_daqmx.c i_ni_daqmx.h \
			d_ni_daqmx_ainput.h d_ni_daqmx_aoutput.h \
			d_ni_daqmx_dinput.h d_ni_daqmx_doutput.h \
			d_ni_daqmx_thermocouple.h
	$(COMPILE) $(CFLAGS) ni_daqmx.c

i_ni_daqmx.$(OBJ): i_ni_daqmx.c i_ni_daqmx.h
	$(COMPILE) $(CFLAGS) i_ni_daqmx.c

d_ni_daqmx_ainput.$(OBJ): d_ni_daqmx_ainput.c i_ni_daqmx.h \
			d_ni_daqmx_ainput.h
	$(COMPILE) $(CFLAGS) d_ni_daqmx_ainput.c

d_ni_daqmx_aoutput.$(OBJ): d_ni_daqmx_aoutput.c i_ni_daqmx.h \
			d_ni_daqmx_aoutput.h
	$(COMPILE) $(CFLAGS) d_ni_daqmx_aoutput.c

d_ni_daqmx_dinput.$(OBJ): d_ni_daqmx_dinput.c i_ni_daqmx.h \
			d_ni_daqmx_dinput.h
	$(COMPILE) $(CFLAGS) d_ni_daqmx_dinput.c

d_ni_daqmx_doutput.$(OBJ): d_ni_daqmx_doutput.c i_ni_daqmx.h \
			d_ni_daqmx_doutput.h
	$(COMPILE) $(CFLAGS) d_ni_daqmx_doutput.c

install: ni_daqmx.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp ni_daqmx.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

