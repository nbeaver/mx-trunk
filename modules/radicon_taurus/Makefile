#
# This module is only for the 'win32' platform.
#

MX_ARCH = win32

all: radicon_taurus.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__

RADICON_TAURUS_OBJS = radicon_taurus.$(OBJ) d_radicon_taurus.$(OBJ)

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

radicon_taurus.$(OBJ): radicon_taurus.c d_radicon_taurus.h
	$(COMPILE) $(CFLAGS) radicon_taurus.c

d_radicon_taurus.$(OBJ): d_radicon_taurus.c d_radicon_taurus.h \
		..\pleora_iport\i_pleora_iport.h \
		..\pleora_iport\d_pleora_iport_vinput.h
	$(COMPILE) $(CFLAGS) d_radicon_taurus.c

install: radicon_taurus.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp radicon_taurus.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

