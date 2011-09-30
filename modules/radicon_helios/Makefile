#
# This module can only be used on the 'win32' platform.  It _might_ be
# possible to get 'win32-mingw' to work, but this has not been tested.
#

MX_ARCH = win32

#PLEORA_IPORT_DIR = c:\\progra~2\\pleora~1\\ebus-v~1
PLEORA_IPORT_DIR = d:\\progra~1\\pleora~1\\ebus-v~1

all: radicon_helios.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I../pleora_iport
CFLAGS += -I$(PLEORA_IPORT_DIR)\\includes
CFLAGS += -I../epics -I../epics_mbc

RADICON_HELIOS_OBJS = radicon_helios.$(OBJ) d_radicon_helios.$(OBJ) \
		d_radicon_helios_trigger.$(OBJ)

radicon_helios.mxo: $(RADICON_HELIOS_OBJS)
	link /dll /debug /nologo /out:radicon_helios.mxo /nodefaultlib:libc \
		$(RADICON_HELIOS_OBJS) \
		..\\pleora_iport\\pleora_iport.lib \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyComLib.lib \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyCamLib.lib \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyImgLib.lib \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyUtilsLib.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f radicon_helios.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:radicon_helios.mxo\;2 \
			-manifest radicon_helios.mxo.manifest ; \
		rm -f radicon_helios.mxo.manifest; \
	fi

radicon_helios.$(OBJ): radicon_helios.c d_radicon_helios.h
	$(COMPILE) $(CFLAGS) radicon_helios.c

d_radicon_helios.$(OBJ): d_radicon_helios.cpp d_radicon_helios.h \
		..\pleora_iport\i_pleora_iport.h \
		..\pleora_iport\d_pleora_iport_vinput.h
	$(COMPILE) $(CFLAGS) d_radicon_helios.cpp

d_radicon_helios_trigger.$(OBJ): d_radicon_helios_trigger.cpp \
				d_radicon_helios_trigger.h \
				..\pleora_iport\i_pleora_iport.h \
				..\pleora_iport\d_pleora_iport_vinput.h
	$(COMPILE) $(CFLAGS) d_radicon_helios_trigger.cpp

install: radicon_helios.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp radicon_helios.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

