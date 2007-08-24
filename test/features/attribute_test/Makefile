LIBMXDIR = ../../../libMx

all: vcc_get vcc_set

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vcc_get: vcc_get.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_get$(DOTEXE) vcc_get.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

vcc_set: vcc_set.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_set$(DOTEXE) vcc_set.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) vcc_get vcc_set *.o *.obj *.exe *.ilk *.pdb *.manifest

