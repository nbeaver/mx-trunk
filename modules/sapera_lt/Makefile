#
# This modules is only for the 'win32' platform.
#

MX_ARCH = win32

SAPERA_LT_DIR = c:\\DALSA\\Sapera

all: sapera_lt.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(SAPERA_LT_DIR)\\Classes\\Basic
CFLAGS += -I$(SAPERA_LT_DIR)\\Include

SAPERA_LT_OBJS = sapera_lt.$(OBJ) \
	i_sapera_lt.$(OBJ) d_sapera_lt_frame_grabber.$(OBJ)

sapera_lt.mxo: $(SAPERA_LT_OBJS)
	link /dll /debug /nologo /out:sapera_lt.mxo /nodefaultlib:libc \
		$(SAPERA_LT_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(SAPERA_LT_DIR)\\Lib\\SapClassBasic.lib \
		$(SAPERA_LT_DIR)\\Lib\\corapi.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f sapera_lt.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:sapera_lt.mxo\;2 \
			-manifest sapera_lt.mxo.manifest ; \
		rm -f sapera_lt.mxo.manifest; \
	fi

sapera_lt.$(OBJ): sapera_lt.c i_sapera_lt.h d_sapera_lt_frame_grabber.h
	$(COMPILE) $(CFLAGS) sapera_lt.c

i_sapera_lt.$(OBJ): i_sapera_lt.cpp i_sapera_lt.h
	$(COMPILE) $(CFLAGS) i_sapera_lt.cpp

d_sapera_lt_frame_grabber.$(OBJ): d_sapera_lt_frame_grabber.cpp \
				d_sapera_lt_frame_grabber.h i_sapera_lt.h
	$(COMPILE) $(CFLAGS) d_sapera_lt_frame_grabber.cpp

install: sapera_lt.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp sapera_lt.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


