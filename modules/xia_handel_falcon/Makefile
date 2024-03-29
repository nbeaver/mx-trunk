#
# Supported values of MX_ARCH: linux win32
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

ifeq ($(MX_ARCH),linux)
	XIA_HANDEL_INCLUDES = /usr/local/src/handel-falcon-1.1.22/inc

	CFLAGS += -I$(XIA_HANDEL_INCLUDES)
endif

ifeq ($(MX_ARCH),linux-clang)
	XIA_HANDEL_DIR = /opt/handel

	CFLAGS += -I$(XIA_HANDEL_DIR)/inc
endif

ifeq ($(MX_ARCH),win32)
	XIA_HANDEL_DIR = c:\\opt\\handel-1.2.22
	#
	#XIA_HANDEL_DIR = c:\\progra~1\\XIA\\xmanag~1.1\\sdk
	#
	#XIA_HANDEL_DIR = c:\\progra~2\\XIA\\xmanag~1.3\\sdk
	#
	#XIA_HANDEL_DIR = c:\\progra~2\\XIA\\prospe~1.1\\sdk

	CFLAGS += -I$(XIA_HANDEL_DIR)\\inc
endif

ifeq ($(MX_ARCH),win32-mingw)
	XIA_HANDEL_DIR = c:/progra~1/XIA/xmanag~1.0/sdk

	CFLAGS += -I$(XIA_HANDEL_DIR)/inc -Wno-unused-variable
endif

CFLAGS += -DIS_MX_DRIVER

#--------------------------------------------------------------------------

XIA_HANDEL_MXO = xia_handel_falcon.mxo

all: $(XIA_HANDEL_MXO)

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

XIA_HANDEL_OBJS = xia_handel.$(OBJ) i_handel.$(OBJ) \
		d_handel_input.$(OBJ) d_handel_mapping_pixel_next.$(OBJ) \
		d_handel_mca.$(OBJ) d_handel_mcs.$(OBJ) \
		d_handel_sum.$(OBJ) d_handel_timer.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

$(XIA_HANDEL_MXO): $(XIA_HANDEL_OBJS)
	gcc -shared -rdynamic -Wl,-soname,$(XIA_HANDEL_MXO) \
		-o $(XIA_HANDEL_MXO) $(XIA_HANDEL_OBJS) \
		-L$(XIA_HANDEL_DIR)/lib -lhandel -lusb

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux-clang)

$(XIA_HANDEL_MXO): $(XIA_HANDEL_OBJS)
	gcc -shared -rdynamic -Wl,-soname,$(XIA_HANDEL_MXO) \
		-o $(XIA_HANDEL_MXO) $(XIA_HANDEL_OBJS) \
		-L$(XIA_HANDEL_DIR)/lib -lhandel -lusb

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

$(XIA_HANDEL_MXO): $(XIA_HANDEL_OBJS)
	link /dll /debug /nologo /out:$(XIA_HANDEL_MXO) /nodefaultlib:libc \
		$(XIA_HANDEL_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(XIA_HANDEL_DIR)\\lib\\handel.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f $(XIA_HANDEL_MXO).manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:$(XIA_HANDEL_MXO)\;2 \
			-manifest $(XIA_HANDEL_MXO).manifest ; \
		rm -f $(XIA_HANDEL_MXO).manifest; \
	fi

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32-mingw)

$(XIA_HANDEL_MXO): $(XIA_HANDEL_OBJS)
	gcc -shared -o $(XIA_HANDEL_MXO) $(XIA_HANDEL_OBJS) \
		../../libMx/$(MX_LIBRARY_NAME) \
		-L$(XIA_HANDEL_DIR)/lib -lhandel -lusb

endif

#--------------------------------------------------------------------------

xia_handel.$(OBJ): xia_handel.c i_handel.h \
			d_handel_input.h d_handel_mca.h \
			d_handel_mcs.h d_handel_sum.h \
			d_handel_timer.h
	$(COMPILE) $(CFLAGS) xia_handel.c

i_handel.$(OBJ): i_handel.c i_handel.h d_handel_mca.h d_handel_mcs.h
	$(COMPILE) $(CFLAGS) i_handel.c

d_handel_input.$(OBJ): d_handel_input.c d_handel_input.h \
				i_handel.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) d_handel_input.c

d_handel_mca.$(OBJ): d_handel_mca.c i_handel.h d_handel_mca.h d_handel_mcs.h
	$(COMPILE) $(CFLAGS) d_handel_mca.c

d_handel_mcs.$(OBJ): d_handel_mcs.c i_handel.h d_handel_mcs.h d_handel_mca.h
	$(COMPILE) $(CFLAGS) d_handel_mcs.c

d_handel_sum.$(OBJ): d_handel_sum.c d_handel_sum.h d_handel_mcs.h
	$(COMPILE) $(CFLAGS) d_handel_sum.c

d_handel_timer.$(OBJ): d_handel_timer.c d_handel_timer.h \
				i_handel.h d_handel_mca.h \
				d_handel_mcs.h
	$(COMPILE) $(CFLAGS) d_handel_timer.c

install: $(XIA_HANDEL_MXO)
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp $(XIA_HANDEL_MXO) $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

