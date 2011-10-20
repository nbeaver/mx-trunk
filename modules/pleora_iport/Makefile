#
# This module can only be used on the 'win32' platform.  It _might_ be
# possible to get 'win32-mingw' to work, but this has not been tested.
#

MX_ARCH = win32

PLEORA_IPORT_DIR = c:\\progra~1\\pleora~1\\ebus-v~1
#PLEORA_IPORT_DIR = c:\\progra~2\\pleora~1\\ebus-v~1
#PLEORA_IPORT_DIR = d:\\progra~1\\pleora~1\\ebus-v~1

all: pleora_iport.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(PLEORA_IPORT_DIR)\\Includes

PLEORA_IPORT_OBJS = pleora_iport.$(OBJ) \
		i_pleora_iport.$(OBJ) d_pleora_iport_vinput.$(OBJ) \
		d_pleora_iport_dio.$(OBJ) 

pleora_iport.mxo: $(PLEORA_IPORT_OBJS)
	link /dll /debug /nologo /out:pleora_iport.mxo /nodefaultlib:libc \
		$(PLEORA_IPORT_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyComLib.lib \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyCamLib.lib \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyImgLib.lib \
		$(PLEORA_IPORT_DIR)\\Libraries\\CyUtilsLib.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f pleora_iport.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:pleora_iport.mxo\;2 \
			-manifest pleora_iport.mxo.manifest ; \
		rm -f pleora_iport.mxo.manifest; \
	fi

pleora_iport.$(OBJ): pleora_iport.c i_pleora_iport.h d_pleora_iport_vinput.h
	$(COMPILE) $(CFLAGS) pleora_iport.c

i_pleora_iport.$(OBJ): i_pleora_iport.cpp i_pleora_iport.h
	$(COMPILE) $(CFLAGS) i_pleora_iport.cpp

d_pleora_iport_vinput.$(OBJ): d_pleora_iport_vinput.cpp \
				d_pleora_iport_vinput.h i_pleora_iport.h
	$(COMPILE) $(CFLAGS) d_pleora_iport_vinput.cpp

d_pleora_iport_dio.$(OBJ): d_pleora_iport_dio.cpp d_pleora_iport_dio.h \
				d_pleora_iport_vinput.h i_pleora_iport.h
	$(COMPILE) $(CFLAGS) d_pleora_iport_dio.cpp

install: pleora_iport.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp pleora_iport.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

