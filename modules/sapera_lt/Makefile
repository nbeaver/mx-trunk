#
# This modules is only for the 'win32' and 'linux' platforms.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

ifeq ($(MX_ARCH),linux)
	SAPERA_LT_INCLUDES  = -I/usr/local/include/Sapera/classes/basic
	SAPERA_LT_INCLUDES += -I/usr/local/include/Sapera/include

	SAPERA_LT_LIBRARIES = -L/usr/local/lib -lSapera++ -lSaperaLT
endif

ifeq ($(MX_ARCH),win32)
	#
	# Note: The directory name SAPERA_LT_DIR must _not_ have any spaces
	# in it.  This means names like "C:\\Program Files\\Teledyne DALSA"
	# are not acceptable.
	#

	SAPERA_LT_DIR = c:\\progra~1\\teledy~1\\Sapera

	SAPERA_LT_INCLUDES = -I$(SAPERA_LT_DIR)\\Classes\\Basic
	SAPERA_LT_INCLUDES += -I$(SAPERA_LT_DIR)\\Include

	#
	# For Sapera LT 7.2 use this:
	#
	##SAPERA_LT_LIBRARIES = $(SAPERA_LT_DIR)\\Lib\\SapClassBasic.lib \
	##			$(SAPERA_LT_DIR)\\Lib\\corapi.lib

	#
	# For Sapera LT 8.0 use this.  Select either Win64 or Win32 as needed.
	#
	SAPERA_LT_LIBRARIES = $(SAPERA_LT_DIR)\\Lib\\Win64\\SapClassBasic.lib \
			$(SAPERA_LT_DIR)\\Lib\\Win64\\corapi.lib
endif

#--------------------------------------------------------------------------

all: sapera_lt.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS_MODULE = -I../../libMx -D__MX_LIBRARY__ $(SAPERA_LT_INCLUDES)

SAPERA_LT_OBJS = sapera_lt.$(OBJ) i_sapera_lt.$(OBJ) \
			d_sapera_lt_frame_grabber.$(OBJ) \
			d_sapera_lt_camera.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

#
# The vendor include files make a lot of use of functions that are forbidden
# in the core of MX like sprintf(), strcpy(), and so forth.  We must define
# MX_NO_POISON to relax that restriction.
#
# POSIX_HOSTPC must be defined for Linux builds.  If it is not, then the
# Sapera include files appear to assume that we are compiling on Windows
# and start decorating everything with __declspec() attributes.  This
# behavior of the header files does not seem to be documented anywhere.
#

COMPILE = g++ -c

CFLAGS = -g -fPIC $(GCCFLAGS) -DOS_LINUX -DDEBUG \
		-DMX_NO_POISON -DPOSIX_HOSTPC -Wno-cast-qual $(CFLAGS_MODULE)

sapera_lt.mxo: $(SAPERA_LT_OBJS)
	g++ -shared -rdynamic -Wl,-soname,sapera_lt.mxo \
		-o sapera_lt.mxo $(SAPERA_LT_OBJS) \
		$(SAPERA_LT_LIBRARIES)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

CFLAGS += -EHsc
CFLAGS += $(CFLAGS_MODULE)

sapera_lt.mxo: $(SAPERA_LT_OBJS)
	link /dll /debug /nologo /out:sapera_lt.mxo /nodefaultlib:libc \
		$(SAPERA_LT_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(SAPERA_LT_LIBRARIES)

	# For VC2005 or later, embed the manifest in the module.

	if test -f sapera_lt.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:sapera_lt.mxo\;2 \
			-manifest sapera_lt.mxo.manifest ; \
		rm -f sapera_lt.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

sapera_lt.$(OBJ): sapera_lt.c i_sapera_lt.h d_sapera_lt_frame_grabber.h
	$(COMPILE) $(CFLAGS) sapera_lt.c

i_sapera_lt.$(OBJ): i_sapera_lt.cpp i_sapera_lt.h
	$(COMPILE) $(CFLAGS) i_sapera_lt.cpp

d_sapera_lt_frame_grabber.$(OBJ): d_sapera_lt_frame_grabber.cpp \
				d_sapera_lt_frame_grabber.h i_sapera_lt.h
	$(COMPILE) $(CFLAGS) d_sapera_lt_frame_grabber.cpp

d_sapera_lt_camera.$(OBJ): d_sapera_lt_camera.cpp \
				d_sapera_lt_camera.h i_sapera_lt.h
	$(COMPILE) $(CFLAGS) d_sapera_lt_camera.cpp

install: sapera_lt.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp sapera_lt.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


