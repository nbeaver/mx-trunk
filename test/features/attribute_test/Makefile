LIBMXDIR = ../../../libMx

all: mxattr_get mxattr_set

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

mxattr_get: mxattr_get.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)mxattr_get$(DOTEXE) mxattr_get.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

mxattr_set: mxattr_set.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)mxattr_set$(DOTEXE) mxattr_set.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) mxattr_get mxattr_set *.o *.obj *.exe *.ilk *.pdb *.manifest

