LIBMXDIR = ../../../libMx

all: vcc_monitor vcc_monitor_long

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vcc_monitor: vcc_monitor.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_monitor$(DOTEXE) vcc_monitor.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

vcc_monitor_long: vcc_monitor_long.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_monitor_long$(DOTEXE) vcc_monitor_long.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) vcc_monitor vcc_monitor_long
	-$(RM) *.o *.obj *.exe *.ilk *.pdb *.manifest

