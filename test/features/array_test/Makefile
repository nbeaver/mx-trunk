LIBMXDIR = ../../../libMx

all: scaled_vector

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

scaled_vector: scaled_vector.c 
	$(CC) $(CFLAGS) -Wno-error \
		$(EXEOUT)scaled_vector$(DOTEXE) scaled_vector.c \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) \
		$(LIB_DIRS) $(LIBRARIES)

clean:
	-$(RM) scaled_vector *.o *.obj *.exe *.ilk *.pdb *.manifest

