#
# Win32 driver for the EG&G Ortec Unified MCB Interface for 32 Bits
# programmer's toolkit (Part # A11-B32) under Microsoft Windows 95.
# Note that these driver does not support the 16 bit version of the MCB
# interface software.
#
# The 'ortec_umcbi' module is only for the 'win32' platform.
#

MX_ARCH = win32

UMCBI_DIR       = c:\\Ortec\\Umcbi
UMCBI_INCLUDES  = -I$(UMCBI_DIR)
UMCBI_LIBS      = $(UMCBI_DIR)\\Mcbcio32.lib

all: ortec_umcbi.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += $(UMCBI_INCLUDES)

ORTEC_UMCBI_OBJS = ortec_umcbi.$(OBJ) d_trump.$(OBJ) i_umcbi.$(OBJ)

ortec_umcbi.mxo: $(ORTEC_UMCBI_OBJS)
	link /dll /debug /nologo /out:ortec_umcbi.mxo /nodefaultlib:libc \
		$(ORTEC_UMCBI_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(UMCBI_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f ortec_umcbi.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:ortec_umcbi.mxo\;2 \
			-manifest ortec_umcbi.mxo.manifest ; \
		rm -f ortec_umcbi.mxo.manifest; \
	fi

ortec_umcbi.$(OBJ): ortec_umcbi.c d_trump.h
	$(COMPILE) $(CFLAGS) ortec_umcbi.c

d_trump.$(OBJ): d_trump.c d_trump.h
	$(COMPILE) $(CFLAGS) d_trump.c

i_umcbi.$(OBJ): i_umcbi.c i_umcbi.h
	$(COMPILE) $(CFLAGS) i_umcbi.c

install: ortec_umcbi.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp ortec_umcbi.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

