MX_ARCH = linux

LIBMXDIR = ../libMx

all: example1 example1_lowlevel example2 example3

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

example1: example1.o
	$(CC) $(CFLAGS) -o example1 example1.o \
			$(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

example1.o: example1.c
	$(CC) $(CFLAGS) -c -I$(LIBMXDIR) example1.c

example1_lowlevel: example1_lowlevel.o
	$(CC) $(CFLAGS) -o example1_lowlevel example1_lowlevel.o \
			$(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

example1_lowlevel.o: example1_lowlevel.c
	$(CC) $(CFLAGS) -c -I$(LIBMXDIR) example1_lowlevel.c

example2: example2.o
	$(CC) $(CFLAGS) -o example2 example2.o \
			$(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

example2.o: example2.c
	$(CC) $(CFLAGS) -c -I$(LIBMXDIR) example2.c

example3: example3.o
	$(CC) $(CFLAGS) -o example3 example3.o \
			$(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

example3.o: example3.c
	$(CC) $(CFLAGS) -c -I$(LIBMXDIR) example3.c

clean:
	-rm example1 example1_lowlevel example2 example3
	-rm *.o
	-rm *.out

distclean: clean

depend:

