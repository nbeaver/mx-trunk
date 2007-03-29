LIBMXDIR = ../../../libMx

all: vcc_monitor

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vcc_monitor: vcc_monitor.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_monitor$(DOTEXE) vcc_monitor.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) vcc_monitor *.o *.obj *.exe *.ilk *.pdb *.manifest

