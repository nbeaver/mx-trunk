#
# This module is only for the 'win32' platform.
#

MX_ARCH = win32

U500_DIR = c:\\U500\\toolkit
#U500_DIR = c:\\U500\\mmi

all: u500.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(U500_DIR)\\include

U500_OBJS = u500.$(OBJ) d_u500.$(OBJ) d_u500_status.$(OBJ) \
	i_u500.$(OBJ) i_u500_rs232.$(OBJ) v_u500.$(OBJ)

u500.mxo: $(U500_OBJS)
	link /dll /debug /nologo /out:u500.mxo /nodefaultlib:libc \
		$(U500_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(U500_DIR)\\Win50032.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f u500.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:u500.mxo\;2 \
			-manifest u500.mxo.manifest ; \
		rm -f u500.mxo.manifest; \
	fi

u500.$(OBJ): u500.c d_u500.h
	$(COMPILE) $(CFLAGS) u500.c

d_u500.$(OBJ): d_u500.c d_u500.h
	$(COMPILE) $(CFLAGS) d_u500.c

d_u500_status.$(OBJ): d_u500_status.c d_u500_status.h
	$(COMPILE) $(CFLAGS) d_u500_status.c

i_u500.$(OBJ): i_u500.c i_u500.h
	$(COMPILE) $(CFLAGS) i_u500.c

i_u500_rs232.$(OBJ): i_u500_rs232.c i_u500_rs232.h
	$(COMPILE) $(CFLAGS) i_u500_rs232.c

v_u500.$(OBJ): v_u500.c v_u500.h
	$(COMPILE) $(CFLAGS) v_u500.c

install: u500.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp u500.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

