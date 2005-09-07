LIBMXDIR = ../../../libMx

all: semaphore_lock semaphore_trylock \
		named_semaphore_server named_semaphore_client

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

semaphore_lock: semaphore_lock.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o semaphore_lock semaphore_lock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

semaphore_trylock: semaphore_trylock.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o semaphore_trylock semaphore_trylock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

named_semaphore_server: named_semaphore_server.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o named_semaphore_server named_semaphore_server.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

named_semaphore_client: named_semaphore_client.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o named_semaphore_client named_semaphore_client.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

clean:
	-$(RM) semaphore_lock semaphore_trylock
	-$(RM) named_semaphore_server named_semaphore_client

