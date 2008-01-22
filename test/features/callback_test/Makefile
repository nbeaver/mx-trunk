LIBMXDIR = ../../../libMx

all: vcc_monitor_long

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

vcc_monitor_long: vcc_monitor_long.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)vcc_monitor_long$(DOTEXE) vcc_monitor_long.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) vcc_monitor_long *.o *.obj *.exe *.ilk *.pdb *.manifest

