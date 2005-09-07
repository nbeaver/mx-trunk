LIBMXDIR = ../../../libMx

all: mutex_lock mutex_trylock mutex_recursive

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

mutex_lock: mutex_lock.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o mutex_lock mutex_lock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

mutex_trylock: mutex_trylock.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o mutex_trylock mutex_trylock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

mutex_recursive: mutex_recursive.c $(LIBMXDIR)/libMx.a
	$(CC) $(CFLAGS) -o mutex_recursive mutex_recursive.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/libMx.a $(LIBRARIES)

clean:
	-$(RM) mutex_lock mutex_trylock mutex_recursive

