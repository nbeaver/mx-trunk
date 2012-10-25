#
# Linux and Win32 drivers for the Precision MicroControl Motion Control API
# (MCAPI) for their series of motor controllers.
#
# The vendor's software and drivers may be downloaded from
#   http://www.pmccorp.com/support/downloads.php
#

#
# Supported values of MX_ARCH:  linux win32
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifeq ($(MX_ARCH),linux)
	PMC_MCAPI_INCLUDES  = /usr/local/mcapi-4.1.2/src
	PMC_MCAPI_LIB_DIRS  = /usr/local/lib
	PMC_MCAPI_LIBRARIES = -lmcapi
endif

ifeq ($(MX_ARCH),win32)
	PMC_MCAPI_DIR       = c:\\progra~1\\motion~1\motion~1\\sources
	PMC_MCAPI_INCLUDES  = $(PMC_MCAPI_DIR)
	PMC_MCAPI_LIBS      = $(PMC_MCAPI_DIR)\\mcapi32.lib \
				$(PMC_MCAPI_DIR)\\mcdlg32.lib
endif

#--------------------------------------------------------------------------

all: pmc_mcapi.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -I$(PMC_MCAPI_INCLUDES)

PMC_MCAPI_OBJS = pmc_mcapi.$(OBJ) i_pmc_mcapi.$(OBJ) \
		d_pmc_mcapi.$(OBJ) d_pmc_mcapi_aio.$(OBJ) \
		d_pmc_mcapi_dio.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

pmc_mcapi.mxo: $(PMC_MCAPI_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,pmc_mcapi.mxo \
		-o pmc_mcapi.mxo $(PMC_MCAPI_OBJS) \
		-L$(PMC_MCAPI_LIB_DIRS) $(PMC_MCAPI_LIBRARIES)

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

pmc_mcapi.mxo: $(PMC_MCAPI_OBJS)
	link /dll /debug /nologo /out:pmc_mcapi.mxo /nodefaultlib:libc \
		$(PMC_MCAPI_OBJS) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(PMC_MCAPI_LIBRARY)

	# For VC2005 or later, embed the manifest in the module.

	if test -f pmc_mcapi.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:pmc_mcapi.mxo\;2 \
			-manifest pmc_mcapi.mxo.manifest ; \
		rm -f pmc_mcapi.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

pmc_mcapi.$(OBJ): pmc_mcapi.c i_pmc_mcapi.h \
			d_pmc_mcapi.h d_pmc_mcapi_aio.h \
			d_pmc_mcapi_dio.h
	$(COMPILE) $(CFLAGS) pmc_mcapi.c

i_pmc_mcapi.$(OBJ): i_pmc_mcapi.c i_pmc_mcapi.h
	$(COMPILE) $(CFLAGS) i_pmc_mcapi.c

d_pmc_mcapi.$(OBJ): d_pmc_mcapi.c i_pmc_mcapi.h \
			d_pmc_mcapi.h
	$(COMPILE) $(CFLAGS) d_pmc_mcapi.c

d_pmc_mcapi_aio.$(OBJ): d_pmc_mcapi_aio.c i_pmc_mcapi.h \
			d_pmc_mcapi_aio.h
	$(COMPILE) $(CFLAGS) d_pmc_mcapi_aio.c

d_pmc_mcapi_dio.$(OBJ): d_pmc_mcapi_dio.c i_pmc_mcapi.h \
			d_pmc_mcapi_dio.h
	$(COMPILE) $(CFLAGS) d_pmc_mcapi_dio.c

install: pmc_mcapi.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp pmc_mcapi.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

