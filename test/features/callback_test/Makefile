LIBMXDIR = ../../../libMx

all: vcc_monitor vcc_monitor_old

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vcc_monitor: vcc_monitor.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_monitor$(DOTEXE) vcc_monitor.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

vcc_monitor_old: vcc_monitor_old.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_monitor_old$(DOTEXE) vcc_monitor_old.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) vcc_monitor vcc_monitor_old
	-$(RM) *.o *.obj *.exe *.ilk *.pdb *.manifest

