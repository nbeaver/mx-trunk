LIBMXDIR = ../../../libMx

all: boot_test

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

boot_test: boot_test.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) -I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(EXEOUT)boot_test$(DOTEXE) boot_test.c $(LIBRARIES)

clean:
	-$(RM) boot_test *.o *.obj *.exe *.ilk *.pdb *.tds *.manifest

