MX_ARCH = linux

MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH)

all:
	( cd itimer_test ; $(MAKECMD) )
	( cd mutex_test ; $(MAKECMD) )
	( cd semaphore_test ; $(MAKECMD) )
	( cd thread_test ; $(MAKECMD) )
	( cd types_test ; $(MAKECMD) )
	( cd vtimer_test ; $(MAKECMD) )

clean:
	( cd itimer_test ; $(MAKECMD) clean )
	( cd mutex_test ; $(MAKECMD) clean )
	( cd semaphore_test ; $(MAKECMD) clean )
	( cd thread_test ; $(MAKECMD) clean )
	( cd types_test ; $(MAKECMD) clean )
	( cd vtimer_test ; $(MAKECMD) clean )

