
MX_ARCH = linux

#------------------------------------------------------------

ifeq ($(MX_ARCH),win32)
	MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH) CXX="cl -DOS_WIN32" CXXFLAGS=-I../../../libMx
else
	MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH)
endif

all:
	( cd attribute_test ; $(MAKECMD) )
	( cd boot_test ; $(MAKECMD) )
	( cd callback_test ; $(MAKECMD) )
	( cd coprocess_test ; $(MAKECMD) )
	( cd cxx_test ; $(MAKECMD) )
	( cd itimer_test ; $(MAKECMD) )
	( cd math_test ; $(MAKECMD) )
	( cd multi_test ; $(MAKECMD) )
	( cd mutex_test ; $(MAKECMD) )
	( cd semaphore_test ; $(MAKECMD) )
	( cd thread_test ; $(MAKECMD) )
	( cd types_test ; $(MAKECMD) )
	( cd vtimer_test ; $(MAKECMD) )

clean:
	( cd attribute_test ; $(MAKECMD) clean )
	( cd boot_test ; $(MAKECMD) clean )
	( cd callback_test ; $(MAKECMD) clean )
	( cd coprocess_test ; $(MAKECMD) clean )
	( cd cxx_test ; $(MAKECMD) clean )
	( cd itimer_test ; $(MAKECMD) clean )
	( cd math_test ; $(MAKECMD) clean )
	( cd multi_test ; $(MAKECMD) clean )
	( cd mutex_test ; $(MAKECMD) clean )
	( cd semaphore_test ; $(MAKECMD) clean )
	( cd thread_test ; $(MAKECMD) clean )
	( cd types_test ; $(MAKECMD) clean )
	( cd vtimer_test ; $(MAKECMD) clean )

distclean: clean

