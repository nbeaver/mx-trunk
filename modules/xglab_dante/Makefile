#
# Supported values of MX_ARCH: linux win32
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

XGLAB_DANTE_VERSION   = 3.7.26.0

$(info XGLAB_DANTE_VERSION is [${XGLAB_DANTE_VERSION}])

ifeq ($(MX_ARCH),linux)
XGLAB_DANTE_VERSION_2 = 8.3.0

$(info XGLAB_DANTE_VERSION_2 is [${XGLAB_DANTE_VERSION_2}])
endif

ifeq ($(MX_ARCH),linux)
	XGLAB_DANTE_DIR = /opt/DANTE/$(XGLAB_DANTE_VERSION)/Library/Linux/$(XGLAB_DANTE_VERSION)-$(XGLAB_DANTE_VERSION_2)-Callback-x64

	XGLAB_DANTE_INCLUDES = -I$(XGLAB_DANTE_DIR)
	XGLAB_DANTE_LIBS = -L$(XGLAB_DANTE_DIR) -lXGL_DPP
endif

ifeq ($(MX_ARCH),win32)
	XGLAB_DANTE_DIR = "C:\\opt\\DANTE\\$(XGLAB_DANTE_VERSION)\\Library\\Windows\\$(XGLAB_DANTE_VERSION)-Callback-x64"

	XGLAB_DANTE_INCLUDES = -I$(XGLAB_DANTE_DIR)
	XGLAB_DANTE_LIBS = $(XGLAB_DANTE_DIR)\\Release\\XGL_DPP.lib
endif

#
# Override the XGLAB_DANTE_... definitions if using a vendored copy
# of the xglab DANTE library from somebody else's package and the copy
# does not use the same directory layout as the xglab version.
#
# At present, this only comes up if you are using the copy of the
# xglab DANTE library which is bundled with the EPICS Dante app.
#
# You should probably use the version from xglab unless you have a
# specific reason not to.
#

ifeq (0,1)
	XGLAB_DANTE_DIR = C:\\opt\\epics\support\\Dante\\danteApp\\danteSupport

	XGLAB_DANTE_INCLUDES = -I$(XGLAB_DANTE_DIR)
	XGLAB_DANTE_LIBS = $(XGLAB_DANTE_DIR)\\os\\WIN32\\x64\\XGL_DPP.lib
endif

$(info XGLAB_DANTE_DIR is [${XGLAB_DANTE_DIR}])
$(info XGLAB_DANTE_INCLUDES is [${XGLAB_DANTE_INCLUDES}])
$(info XGLAB_DANTE_LIBS is [${XGLAB_DANTE_LIBS}])

#--------------------------------------------------------------------------

XGLAB_DANTE_MXO = xglab_dante.mxo

all: $(XGLAB_DANTE_MXO)

include ../../libMx/Makehead.$(MX_ARCH)

ifeq ($(MX_ARCH),linux)
	CFLAGS = $(GCCFLAGS) -DOS_LINUX $(XGLAB_DANTE_INCLUDES)
	C_COMPILE = gcc -c -fPIC -DDEBUG
	CPP_COMPILE = g++ -c -fPIC -DDEBUG
endif
ifeq ($(MX_ARCH),win32)
	CFLAGS = -nologo -Zi -WX -DOS_WIN32 $(XGLAB_DANTE_INCLUDES)
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
		$(XGLAB_DANTE_LIBS)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

$(XGLAB_DANTE_MXO): $(XGLAB_DANTE_OBJS)
	link /dll /debug /nologo /out:$(XGLAB_DANTE_MXO) /nodefaultlib:libc \
		$(XGLAB_DANTE_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(XGLAB_DANTE_LIBS)

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

i_dante.$(OBJ): i_dante.cpp i_dante.h mx_dll_dpp_callback.h
	$(CPP_COMPILE) $(CFLAGS) i_dante.cpp

d_dante_mca.$(OBJ): d_dante_mca.cpp i_dante.h d_dante_mca.h d_dante_mcs.h \
							mx_dll_dpp_callback.h
	$(CPP_COMPILE) $(CFLAGS) d_dante_mca.cpp

d_dante_mcs.$(OBJ): d_dante_mcs.cpp i_dante.h d_dante_mca.h d_dante_mcs.h \
							mx_dll_dpp_callback.h
	$(CPP_COMPILE) $(CFLAGS) d_dante_mcs.cpp

install: $(XGLAB_DANTE_MXO)
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp $(XGLAB_DANTE_MXO) $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

