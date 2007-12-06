LIBMXDIR = ../../../libMx

all: tridiagonal natural_cubic_spline clamped_cubic_spline

include $(LIBMXDIR)/Makefile.version
include $(LIBMXDIR)/Makehead.$(MX_ARCH)

tridiagonal: tridiagonal.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)tridiagonal$(DOTEXE) tridiagonal.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

natural_cubic_spline: natural_cubic_spline.c \
			$(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)natural_cubic_spline$(DOTEXE) \
		natural_cubic_spline.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clamped_cubic_spline: clamped_cubic_spline.c \
			$(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)clamped_cubic_spline$(DOTEXE) \
		clamped_cubic_spline.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) tridiagonal natural_cubic_spline clamped_cubic_spline \
		*.o *.obj *.exe *.ilk *.pdb *.manifest

