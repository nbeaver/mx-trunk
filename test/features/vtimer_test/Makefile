LIBMXDIR = ../../../libMx

all: vtimer_oneshot vtimer_periodic

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vtimer_oneshot: vtimer_oneshot.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vtimer_oneshot$(DOTEXE) vtimer_oneshot.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

vtimer_periodic: vtimer_periodic.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vtimer_periodic$(DOTEXE) vtimer_periodic.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) vtimer_oneshot vtimer_periodic *.o *.obj *.exe *.ilk *.pdb

