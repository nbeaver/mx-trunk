#
# This module is only for the 'win32' platform.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

all: xineos_gige.mxo

include ../../libMx/Makehead.$(MX_ARCH)

ifeq ($(MX_ARCH),win32)
  CFLAGS += -EHsc
endif

CFLAGS += -I../../libMx -D__MX_LIBRARY__

XINEOS_GIGE_OBJS = xineos_gige.$(OBJ) d_xineos_gige.$(OBJ)

ifeq ($(MX_ARCH),win32)
    xineos_gige.mxo: $(XINEOS_GIGE_OBJS)
	link /dll /debug /nologo /out:xineos_gige.mxo \
		/nodefaultlib:libc \
		$(XINEOS_GIGE_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f xineos_gige.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:xineos_gige.mxo\;2 \
			-manifest xineos_gige.mxo.manifest ; \
		rm -f xineos_gige.mxo.manifest; \
	fi
endif

ifeq ($(MX_ARCH),linux)
    xineos_gige.mxo: $(XINEOS_GIGE_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,xineos_gige.mxo \
		-o xineos_gige.mxo $(XINEOS_GIGE_OBJS)
endif

xineos_gige.$(OBJ): xineos_gige.c d_xineos_gige.h
	$(COMPILE) $(CFLAGS) xineos_gige.c

d_xineos_gige.$(OBJ): d_xineos_gige.c d_xineos_gige.h
	$(COMPILE) $(CFLAGS) d_xineos_gige.c

install: xineos_gige.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp xineos_gige.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

