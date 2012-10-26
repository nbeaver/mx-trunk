#
# The 'linux_portio' module is only for Linux.
#

MX_ARCH = linux

PORTIO_INCLUDES=/usr/local/mx/include

all: linux_portio.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__
CFLAGS += -I$(PORTIO_INCLUDES)

LINUX_PORTIO_OBJS = linux_portio.$(OBJ) i_linux_portio.$(OBJ)

linux_portio.mxo: $(LINUX_PORTIO_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,linux_portio.mxo \
		-o linux_portio.mxo $(LINUX_PORTIO_OBJS)

linux_portio.$(OBJ): linux_portio.c
	$(COMPILE) $(CFLAGS) linux_portio.c

i_linux_portio.$(OBJ): i_linux_portio.c i_linux_portio.h
	$(COMPILE) $(CFLAGS) i_linux_portio.c

install: linux_portio.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp linux_portio.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

