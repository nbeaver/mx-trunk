#
# This module is only for the 'win32' platform.
#

MX_ARCH = win32

all: radicon_xineos_gige.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__

RADICON_XINEOS_GIGE_OBJS = radicon_xineos_gige.$(OBJ) \
			d_radicon_xineos_gige.$(OBJ)

radicon_xineos_gige.mxo: $(RADICON_XINEOS_GIGE_OBJS)
	link /dll /debug /nologo /out:radicon_xineos_gige.mxo \
		/nodefaultlib:libc \
		$(RADICON_XINEOS_GIGE_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f radicon_xineos_gige.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:radicon_xineos_gige.mxo\;2 \
			-manifest radicon_xineos_gige.mxo.manifest ; \
		rm -f radicon_xineos_gige.mxo.manifest; \
	fi

radicon_xineos_gige.$(OBJ): radicon_xineos_gige.c d_radicon_xineos_gige.h
	$(COMPILE) $(CFLAGS) radicon_xineos_gige.c

d_radicon_xineos_gige.$(OBJ): d_radicon_xineos_gige.c d_radicon_xineos_gige.h
	$(COMPILE) $(CFLAGS) d_radicon_xineos_gige.c

install: radicon_xineos_gige.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp radicon_xineos_gige.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

