#
# This module is only for the 'win32' platform.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

all: radicon_taurus.mxo

include ../../libMx/Makehead.$(MX_ARCH)

ifeq ($(MX_ARCH),win32)
  CFLAGS += -EHsc
endif

CFLAGS += -I../../libMx -D__MX_LIBRARY__

RADICON_TAURUS_OBJS = radicon_taurus.$(OBJ) d_radicon_taurus.$(OBJ) \
		i_radicon_taurus_rs232.$(OBJ)

ifeq ($(MX_ARCH),win32)
    radicon_taurus.mxo: $(RADICON_TAURUS_OBJS)
	link /dll /debug /nologo /out:radicon_taurus.mxo /nodefaultlib:libc \
		$(RADICON_TAURUS_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f radicon_taurus.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:radicon_taurus.mxo\;2 \
			-manifest radicon_taurus.mxo.manifest ; \
		rm -f radicon_taurus.mxo.manifest; \
	fi
endif

ifeq ($(MX_ARCH),linux)
    radicon_taurus.mxo: $(RADICON_TAURUS_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,radicon_taurus.mxo \
		-o radicon_taurus.mxo $(RADICON_TAURUS_OBJS)
endif

radicon_taurus.$(OBJ): radicon_taurus.c d_radicon_taurus.h \
						i_radicon_taurus_rs232.h
	$(COMPILE) $(CFLAGS) radicon_taurus.c

d_radicon_taurus.$(OBJ): d_radicon_taurus.c d_radicon_taurus.h \
		../../libMx/mx_area_detector_rdi.h
	$(COMPILE) $(CFLAGS) d_radicon_taurus.c

i_radicon_taurus_rs232.$(OBJ): i_radicon_taurus_rs232.c i_radicon_taurus_rs232.h
	$(COMPILE) $(CFLAGS) i_radicon_taurus_rs232.c

install: radicon_taurus.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp radicon_taurus.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

