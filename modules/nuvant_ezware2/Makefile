#
# This module contains drivers for the Nuvant EzStat series of
# potentiostats and galvanostats.
#

ifndef MX_ARCH
  MX_ARCH = win32
endif

#--------------------------------------------------------------------------

all: nuvant_ezware2.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER

#NUVANT_EZWARE2_OBJS = nuvant_ezware2.$(OBJ) i_nuvant_ezware2.$(OBJ) \
#	d_nuvant_ezware2_ainput.$(OBJ) d_nuvant_ezware2_aoutput.$(OBJ) \
#	d_nuvant_ezware2_doutput.$(OBJ)

NUVANT_EZWARE2_OBJS = nuvant_ezware2.$(OBJ) i_nuvant_ezware2.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

nuvant_ezware2.mxo: $(NUVANT_EZWARE2_OBJS)
	gcc -shared -rdynamic \
		-Wl,-soname,nuvant_ezware2.mxo -o nuvant_ezware2.mxo \
		$(NUVANT_EZWARE2_OBJS) ../../libMx/libMx.so

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

nuvant_ezware2.mxo: $(NUVANT_EZWARE2_OBJS)
	link /dll /debug /nologo /out:nuvant_ezware2.mxo /nodefaultlib:libc \
		$(NUVANT_EZWARE2_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f nuvant_ezware2.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:nuvant_ezware2.mxo\;2 \
			-manifest nuvant_ezware2.mxo.manifest ; \
		rm -f nuvant_ezware2.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

#nuvant_ezware2.$(OBJ): nuvant_ezware2.c i_nuvant_ezware2.h \
#			d_nuvant_ezware2_ainput.h d_nuvant_ezware2_aoutput.h \
#			d_nuvant_ezware2_doutput.h
nuvant_ezware2.$(OBJ): nuvant_ezware2.c
	$(COMPILE) $(CFLAGS) nuvant_ezware2.c

i_nuvant_ezware2.$(OBJ): i_nuvant_ezware2.c i_nuvant_ezware2.h
	$(COMPILE) $(CFLAGS) i_nuvant_ezware2.c

d_nuvant_ezware2_ainput.$(OBJ): d_nuvant_ezware2_ainput.c \
				d_nuvant_ezware2_ainput.h i_nuvant_ezware2.h
	$(COMPILE) $(CFLAGS) d_nuvant_ezware2_ainput.c

d_nuvant_ezware2_aoutput.$(OBJ): d_nuvant_ezware2_aoutput.c \
				d_nuvant_ezware2_aoutput.h i_nuvant_ezware2.h
	$(COMPILE) $(CFLAGS) d_nuvant_ezware2_aoutput.c

d_nuvant_ezware2_doutput.$(OBJ): d_nuvant_ezware2_doutput.c \
				d_nuvant_ezware2_doutput.h i_nuvant_ezware2.h
	$(COMPILE) $(CFLAGS) d_nuvant_ezware2_doutput.c

install: nuvant_ezware2.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp nuvant_ezware2.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

