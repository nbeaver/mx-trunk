#
# This module is only for the 'linux' and 'win32' platforms.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

all: aviex_pccd.mxo

include ../../libMx/Makehead.$(MX_ARCH)

ifeq ($(MX_ARCH),win32)
  CFLAGS += -EHsc
endif

CFLAGS += -I../../libMx -D__MX_LIBRARY__
AVIEX_OBJS = aviex_pccd.$(OBJ) d_aviex_pccd.$(OBJ) \
		d_aviex_pccd_16080.$(OBJ) d_aviex_pccd_170170.$(OBJ) \
		d_aviex_pccd_4824.$(OBJ) d_aviex_pccd_9785.$(OBJ) \
		../../tools/smvspatial.$(OBJ)

ifeq ($(MX_ARCH),win32)
    aviex_pccd.mxo: $(AVIEX_OBJS)
	link /dll /debug /nologo /out:aviex_pccd.mxo \
		/nodefaultlib:libc \
		$(AVIEX_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f aviex_pccd.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo \
			-outputresource:aviex_pccd.mxo\;2 \
			-manifest aviex_pccd.mxo.manifest ; \
		rm -f aviex_pccd.mxo.manifest; \
	fi
endif

ifeq ($(MX_ARCH),linux)
    aviex_pccd.mxo: $(AVIEX_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,aviex_pccd.mxo \
		-o aviex_pccd.mxo $(AVIEX_OBJS)
endif

d_aviex_pccd.h: d_aviex_pccd_16080.h d_aviex_pccd_170170.h \
		d_aviex_pccd_4824.h d_aviex_pccd_9785.h

aviex_pccd.$(OBJ): aviex_pccd.c d_aviex_pccd.h
	$(COMPILE) $(CFLAGS) aviex_pccd.c

d_aviex_pccd.$(OBJ): d_aviex_pccd.c d_aviex_pccd.h
	$(COMPILE) $(CFLAGS) d_aviex_pccd.c

d_aviex_pccd_16080.$(OBJ): d_aviex_pccd_16080.c \
				d_aviex_pccd.h d_aviex_pccd_16080.h
	$(COMPILE) $(CFLAGS) d_aviex_pccd_16080.c

d_aviex_pccd_170170.$(OBJ): d_aviex_pccd_170170.c \
				d_aviex_pccd.h d_aviex_pccd_170170.h
	$(COMPILE) $(CFLAGS) d_aviex_pccd_170170.c

d_aviex_pccd_4824.$(OBJ): d_aviex_pccd_4824.c \
				d_aviex_pccd.h d_aviex_pccd_4824.h
	$(COMPILE) $(CFLAGS) d_aviex_pccd_4824.c

d_aviex_pccd_9785.$(OBJ): d_aviex_pccd_9785.c \
				d_aviex_pccd.h d_aviex_pccd_9785.h
	$(COMPILE) $(CFLAGS) d_aviex_pccd_9785.c

install: aviex_pccd.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp aviex_pccd.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

