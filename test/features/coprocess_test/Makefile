LIBMXDIR = ../../../libMx

all: coprocess_test coprocess_echo

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

coprocess_test: coprocess_test.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)coprocess_test$(DOTEXE) coprocess_test.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

coprocess_echo: coprocess_echo.c $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CC) $(CFLAGS) $(EXEOUT)coprocess_echo$(DOTEXE) coprocess_echo.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

clean:
	-$(RM) coprocess_test coprocess_echo
	-$(RM) *.o *.obj *.exe *.ilk *.pdb *.manifest

