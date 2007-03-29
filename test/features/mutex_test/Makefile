LIBMXDIR = ../../../libMx

all: mutex_lock mutex_trylock mutex_recursive

include $(LIBMXDIR)/Makefile.version
include $(LIBMXDIR)/Makehead.$(MX_ARCH)

mutex_lock: mutex_lock.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)mutex_lock$(DOTEXE) mutex_lock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

mutex_trylock: mutex_trylock.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)mutex_trylock$(DOTEXE) mutex_trylock.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

mutex_recursive: mutex_recursive.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)mutex_recursive$(DOTEXE) mutex_recursive.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) mutex_lock mutex_trylock mutex_recursive \
		*.o *.obj *.exe *.ilk *.pdb *.manifest

