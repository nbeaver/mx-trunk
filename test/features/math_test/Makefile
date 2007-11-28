LIBMXDIR = ../../../libMx

all: tridiagonal cubic_spline

include $(LIBMXDIR)/Makefile.version
include $(LIBMXDIR)/Makehead.$(MX_ARCH)

tridiagonal: tridiagonal.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)tridiagonal$(DOTEXE) tridiagonal.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

cubic_spline: cubic_spline.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)cubic_spline$(DOTEXE) cubic_spline.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) tridiagonal cubic_spline *.o *.obj *.exe *.ilk *.pdb *.manifest

