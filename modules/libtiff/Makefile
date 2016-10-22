#
# Supported values of MX_ARCH:   linux
#

ifndef MX_ARCH
MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
MX_INSTALL_DIR = /opt/mx
endif

#----

ifeq ($(MX_ARCH),win32)

# Not tested on Win32 yet.

LIBTIFF_DIR = c:\\opt\\tiff-4.0.6
LIBTIFF_INCLUDES = -I$(LIBTIFF_DIR)\\libtiff
LIBTIFF_LIBRARY = $(LIBTIFF_DIR)\\libtiff\\libtiff.lib

#----

else ifeq ($(MX_ARCH),linux)

#LIBTIFF_INCLUDES = 
#LIBTIFF_LIBRARY = -ltiff

LIBTIFF_INCLUDES = -I/home/det/local/include
LIBTIFF_LIBRARY = -L/home/det/local/lib -ltiff

#----

else ifeq ($(MX_ARCH),linux-clang)

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -ltiff

else

error not tested yet.

endif

#--------------------------------------------------------------------------

all: libtiff.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ $(LIBTIFF_INCLUDES)

LIBTIFF_OBJS = libtiff.$(OBJ) e_libtiff.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

libtiff.mxo: $(LIBTIFF_OBJS)
	link /dll /debug /nologo /out:libtiff.mxo /nodefaultlib:libc \
		$(LIBTIFF_OBJS) $(LIBTIFF_LIBRARY) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) $(WIN32_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f libtiff.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:libtiff.mxo\;2 \
			-manifest libtiff.mxo.manifest ; \
		rm -f libtiff.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,libtiff.mxo \
		-o libtiff.mxo $(LIBTIFF_OBJS) \
		../../libMx/libMx.so $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux-clang)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) $(SANITIZE) -shared -rdynamic -Wl,-soname,libtiff.mxo \
		-o libtiff.mxo $(LIBTIFF_OBJS) \
		../../libMx/libMx.so $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

libtiff.$(OBJ): libtiff.c e_libtiff.h
	$(COMPILE) $(CFLAGS) libtiff.c

e_libtiff.$(OBJ): e_libtiff.c e_libtiff.h
	$(COMPILE) $(CFLAGS) e_libtiff.c

install: libtiff.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libtiff.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

