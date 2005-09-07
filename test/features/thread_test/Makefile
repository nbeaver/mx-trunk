LIBMXDIR = ../../../libMx

all: thread_stop thread_kill

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

thread_stop: thread_stop.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o thread_stop thread_stop.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

thread_kill: thread_kill.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o thread_kill thread_kill.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

clean:
	-$(RM) thread_stop thread_kill

