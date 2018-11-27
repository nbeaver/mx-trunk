#
# Supported values of MX_ARCH:   cygwin, linux, linux-clang, macosx-clang, win32
#

ifndef MX_ARCH
MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
MX_INSTALL_DIR = /opt/mx
endif

#----

ifeq ($(MX_ARCH),win32)

LIBCURL_DIR = c:\\opt\\curl-4.0.6
LIBCURL_INCLUDES = -I$(LIBCURL_DIR)\\libcurl
LIBCURL_LIBRARY = $(LIBCURL_DIR)\\libcurl\\libcurl.lib

#----

else ifeq ($(MX_ARCH),linux)

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -lcurl

#LIBCURL_INCLUDES = -I/home/det/local/include
#LIBCURL_LIBRARY = -L/home/det/local/lib -lcurl

#----

else ifeq ($(MX_ARCH),linux-clang)

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -lcurl

#----

else ifeq ($(MX_ARCH),cygwin)

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -lcurl

#----

else ifeq ($(MX_ARCH),macosx-clang)

# The following assumes that libcurl was 'make install'ed into /usr/local.

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -lcurl

#----

else ifeq ($(MX_ARCH),bsd)

LIBCURL_INCLUDES = -I/usr/local/include
LIBCURL_LIBRARY = -L/usr/local/lib -lcurl

#----

else ifeq ($(MX_ARCH),bsd-clang)

LIBCURL_INCLUDES = -I/usr/local/include
LIBCURL_LIBRARY = -L/usr/local/lib -lcurl

#----

else ifeq ($(MX_ARCH),solaris-gcc)

LIBCURL_INCLUDES =
LIBCURL_LIBRARY = -lcurl

#----

else ifeq ($(MX_ARCH),minix)

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -L/usr/pkg/lib -lcurl

#----

else ifeq ($(MX_ARCH),hurd)

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -lcurl

#----

else ifeq ($(MX_ARCH),android)

LIBCURL_INCLUDES = 
LIBCURL_LIBRARY = -lcurl

#----

else

error not tested yet.

endif

#--------------------------------------------------------------------------

all: libcurl.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ $(LIBCURL_INCLUDES)

LIBCURL_OBJS = libcurl.$(OBJ) e_libcurl.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

libcurl.mxo: $(LIBCURL_OBJS)
	link /dll /debug /nologo /out:libcurl.mxo /nodefaultlib:libc \
		$(LIBCURL_OBJS) $(LIBCURL_LIBRARY) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) $(WIN32_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f libcurl.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:libcurl.mxo\;2 \
			-manifest libcurl.mxo.manifest ; \
		rm -f libcurl.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,libcurl.mxo \
		-o libcurl.mxo $(LIBCURL_OBJS) \
		../../libMx/libMx.so $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux-clang)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) $(SANITIZE) -shared -rdynamic -Wl,-soname,libcurl.mxo \
		-o libcurl.mxo $(LIBCURL_OBJS) \
		../../libMx/libMx.so $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),macosx-clang)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -dynamiclib -Wl,-headerpad_max_install_names \
		-o libcurl.mxo -install_name "$(PWD)/libcurl.mxo" \
		$(LIBCURL_OBJS) ../../libMx/libMx.dylib $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),bsd)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -rdynamic -o libcurl.mxo $(LIBCURL_OBJS) \
		../../libMx/libMx.so $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),bsd-clang)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -rdynamic -o libcurl.mxo $(LIBCURL_OBJS) \
		../../libMx/libMx.so $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),solaris-gcc)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -rdynamic -o libcurl.mxo $(LIBCURL_OBJS) \
		../../libMx/libMx.so $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),cygwin)

libcurl.mxo: $(LIBCURL_OBJS)
	dllwrap -o libcurl.mxo \
		$(LIBCURL_OBJS) ../../libMx/libMx.dll.a $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),minix)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -o libcurl.mxo $(LIBCURL_OBJS) \
		-L../../libMx -lMx $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),hurd)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -o libcurl.mxo $(LIBCURL_OBJS) \
		-L../../libMx -lMx $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),android)

libcurl.mxo: $(LIBCURL_OBJS)
	$(CC) -shared -o libcurl.mxo $(LIBCURL_OBJS) \
		-L../../libMx -lMx $(LIBCURL_LIBRARY)

endif

#--------------------------------------------------------------------------

INSTALLED_MODULE_NAME=$(MX_INSTALL_DIR)/lib/modules/libcurl.mxo

libcurl.$(OBJ): libcurl.c e_libcurl.h
	$(COMPILE) $(CFLAGS) libcurl.c

e_libcurl.$(OBJ): e_libcurl.c e_libcurl.h
	$(COMPILE) $(CFLAGS) e_libcurl.c

ifeq ($(MX_ARCH),macosx-clang)

install: libcurl.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libcurl.mxo $(INSTALLED_MODULE_NAME)
	/usr/bin/install_name_tool -id "$(INSTALLED_MODULE_NAME)" \
				$(INSTALLED_MODULE_NAME)
	mx_mac_fix_lib_path $(INSTALLED_MODULE_NAME) \
			$(MX_INSTALL_DIR)/lib/libMx.dylib
else

install: libcurl.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libcurl.mxo $(INSTALLED_MODULE_NAME)
endif

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

