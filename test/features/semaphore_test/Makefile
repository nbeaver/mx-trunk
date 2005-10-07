LIBMXDIR = ../../../libMx

all: semaphore_lock semaphore_trylock \
		named_semaphore_server named_semaphore_client

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

semaphore_lock: semaphore_lock.c $(LIBMXDIR)/$(MX_LIBRARY_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)semaphore_lock$(DOTEXE) semaphore_lock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_NAME) $(LIBRARIES)

semaphore_trylock: semaphore_trylock.c $(LIBMXDIR)/$(MX_LIBRARY_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)semaphore_trylock$(DOTEXE) \
		semaphore_trylock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_NAME) $(LIBRARIES)

named_semaphore_server: named_semaphore_server.c $(LIBMXDIR)/$(MX_LIBRARY_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)named_semaphore_server$(DOTEXE) \
		named_semaphore_server.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_NAME) $(LIBRARIES)

named_semaphore_client: named_semaphore_client.c $(LIBMXDIR)/$(MX_LIBRARY_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)named_semaphore_client$(DOTEXE) \
		named_semaphore_client.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_NAME) $(LIBRARIES)

clean:
	-$(RM) semaphore_lock semaphore_trylock
	-$(RM) named_semaphore_server named_semaphore_client
	-$(RM) *.o *.obj *.exe *.ilk *.pdb

