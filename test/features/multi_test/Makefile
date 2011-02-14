LIBMXDIR = ../../../libMx

all: multi_get

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

multi_get: multi_get.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)multi_get$(DOTEXE) multi_get.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

clean:
	-$(RM) multi_get
	-$(RM) *.o *.obj *.exe *.ilk *.pdb *.manifest

