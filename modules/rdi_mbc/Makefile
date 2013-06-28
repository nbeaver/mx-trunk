#
# This module contains drivers for Molecular Biology Consortium-specific
# addtions for RDI detectors.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

#--------------------------------------------------------------------------

all: rdi_mbc.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER

RDI_MBC_OBJS = rdi_mbc.$(OBJ) v_rdi_mbc_string.$(OBJ)

#--------------------------------------------------------------------------

# Windows

ifeq ($(MX_ARCH),win32)

rdi_mbc.mxo: $(RDI_MBC_OBJS)
	link /dll /debug /nologo /out:rdi_mbc.mxo /nodefaultlib:libc \
		$(RDI_MBC_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f rdi_mbc.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:rdi_mbc.mxo\;2 \
			-manifest rdi_mbc.mxo.manifest ; \
		rm -f rdi_mbc.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

rdi_mbc.$(OBJ): rdi_mbc.c v_rdi_mbc_string.h 
	$(COMPILE) $(CFLAGS) rdi_mbc.c

v_rdi_mbc_string.$(OBJ): v_rdi_mbc_string.c v_rdi_mbc_string.h
	$(COMPILE) $(CFLAGS) v_rdi_mbc_string.c

install: rdi_mbc.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp rdi_mbc.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

