#
# This modules is only for the 'win32' platform.
#

MX_ARCH = win32

FASTCAM_PCCLIB_DIR = c:\\docume~1\\lavender\\archives\\photron\\PFV2452\\SDK\\VC

all: fastcam_pcclib.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -EHsc
CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(FASTCAM_PCCLIB_DIR)\\Include

FASTCAM_PCCLIB_OBJS = fastcam_pcclib.$(OBJ) \
	i_fastcam_pcclib.$(OBJ) d_fastcam_pcclib_camera.$(OBJ)

fastcam_pcclib.mxo: $(FASTCAM_PCCLIB_OBJS)
	link /dll /debug /nologo /out:fastcam_pcclib.mxo /nodefaultlib:libc \
		$(FASTCAM_PCCLIB_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME) \
		$(FASTCAM_PCCLIB_DIR)\\Lib\\PccLib.lib

	# For VC2005 or later, embed the manifest in the module.

	if test -f fastcam_pcclib.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:fastcam_pcclib.mxo\;2 \
			-manifest fastcam_pcclib.mxo.manifest ; \
		rm -f fastcam_pcclib.mxo.manifest; \
	fi

fastcam_pcclib.$(OBJ): fastcam_pcclib.c i_fastcam_pcclib.h \
					d_fastcam_pcclib_camera.h
	$(COMPILE) $(CFLAGS) fastcam_pcclib.c

i_fastcam_pcclib.$(OBJ): i_fastcam_pcclib.cpp i_fastcam_pcclib.h
	$(COMPILE) $(CFLAGS) i_fastcam_pcclib.cpp

d_fastcam_pcclib_camera.$(OBJ): d_fastcam_pcclib_camera.cpp \
				d_fastcam_pcclib_camera.h i_fastcam_pcclib.h
	$(COMPILE) $(CFLAGS) d_fastcam_pcclib_camera.cpp

install: fastcam_pcclib.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp fastcam_pcclib.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


