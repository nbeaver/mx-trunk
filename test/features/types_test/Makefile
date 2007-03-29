LIBMXDIR = ../../../libMx

all: types

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

types: types.c 
	$(CC) $(CFLAGS) $(EXEOUT)types$(DOTEXE) types.c

clean:
	-$(RM) types *.o *.obj *.exe *.ilk *.pdb *.manifest

