LIBMXDIR = ../../../libMx

all: tridiagonal

include $(LIBMXDIR)/Makefile.version
include $(LIBMXDIR)/Makehead.$(MX_ARCH)

tridiagonal: tridiagonal.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)tridiagonal$(DOTEXE) tridiagonal.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) tridiagonal *.o *.obj *.exe *.ilk *.pdb *.manifest

