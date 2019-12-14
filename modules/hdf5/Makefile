#
# Supported values of MX_ARCH:   linux, win32
#

ifndef MX_ARCH
MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
MX_INSTALL_DIR = /opt/mx
endif

#----

ifeq ($(MX_ARCH),win32)

HDF5_DIR = c:\\opt\\xyzzy
HDF5_INCLUDES = -I$(HDF5_DIR)\\hdf5
HDF5_LIBRARY = $(HDF5_DIR)\\hdf5\\hdf5.lib

#----

else ifeq ($(MX_ARCH),linux)

# FIXME: The following is somewhat fragile.
#
HDF5_INCLUDES := $(shell h5cc -show | cut -d' ' -f2 )
HDF5_LIBRARY  := $(shell h5cc -show | cut -d' ' -f3- )

$(info $$HDF5_INCLUDES is [${HDF5_INCLUDES}])
$(info $$HDF5_LIBRARY is [${HDF5_LIBRARY}])

#----

else

error not tested yet.

endif

#--------------------------------------------------------------------------

all: hdf5.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ $(HDF5_INCLUDES)

#HDF5_OBJS = hdf5.$(OBJ) e_hdf5.$(OBJ)
HDF5_OBJS = hdf5.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),win32)

hdf5.mxo: $(HDF5_OBJS)
	link /dll /debug /nologo /out:hdf5.mxo /nodefaultlib:libc \
		$(HDF5_OBJS) $(HDF5_LIBRARY) \
		..\\..\\libMx\\$(MX_LIBRARY_NAME) $(WIN32_LIBS)

	# For VC2005 or later, embed the manifest in the module.

	if test -f hdf5.mxo.manifest; then \
		$(MSMANIFEST_TOOL) -nologo -outputresource:hdf5.mxo\;2 \
			-manifest hdf5.mxo.manifest ; \
		rm -f hdf5.mxo.manifest; \
	fi
endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

hdf5.mxo: $(HDF5_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,hdf5.mxo \
		-o hdf5.mxo $(HDF5_OBJS) \
		../../libMx/libMx.so $(HDF5_LIBRARY)

endif

#--------------------------------------------------------------------------

INSTALLED_MODULE_NAME=$(MX_INSTALL_DIR)/lib/modules/hdf5.mxo

hdf5.$(OBJ): hdf5.c e_hdf5.h
	$(COMPILE) $(CFLAGS) hdf5.c

e_hdf5.$(OBJ): e_hdf5.c e_hdf5.h
	$(COMPILE) $(CFLAGS) e_hdf5.c

install: hdf5.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp hdf5.mxo $(INSTALLED_MODULE_NAME)

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

