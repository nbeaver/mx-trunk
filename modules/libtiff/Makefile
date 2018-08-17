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

LIBTIFF_DIR = c:\\opt\\tiff-4.0.6
LIBTIFF_INCLUDES = -I$(LIBTIFF_DIR)\\libtiff
LIBTIFF_LIBRARY = $(LIBTIFF_DIR)\\libtiff\\libtiff.lib

#----

else ifeq ($(MX_ARCH),linux)

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -ltiff

#LIBTIFF_INCLUDES = -I/home/det/local/include
#LIBTIFF_LIBRARY = -L/home/det/local/lib -ltiff

#----

else ifeq ($(MX_ARCH),linux-clang)

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -ltiff

#----

else ifeq ($(MX_ARCH),cygwin)

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -ltiff

#----

else ifeq ($(MX_ARCH),macosx-clang)

# The following assumes that libtiff was 'make install'ed into /usr/local.

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -ltiff

#----

else ifeq ($(MX_ARCH),bsd)

LIBTIFF_INCLUDES = -I/usr/local/include
LIBTIFF_LIBRARY = -L/usr/local/lib -ltiff

#----

else ifeq ($(MX_ARCH),bsd-clang)

LIBTIFF_INCLUDES = -I/usr/local/include
LIBTIFF_LIBRARY = -L/usr/local/lib -ltiff

#----

else ifeq ($(MX_ARCH),solaris-gcc)

LIBTIFF_INCLUDES =
LIBTIFF_LIBRARY = -ltiff

#----

else ifeq ($(MX_ARCH),minix)

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -L/usr/pkg/lib -ltiff

#----

else ifeq ($(MX_ARCH),hurd)

LIBTIFF_INCLUDES = 
LIBTIFF_LIBRARY = -ltiff

#----

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

ifeq ($(MX_ARCH),macosx-clang)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -dynamiclib -Wl,-headerpad_max_install_names \
		-o libtiff.mxo -install_name "$(PWD)/libtiff.mxo" \
		$(LIBTIFF_OBJS) ../../libMx/libMx.dylib $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),bsd)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -shared -rdynamic -o libtiff.mxo $(LIBTIFF_OBJS) \
		../../libMx/libMx.so $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),bsd-clang)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -shared -rdynamic -o libtiff.mxo $(LIBTIFF_OBJS) \
		../../libMx/libMx.so $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),solaris-gcc)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -shared -rdynamic -o libtiff.mxo $(LIBTIFF_OBJS) \
		../../libMx/libMx.so $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),cygwin)

libtiff.mxo: $(LIBTIFF_OBJS)
	dllwrap -o libtiff.mxo \
		$(LIBTIFF_OBJS) ../../libMx/libMx.dll.a $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),minix)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -shared -o libtiff.mxo $(LIBTIFF_OBJS) \
		-L../../libMx -lMx $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),hurd)

libtiff.mxo: $(LIBTIFF_OBJS)
	$(CC) -shared -o libtiff.mxo $(LIBTIFF_OBJS) \
		-L../../libMx -lMx $(LIBTIFF_LIBRARY)

endif

#--------------------------------------------------------------------------

INSTALLED_MODULE_NAME=$(MX_INSTALL_DIR)/lib/modules/libtiff.mxo

libtiff.$(OBJ): libtiff.c e_libtiff.h
	$(COMPILE) $(CFLAGS) libtiff.c

e_libtiff.$(OBJ): e_libtiff.c e_libtiff.h
	$(COMPILE) $(CFLAGS) e_libtiff.c

ifeq ($(MX_ARCH),macosx-clang)

install: libtiff.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libtiff.mxo $(INSTALLED_MODULE_NAME)
	/usr/bin/install_name_tool -id "$(INSTALLED_MODULE_NAME)" \
				$(INSTALLED_MODULE_NAME)
	mx_mac_fix_lib_path $(INSTALLED_MODULE_NAME) \
			$(MX_INSTALL_DIR)/lib/libMx.dylib
else

install: libtiff.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp libtiff.mxo $(INSTALLED_MODULE_NAME)
endif

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

