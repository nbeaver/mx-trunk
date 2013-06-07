#
# This module is only for the 'win32' platform.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

all: rdi_api.mxo

include ../../libMx/Makehead.$(MX_ARCH)

ifeq ($(MX_ARCH),win32)
  CFLAGS += -EHsc
endif

CFLAGS += -I../../libMx -D__MX_LIBRARY__

RDI_API_OBJS = rdi_api.$(OBJ) z_rdi_api.$(OBJ)

ifeq ($(MX_ARCH),win32)
    rdi_api.mxo: $(RDI_API_OBJS)
	link /dll /debug /nologo /out:rdi_api.mxo /nodefaultlib:libc \
		$(RDI_API_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f rdi_api.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:rdi_api.mxo\;2 \
			-manifest rdi_api.mxo.manifest ; \
		rm -f rdi_api.mxo.manifest; \
	fi
endif

ifeq ($(MX_ARCH),linux)
    rdi_api.mxo: $(RDI_API_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,rdi_api.mxo \
		-o rdi_api.mxo $(RDI_API_OBJS)
endif

rdi_api.$(OBJ): rdi_api.c z_rdi_api.h
	$(COMPILE) $(CFLAGS) rdi_api.c

z_rdi_api.$(OBJ): z_rdi_api.c z_rdi_api.h
	$(COMPILE) $(CFLAGS) z_rdi_api.c

install: rdi_api.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp rdi_api.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

