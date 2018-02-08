#
# This module contains drivers for the Compumotor TOAST "toast" controller
# used at BioCAT.
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

#--------------------------------------------------------------------------

all: site_biocat_toast.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -DIS_MX_DRIVER

SITE_BIOCAT_TOAST_OBJS = site_biocat_toast.$(OBJ) \
			o_biocat_6k_toast.$(OBJ) \
			v_biocat_6k_joystick.$(OBJ)

#--------------------------------------------------------------------------

# Windows

ifeq ($(MX_ARCH),win32)

site_biocat_toast.mxo: $(SITE_BIOCAT_TOAST_OBJS)
	link /dll /debug /nologo /out:site_biocat_toast.mxo /nodefaultlib:libc \
		$(SITE_BIOCAT_TOAST_OBJS) ..\\..\\libMx\\$(MX_LIBRARY_NAME)

	# For VC2005 or later, embed the manifest in the module.

	if test -f site_biocat_toast.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo \
			-outputresource:site_biocat_toast.mxo\;2 \
			-manifest site_biocat_toast.mxo.manifest ; \
		rm -f site_biocat_toast.mxo.manifest; \
	fi

endif

#--------------------------------------------------------------------------

# Linux

ifeq ($(MX_ARCH),linux)

site_biocat_toast.mxo: $(SITE_BIOCAT_TOAST_OBJS)
	gcc -shared -rdynamic -Wl,-soname,site_biocat_toast.mxo \
		-o site_biocat_toast.mxo $(SITE_BIOCAT_TOAST_OBJS)
endif

#--------------------------------------------------------------------------

# Linux (with Clang)

ifeq ($(MX_ARCH),linux-clang)

site_biocat_toast.mxo: $(SITE_BIOCAT_TOAST_OBJS)
	clang -shared -rdynamic -Wl,-soname,site_biocat_toast.mxo \
		-o site_biocat_toast.mxo $(SITE_BIOCAT_TOAST_OBJS)
endif

#--------------------------------------------------------------------------

site_biocat_toast.$(OBJ): site_biocat_toast.c \
			o_biocat_6k_toast.h v_biocat_6k_joystick.h
	$(COMPILE) $(CFLAGS) site_biocat_toast.c

o_biocat_6k_toast.$(OBJ): o_biocat_6k_toast.c o_biocat_6k_toast.h
	$(COMPILE) $(CFLAGS) o_biocat_6k_toast.c

v_biocat_6k_joystick.$(OBJ): v_biocat_6k_joystick.c v_biocat_6k_joystick.h
	$(COMPILE) $(CFLAGS) v_biocat_6k_joystick.c

install: site_biocat_toast.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp site_biocat_toast.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

