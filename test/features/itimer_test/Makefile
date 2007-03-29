LIBMXDIR = ../../../libMx

all: itimer_oneshot itimer_periodic

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

itimer_oneshot: itimer_oneshot.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)itimer_oneshot$(DOTEXE) itimer_oneshot.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

itimer_periodic: itimer_periodic.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)itimer_periodic$(DOTEXE) itimer_periodic.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) itimer_oneshot itimer_periodic *.o *.obj *.exe *.ilk *.pdb *.manifest

