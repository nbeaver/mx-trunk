#
# Supported values of MX_ARCH:   linux, win32
#

ifndef ($(MX_ARCH))
MX_ARCH = linux
endif

ifndef ($(MX_INSTALL_DIR))
MX_INSTALL_DIR = /opt/mx
endif

#----

ifeq ($(MX_ARCH),win32)

PYTHON_DIR = c:/python27

PYTHON_DIR = c:\\python27

PYTHON_INCLUDES = $(PYTHON_DIR)\\include

PYTHON_LIBRARY = $(PYTHON_DIR)\\libs\\python27.lib

CFLAGS += -DHAVE_SNPRINTF

#----

else

#PYTHON_VERSION = 2.7

ifndef PYTHON_VERSION
    PYTHON_VERSION := $(shell python -V 2>&1 | cut -f2 -d' ' | cut -c1-3)
endif

PYTHON_INCLUDES = /usr/include/python$(PYTHON_VERSION)
PYTHON_LINK = $(shell python$(PYTHON_VERSION)-config --ldflags)

endif

#--------------------------------------------------------------------------

all: python.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -I$(PYTHON_INCLUDES)

PYTHON_OBJS = python.$(OBJ) e_python.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

python.mxo: $(PYTHON_OBJS)
	link /dll /debug /nologo /out:python.mxo /nodefaultlib:libc \
		$(PYTHON_OBJS) $(PYTHON_LIBRARY) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f python.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:python.mxo\;2 \
			-manifest python.mxo.manifest ; \
		rm -f python.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

python.mxo: $(PYTHON_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,python.mxo \
		-o python.mxo $(PYTHON_OBJS) $(PYTHON_LINK) \
		../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

python.$(OBJ): python.c e_python.h
	$(COMPILE) $(CFLAGS) python.c

e_python.$(OBJ): e_python.c e_python.h
	$(COMPILE) $(CFLAGS) e_python.c

install: python.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp python.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

