#
# This module contains drivers for the Nuvant EzStat series of
# potentiostats and galvanostats.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

#--------------------------------------------------------------------------

all: nuvant_ezstat.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER

NUVANT_EZSTAT_OBJS = nuvant_ezstat.$(OBJ) i_nuvant_ezstat.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

nuvant_ezstat.mxo: $(NUVANT_EZSTAT_OBJS)
	gcc -shared -rdynamic \
		-Wl,-soname,nuvant_ezstat.mxo -o nuvant_ezstat.mxo \
		$(NUVANT_EZSTAT_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

nuvant_ezstat.mxo: $(NUVANT_EZSTAT_OBJS)
	link /dll /debug /nologo /out:nuvant_ezstat.mxo /nodefaultlib:libc \
		$(NUVANT_EZSTAT_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f nuvant_ezstat.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:nuvant_ezstat.mxo\;2 \
			-manifest nuvant_ezstat.mxo.manifest ; \
		rm -f nuvant_ezstat.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

nuvant_ezstat.$(OBJ): nuvant_ezstat.c i_nuvant_ezstat.h
	$(COMPILE) $(CFLAGS) nuvant_ezstat.c

i_nuvant_ezstat.$(OBJ): i_nuvant_ezstat.c i_nuvant_ezstat.h
	$(COMPILE) $(CFLAGS) i_nuvant_ezstat.c

install: nuvant_ezstat.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp nuvant_ezstat.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

