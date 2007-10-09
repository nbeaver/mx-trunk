LIBMXDIR = ../../../libMx

all: cxx_test

include $(LIBMXDIR)/Makehead.$(MX_ARCH)

cxx_test: cxx_test.cxx $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME)
	$(CXX) $(CXXFLAGS) $(EXEOUT)cxx_test$(DOTEXE) cxx_test.cxx \
		-I$(LIBMXDIR) $(LIBMXDIR)/$(MX_LIBRARY_STATIC_NAME) $(LIBRARIES)

clean:
	-$(RM) cxx_test *.o *.obj *.exe *.ilk *.pdf *.manifest

