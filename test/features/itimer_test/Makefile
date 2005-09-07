LIBMXDIR = ../../../libMx

all: itimer_oneshot itimer_periodic

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

itimer_oneshot: itimer_oneshot.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o itimer_oneshot itimer_oneshot.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

itimer_periodic: itimer_periodic.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o itimer_periodic itimer_periodic.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

clean:
	-$(RM) itimer_oneshot itimer_periodic

