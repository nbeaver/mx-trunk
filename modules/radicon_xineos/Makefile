#
# This module is only for the 'win32' platform.
#

MX_ARCH = win32

all: radicon_xineos.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__

RADICON_TAURUS_OBJS = radicon_xineos.$(OBJ) d_radicon_xineos.$(OBJ)

radicon_xineos.mxo: $(RADICON_TAURUS_OBJS)
	link /dll /debug /nologo /out:radicon_xineos.mxo /nodefaultlib:libc \
		$(RADICON_TAURUS_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f radicon_xineos.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:radicon_xineos.mxo\;2 \
			-manifest radicon_xineos.mxo.manifest ; \
		rm -f radicon_xineos.mxo.manifest; \
	fi

radicon_xineos.$(OBJ): radicon_xineos.c d_radicon_xineos.h
	$(COMPILE) $(CFLAGS) radicon_xineos.c

d_radicon_xineos.$(OBJ): d_radicon_xineos.c d_radicon_xineos.h \
		..\pleora_iport\i_pleora_iport.h \
		..\pleora_iport\d_pleora_iport_vinput.h
	$(COMPILE) $(CFLAGS) d_radicon_xineos.c

install: radicon_xineos.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp radicon_xineos.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

