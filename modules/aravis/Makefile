#
# This module is only for the 'linux' platform.
#

ARAVIS_INCLUDES = -I/usr/include/aravis-0.2
ARAVIS_INCLUDES += -I/usr/include/glib-2.0
ARAVIS_INCLUDES += -I/usr/lib/glib-2.0/include

ARAVIS_LIBRARIES = -laravis-0.2

#--------------------------------------------------------------------------

all: aravis.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS_MODULE = -I../../libMx -D__MX_LIBRARY__ $(ARAVIS_INCLUDES) \
	-Wno-cast-qual

ARAVIS_OBJS = aravis.$(OBJ) i_aravis.$(OBJ) d_aravis_camera.$(OBJ)

#--------------------------------------------------------------------------

GCC = gcc

CFLAGS = -g -fPIC $(GCCFLAGS) -DOS_LINUX -DDEBUG $(CFLAGS_MODULE)

#--------------------------------------------------------------------------

aravis.mxo: $(ARAVIS_OBJS)
	gcc -shared -rdynamic -Wl,-soname,aravis.mxo \
		-o aravis.mxo $(ARAVIS_OBJS) \
		$(ARAVIS_LIBRARIES)

#--------------------------------------------------------------------------

aravis.$(OBJ): aravis.c i_aravis.h d_aravis_camera.h
	$(GCC) -c $(CFLAGS) aravis.c

i_aravis.$(OBJ): i_aravis.c i_aravis.h
	$(GCC) -c $(CFLAGS) i_aravis.c

d_aravis_camera.$(OBJ): d_aravis_camera.c d_aravis_camera.h i_aravis.h
	$(GCC) -c $(CFLAGS) d_aravis_camera.c

install: aravis.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp aravis.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:


