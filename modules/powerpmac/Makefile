#
# This module is only for Debian 5.0 (powerpc) running on a PowerPMAC CPU.
# You do not cross-compile this module.  Instead, you compile it directly
# on the PowerPMAC using the copies of GCC and other tools that are
# preinstalled in the PowerPMAC's flash memory.
#

all: powerpmac.mxo

MX_ARCH = linux

POWERPMAC_DIR = /opt/ppmac

POWERPMAC_INCLUDES = -I$(POWERPMAC_DIR)/libppmac \
			-I$(POWERPMAC_DIR)/rtpmac \
			-I$(POWERPMAC_DIR)/libopener \
			-Wno-strict-prototypes

XENOMAI_DIR = /usr/local/xenomai

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__ $(POWERPMAC_INCLUDES)

POWERPMAC_OBJS = powerpmac.$(OBJ) \
	d_powerpmac.$(OBJ) d_powerpmac_aio.$(OBJ) d_powerpmac_dio.$(OBJ) \
	i_powerpmac.$(OBJ) v_powerpmac.$(OBJ)

powerpmac.mxo: $(POWERPMAC_OBJS)
	gcc -shared -rdynamic -Wl,-soname,powerpmac.mxo \
		-o powerpmac.mxo $(POWERPMAC_OBJS) \
		-L$(POWERPMAC_DIR)/libppmac -lppmac \
		-L$(XENOMAI_DIR)/lib -lpthread_rt -lxenomai

powerpmac.$(OBJ): powerpmac.c d_powerpmac.h
	$(COMPILE) $(CFLAGS) powerpmac.c

d_powerpmac.$(OBJ): d_powerpmac.c d_powerpmac.h i_powerpmac.h
	$(COMPILE) $(CFLAGS) d_powerpmac.c

d_powerpmac_aio.$(OBJ): d_powerpmac_aio.c d_powerpmac_aio.h i_powerpmac.h
	$(COMPILE) $(CFLAGS) d_powerpmac_aio.c

d_powerpmac_dio.$(OBJ): d_powerpmac_dio.c d_powerpmac_dio.h i_powerpmac.h
	$(COMPILE) $(CFLAGS) d_powerpmac_dio.c

i_powerpmac.$(OBJ): i_powerpmac.c i_powerpmac.h
	$(COMPILE) $(CFLAGS) i_powerpmac.c

v_powerpmac.$(OBJ): v_powerpmac.c v_powerpmac.h i_powerpmac.h
	$(COMPILE) $(CFLAGS) v_powerpmac.c


install: powerpmac.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp powerpmac.mxo $(MX_INSTALL_DIR)/lib/modules

clean:
	-$(RM) *.mxo *.o

distclean: clean

depend:

