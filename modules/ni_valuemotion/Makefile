#
# Win32 driver for the National Instruments stepper and servo boards
# that were originally made by nuLogic.  This includes the ValueMotion
# series of motor controllers.  The PCMOTION32.DLL and PCMOTION32.LIB
# files required are normally provided with the board at the time of
# purchase.
#
# The 'ni_valuemotion' module is only for the 'win32' platform.
#
# Warning: The drivers for the National Instruments ValueMotion controllers
# have not been tested in a long time, so they may require a bit of tweaking
# to work with modern versions of MX.
#

MX_ARCH = win32

#VALUEMOTION_DIR      = c:\\nuLogic\\pcMotion
#VALUEMOTION_INCLUDES = -I$(VALUEMOTION_DIR)\\SUPPORT
#VALUEMOTION_LIBS     = $(VALUEMOTION_DIR)\\SUPPORT\\Microsoft\\pcmotionMS.lib

VALUEMOTION_DIR      = c:\\ValueMotion
VALUEMOTION_INCLUDES = -I$(VALUEMOTION_DIR)\\Include
VALUEMOTION_LIBS     = $(VALUEMOTION_DIR)\\lib\\Microsoft\\pcmotionMS.lib

all: ni_valuemotion.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += $(VALUEMOTION_INCLUDES)

VALUEMOTION_OBJS = ni_valuemotion.$(OBJ) d_pcmotion32.$(OBJ) i_pcmotion32.$(OBJ)

ni_valuemotion.mxo: $(VALUEMOTION_OBJS)
	link /dll /debug /nologo /out:ni_valuemotion.mxo /nodefaultlib:libc \
		$(VALUEMOTION_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(VALUEMOTION_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f ni_valuemotion.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:ni_valuemotion.mxo\;2 \
			-manifest ni_valuemotion.mxo.manifest ; \
		rm -f ni_valuemotion.mxo.manifest; \
	fi

ni_valuemotion.$(OBJ): ni_valuemotion.c d_pcmotion32.h
	$(COMPILE) $(CFLAGS) ni_valuemotion.c

d_pcmotion32.$(OBJ): d_pcmotion32.c d_pcmotion32.h
	$(COMPILE) $(CFLAGS) d_pcmotion32.c

i_pcmotion32.$(OBJ): i_pcmotion32.c i_pcmotion32.h
	$(COMPILE) $(CFLAGS) i_pcmotion32.c

install: ni_valuemotion.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp ni_valuemotion.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

