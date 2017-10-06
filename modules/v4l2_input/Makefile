#
# Linux-only video input driver for use with the Video4Linux version 2 API.
#
# Please note that this driver does not support the obsolete Vide$Linux
# version 1 API.
#

#--------------------------------------------------------------------------

all: v4l2_input.mxo

include ../../libMx/Makehead.$(MX_ARCH)

CFLAGS += -I../../libMx -D__MX_LIBRARY__

V4L2_INPUT_OBJS = v4l2_input.$(OBJ) d_v4l2_input.$(OBJ)

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux)

v4l2_input.mxo: $(V4L2_INPUT_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,v4l2_input.mxo \
		-o v4l2_input.mxo $(V4L2_INPUT_OBJS) \

endif

#--------------------------------------------------------------------------

ifeq ($(MX_ARCH),linux-clang)

v4l2_input.mxo: $(V4L2_INPUT_OBJS)
	$(CC) -shared -rdynamic -Wl,-soname,v4l2_input.mxo \
		-o v4l2_input.mxo $(V4L2_INPUT_OBJS) \

endif

#--------------------------------------------------------------------------

v4l2_input.$(OBJ): v4l2_input.c d_v4l2_input.h
	$(COMPILE) $(CFLAGS) v4l2_input.c

d_v4l2_input.$(OBJ): d_v4l2_input.c d_v4l2_input.h
	$(COMPILE) $(CFLAGS) d_v4l2_input.c

install: v4l2_input.mxo
	-mkdir $(MX_INSTALL_DIR)/lib/modules
	cp v4l2_input.mxo $(MX_INSTALL_DIR)/lib/modules


clean:
	-$(RM) *.mxo *.o *.obj *.exp *.ilk *.lib *.pdb *.manifest

distclean: clean

depend:

