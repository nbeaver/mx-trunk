
MXDIR = /opt/mx

CC = gcc

# The following EXTRA_LIBS variable is necessary for Solaris

# EXTRA_LIBS = -lnsl -lsocket -lposix4

all: example1 example2 example3


example1: example1.o
	$(CC) -o example1 example1.o -L$(MXDIR)/lib -lMx -lm $(EXTRA_LIBS)

example1.o: example1.c
	$(CC) -c -I$(MXDIR)/include example1.c

example2: example2.o
	$(CC) -o example2 example2.o -L$(MXDIR)/lib -lMx -lm $(EXTRA_LIBS)

example2.o: example2.c
	$(CC) -c -I$(MXDIR)/include example2.c

example3: example3.o
	$(CC) -o example3 example3.o -L$(MXDIR)/lib -lMx -lm $(EXTRA_LIBS)

example3.o: example3.c
	$(CC) -c -I$(MXDIR)/include example3.c

clean:
	-rm example1 example2 example3
	-rm *.o
	-rm *.out

distclean: clean

depend:

