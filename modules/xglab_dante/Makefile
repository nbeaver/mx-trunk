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
	XGLAB_DANTE_DIR = /opt/DANTE/Library/Linux-gcc4.8.5-libc2.17-x64/3.4.0.3-4.8.5-Callback-x64
endif

ifeq ($(MX_ARCH),win32)
#	XGLAB_DANTE_DIR = "c:\\opt\\DANTE\\3.2.4.3\\Library\\Callback-x64"
#	XGLAB_DANTE_DIR = "c:\\opt\\DANTE\\3.4.0.3\\Library\\Callback-x64"

	XGLAB_DANTE_DIR = "c:\\opt\\DANTE\\3.4.0.3\\Library\\micros~1\\3.4.0.3-Callback-x64"
endif

#--------------------------------------------------------------------------

XGLAB_DANTE_MXO = xglab_dante.mxo

all: $(XGLAB_DANTE_MXO)

include ../../libMx/Makehead.$(MX_ARCH)

ifeq ($(MX_ARCH),linux)
	CFLAGS = $(GCCFLAGS) -DOS_LINUX -I$(XGLAB_DANTE_DIR)
	C_COMPILE = gcc -c -fPIC -DDEBUG
	CPP_COMPILE = g++ -c -fPIC -DDEBUG
endif
ifeq ($(MX_ARCH),win32)
	CFLAGS = -nologo -Zi -WX -DOS_WIN32 -I$(XGLAB_DANTE_DIR) -DDEBUG
	C_COMPILE = cl -c -DDEBUG
	CPP_COMPILE = cl -c -DDEBUG
endif

CFLAGS += -DIS_MX_DRIVER

CFLAGS += -I../../libMx -D__MX_LIBRARY__

XGLAB_DANTE_OBJS = xglab_dante.$(OBJ) i_dante.$(OBJ) \
		d_dante_mca.$(OBJ) d_dante_mcs.$(OBJ)

$(info $$C_COMPILE is [${C_COMPILE}])
$(info $$CFLAGS is [${CFLAGS}])
$(info $$XGLAB_DANTE_DIR is [${XGLAB_DANTE_DIR}])

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

$(XGLAB_DANTE_MXO): $(XGLAB_DANTE_OBJS)
	gcc -shared -rdynamic -Wl,-soname,$(XGLAB_DANTE_MXO) \
		-o $(XGLAB_DANTE_MXO) $(XGLAB_DANTE_OBJS) \
		-L$(XGLAB_DANTE_DIR) -lXGL_DPP

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

$(XGLAB_DANTE_MXO): $(XGLAB_DANTE_OBJS)
	link /dll /debug /nologo /out:$(XGLAB_DANTE_MXO) /nodefaultlib:libc \
		$(XGLAB_DANTE_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(XGLAB_DANTE_DIR)\\Release\\XGL_DPP.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f $(XGLAB_DANTE_MXO).manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:$(XGLAB_DANTE_MXO)\;2 \
			-manifest $(XGLAB_DANTE_MXO).manifest ; \
		rm -f $(XGLAB_DANTE_MXO).manifest; \
	fi

endif

#--------------------------------------------------------------------------

xglab_dante.$(OBJ): xglab_dante.c i_dante.h d_dante_mca.h d_dante_mcs.h
	$(C_COMPILE) $(CFLAGS) xglab_dante.c

i_dante.$(OBJ): i_dante.cpp i_dante.h
	$(CPP_COMPILE) $(CFLAGS) i_dante.cpp

d_dante_mca.$(OBJ): d_dante_mca.cpp i_dante.h d_dante_mca.h d_dante_mcs.h
	$(CPP_COMPILE) $(CFLAGS) d_dante_mca.cpp

d_dante_mcs.$(OBJ): d_dante_mcs.cpp i_dante.h d_dante_mca.h d_dante_mcs.h
	$(CPP_COMPILE) $(CFLAGS) d_dante_mcs.cpp

install: $(XGLAB_DANTE_MXO)
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp $(XGLAB_DANTE_MXO) $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

