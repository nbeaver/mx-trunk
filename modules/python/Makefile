#
# Supported values of MX_ARCH:   linux
#

ifndef MX_ARCH
  MX_ARCH = linux
endif

ifndef MX_INSTALL_DIR
  MX_INSTALL_DIR = /opt/mx
endif

#--------------------------------------------------------------------------

all: python.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

PYTHON_OBJS = python.$(OBJ) e_python.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

python.mxo: $(PYTHON_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,python.mxo \
		-o python.mxo $(PYTHON_OBJS)

endif

#--------------------------------------------------------------------------

python.$(OBJ): python.c e_python.h
	$(COMPILE) $(CFLAGS) python.c

e_python.$(OBJ): e_python.c e_python.h
	$(COMPILE) $(CFLAGS) e_python.c

install: python.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp python.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

