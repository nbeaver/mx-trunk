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

#--------------------------------------------------------------------------

all: http.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

HTTP_OBJS = http.$(OBJ) e_http.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

http.mxo: $(HTTP_OBJS)
	link /dll /debug /nologo /out:http.mxo /nodefaultlib:libc \
		$(HTTP_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) $(WIN32_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f http.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:http.mxo\;2 \
			-manifest http.mxo.manifest ; \
		rm -f http.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,http.mxo \
		-o http.mxo $(HTTP_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux-clang)

http.mxo: $(HTTP_OBJS)
	$(CC) $(SANITIZE) -shared -rdynamic -Wl,-soname,http.mxo \
		-o http.mxo $(HTTP_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),macosx-clang)

http.mxo: $(HTTP_OBJS)
	$(CC) -dynamiclib -Wl,-headerpad_max_install_names \
		-o http.mxo -install_name "$(PWD)/http.mxo" \
		$(HTTP_OBJS) ../../libMx/libMx.dylib

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),bsd)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -rdynamic -o http.mxo $(HTTP_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),bsd-clang)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -rdynamic -o http.mxo $(HTTP_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),solaris-gcc)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -rdynamic -o http.mxo $(HTTP_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),cygwin)

http.mxo: $(HTTP_OBJS)
	dllwrap -o http.mxo $(HTTP_OBJS) ../../libMx/libMx.dll.a

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),minix)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -o http.mxo $(HTTP_OBJS) -L../../libMx -lMx

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),hurd)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -o http.mxo $(HTTP_OBJS) -L../../libMx -lMx

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),android)

http.mxo: $(HTTP_OBJS)
	$(CC) -shared -o http.mxo $(HTTP_OBJS) -L../../libMx -lMx

endif

#--------------------------------------------------------------------------

INSTALLED_MODULE_NAME=$(MX_INSTALL_DIR)/lib/modules/http.mxo

http.$(OBJ): http.c e_http.h
	$(COMPILE) $(CFLAGS) http.c

e_http.$(OBJ): e_http.c e_http.h
	$(COMPILE) $(CFLAGS) e_http.c

ifeq ($(MX_ARCH),macosx-clang)

install: http.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp http.mxo $(INSTALLED_MODULE_NAME)
	/usr/bin/install_name_tool -id "$(INSTALLED_MODULE_NAME)" \
				$(INSTALLED_MODULE_NAME)
	mx_mac_fix_lib_path $(INSTALLED_MODULE_NAME) \
			$(MX_INSTALL_DIR)/lib/libMx.dylib
else

install: http.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp http.mxo $(INSTALLED_MODULE_NAME)
endif

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

