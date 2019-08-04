#
# Supported values of MX_ARCH:   linux
#

ifndef ($(MX_ARCH))
MX_ARCH = linux
endif

ifndef ($(MX_INSTALL_DIR))
MX_INSTALL_DIR = /opt/mx
endif

#----

ifeq ($(MX_ARCH),win32)

error Not tested on Win32 yet.  The following is just a guess.

CBFLIB_DIR = c:\\opt\\CBFlib-0.9.5

CBFLIB_INCLUDES = -I$(CBFLIB_DIR)\\include

CBFLIB_LIBRARY = $(CBFLIB_DIR)\\lib

#----

else ifeq ($(MX_ARCH),linux)

CBFLIB_INCLUDES += -isystem /usr/include/cbf
CBFLIB_INCLUDES += -isystem /usr/include/cbflib
CBFLIB_INCLUDES += -I /usr/include/hdf5/serial

CBFLIB_LIBRARY = -lcbf

#----

else ifeq ($(MX_ARCH),linux-clang)

CBFLIB_INCLUDES = -isystem /usr/include/cbf

CBFLIB_LIBRARY = -lcbf

else

error not tested yet.

endif

#--------------------------------------------------------------------------

all: cbflib.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ $(CBFLIB_INCLUDES)

CBFLIB_OBJS = cbflib.$(OBJ) e_cbflib.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

cbflib.mxo: $(CBFLIB_OBJS)
	link /dll /debug /nologo /out:cbflib.mxo /nodefaultlib:libc \
		$(CBFLIB_OBJS) $(CBFLIB_LIBRARY) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f cbflib.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:cbflib.mxo\;2 \
			-manifest cbflib.mxo.manifest ; \
		rm -f cbflib.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

cbflib.mxo: $(CBFLIB_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,cbflib.mxo \
		-o cbflib.mxo $(CBFLIB_OBJS) \
		../../libMx/libMx.so $(CBFLIB_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux-clang)

cbflib.mxo: $(CBFLIB_OBJS)
	$(CC) $(SANITIZE) -shared -rdynamic -Wl,-soname,cbflib.mxo \
		-o cbflib.mxo $(CBFLIB_OBJS) \
		../../libMx/libMx.so $(CBFLIB_LIBRARY)

endif

#--------------------------------------------------------------------------

cbflib.$(OBJ): cbflib.c e_cbflib.h
	$(COMPILE) $(CFLAGS) cbflib.c

e_cbflib.$(OBJ): e_cbflib.c e_cbflib.h
	$(COMPILE) $(CFLAGS) e_cbflib.c

install: cbflib.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp cbflib.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

