#
# Supported values of MX_ARCH:   linux
#

MX_ARCH = linux
MX_INSTALL_DIR = /opt/mx

PYTHON_VERSION = 2.7

PYTHON_INCLUDES = /usr/include/python$(PYTHON_VERSION)
PYTHON_LINK = $(shell python$(PYTHON_VERSION)-config --ldflags)

#--------------------------------------------------------------------------

all: python.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ -I$(PYTHON_INCLUDES)

PYTHON_OBJS = python.$(OBJ) e_python.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

python.mxo: $(PYTHON_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,python.mxo \
		-o python.mxo $(PYTHON_OBJS) $(PYTHON_LINK) \
		../../libMx/libMx.so

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

