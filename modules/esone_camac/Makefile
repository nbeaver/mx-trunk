#
# This module can be used on the 'linux' and 'win32' platforms.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

CAMAC_TARGET = jorway_73a

#--------------------------------------------------------------------------

all: esone_camac.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER
CFLAGS += $(ESONE_CAMAC_INCLUDES)

ESONE_CAMAC_OBJS = esone_camac.$(OBJ) i_esone_camac.$(OBJ)

#--------------------------------------------------------------------------

#
# The following is for the Jorway 73A.
#

ifeq ($(CAMAC_TARGET),jorway_73a)

    CFLAGS += -DHAVE_JORWAY_CAMAC

    ifeq ($(MX_ARCH),linux)

	ESONE_CAMAC_DIR      = /usr/local/src/sjyLX-v2.5
	ESONE_CAMAC_INCLUDES = -I$(ESONE_CAMAC_DIR)/include
	ESONE_CAMAC_LIBS     = -L$(ESONE_CAMAC_DIR)/lib -lsjy
	
    endif

    ifeq ($(MX_ARCH),win32)

	ESONE_CAMAC_DIR      = C:\\jorway\\sjyWIN
	ESONE_CAMAC_INCLUDES = -I$(ESONE_CAMAC_DIR)\\inc
	ESONE_CAMAC_LIBS     = $(ESONE_CAMAC_DIR)\\lib\\sjy73a.lib

    endif
endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

esone_camac.mxo: $(ESONE_CAMAC_OBJS)
	pwd
	gcc -shared -rdynamic -Wl,-soname,esone_camac.mxo -o esone_camac.mxo \
		$(ESONE_CAMAC_OBJS) ../../libMx/libMx.so \
		$(ESONE_CAMAC_LIBS)
endif

#--------------------------------------------------------------------------

# Win32

ifeq ($(MX_ARCH),win32)

esone_camac.mxo: $(ESONE_CAMAC_OBJS)
	link /dll /debug /nologo /out:esone_camac.mxo /nodefaultlib:libc \
		$(ESONE_CAMAC_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(ESONE_CAMAC_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f esone_camac.mxo.manifest; then \
		$(MXMANIFEST_TOOL) -nologo -outputresource:esone_camac.mxo\;2 \
			-manifest esone_camac.mxo.manifest ; \
		rm -f esone_camac.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

esone_camac.$(OBJ): esone_camac.c i_esone_camac.h
	$(COMPILE) $(CFLAGS) esone_camac.c

i_esone_camac.$(OBJ): i_esone_camac.c i_esone_camac.h
	$(COMPILE) $(CFLAGS) i_esone_camac.c

install: esone_camac.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp esone_camac.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

