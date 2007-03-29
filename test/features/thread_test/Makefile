LIBMXDIR = ../../../libMx

all: thread_stop thread_kill

include $(LIBMXDIR)/Makefile.version
include $(LIBMXDIR)/Makehead.$(MX_ARCH)

thread_stop: thread_stop.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)thread_stop$(DOTEXE) thread_stop.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

thread_kill: thread_kill.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)thread_kill$(DOTEXE) thread_kill.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) thread_stop thread_kill *.o *.obj *.exe *.ilk *.pdb *.manifest

