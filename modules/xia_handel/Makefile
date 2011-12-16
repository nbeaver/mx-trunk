#
# Supported values of MX_ARCH:  win32
#

CFLAGS += -DHAVE_XIA_HANDEL

ifndef MX_ARCH
  MX_ARCH = win32
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

ifeq ($(MX_ARCH),linux)
	XIA_HANDEL_DIR = /opt/xia/handel

	CFLAGS += -I$(XIA_HANDEL_DIR)/inc
endif

ifeq ($(MX_ARCH),win32)
	XIA_HANDEL_DIR = c:\\progra~1\\XIA\\xmanag~1.1\\sdk

	CFLAGS += -I$(XIA_HANDEL_DIR)\\inc
endif

CFLAGS += -DIS_MX_DRIVER

#--------------------------------------------------------------------------

all: xia_handel.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

XIA_HANDEL_OBJS = xia_handel.$(OBJ) i_handel.$(OBJ) i_handel_network.$(OBJ) \
		d_handel_input.$(OBJ) d_handel_mca.$(OBJ) \
		d_handel_sum.$(OBJ) d_handel_timer.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

xia_handel.mxo: $(XIA_HANDEL_OBJS)
	gcc -shared -rdynamic -Wl,-soname,xia_handel.mxo \
		-o xia_handel.mxo $(XIA_HANDEL_OBJS) \
		-L$(XIA_HANDEL_DIR)/lib -lhandel

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

xia_handel.mxo: $(XIA_HANDEL_OBJS)
	link /dll /debug /nologo /out:xia_handel.mxo /nodefaultlib:libc \
		$(XIA_HANDEL_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(XIA_HANDEL_DIR)\\lib\\handel.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f xia_handel.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:xia_handel.mxo\;2 \
			-manifest xia_handel.mxo.manifest ; \
		rm -f xia_handel.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

xia_handel.$(OBJ): xia_handel.c i_handel.h i_handel_network.h \
			d_handel_input.h d_handel_mca.h \
			d_handel_sum.h d_handel_timer.h
	$(COMPILE) $(CFLAGS) xia_handel.c

i_handel.$(OBJ): i_handel.c i_handel.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) i_handel.c

i_handel_network.$(OBJ): i_handel_network.c i_handel_network.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) i_handel_network.c

d_handel_input.$(OBJ): d_handel_input.c d_handel_input.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) d_handel_input.c

d_handel_mca.$(OBJ): d_handel_mca.c i_handel.h i_handel_network.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) d_handel_mca.c

d_handel_sum.$(OBJ): d_handel_sum.c d_handel_sum.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) d_handel_sum.c

d_handel_timer.$(OBJ): d_handel_timer.c d_handel_timer.h \
			i_handel.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) d_handel_timer.c

install: xia_handel.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp xia_handel.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

