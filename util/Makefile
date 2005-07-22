#
# This makefile is normally intended to be invoked by the makefile in
# the top level directory.  If you want to invoke it directly for some
# reason, use commands like
#
# make MX_ARCH=linux mx_clean
# make MX_ARCH=linux mx_depend
# make MX_ARCH=linux
# make MX_ARCH=linux MX_INSTALL_DIR=/opt/mx mx_install
#
 
#
# List all of the source code files used to build each of the utilities.
#

MXDRIVERINFO_SRCS = mxdriverinfo.c

#
#--------------------------------------------------------------------------
#
# The default makefile target must come before the includes since they
# contain makefile targets too.
#
 
all: mx_build

#
#--------------------------------------------------------------------------
#
# Definitions specific to a particular architecture are included here.
#

include ../libMx/Makefile.version

include ../libMx/Makehead.$(MX_ARCH)

#
#--------------------------------------------------------------------------
#
# Makefile targets.
#

mx_build: $(MXDRIVERINFO_NAME)

mx_clean:
	-$(RM) *.$(OBJ)
	-$(RM) *.bak
	-$(RM) *.exe
	-$(RM) *.ilk
	-$(RM) *.pdb
	-$(RM) *.tds
	-$(RM) core*
	-$(RM) $(MXDRIVERINFO_NAME)

mx_distclean: mx_clean
	-$(MAKEDEPEND_CLEAN)

mx_depend:
	$(MAKEDEPEND)

mx_install: mxdriverinfo_install

include Makefile.depend

