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

MXMONITOR_SRCS = mxmonitor.c

MXGET_SRCS = mxget.c

MXPUT_SRCS = mxput.c

# Mxserial is not available on all platforms, so we instead define the
# MXSERIAL_SRCS macro in the platform-specific Makehead.* files.

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

mx_build: $(MXDRIVERINFO_NAME) $(MXMONITOR_NAME) \
		$(MXGET_NAME) $(MXPUT_NAME) $(MXSERIAL_NAME)

mx_clean:
	-$(RM) *.$(OBJ)
	-$(RM) *.bak
	-$(RM) *.dbg
	-$(RM) *.exe
	-$(RM) *.ilk
	-$(RM) *.manifest
	-$(RM) *.map
	-$(RM) *.pdb
	-$(RM) *.tds
	-$(RM) core*
	-$(RM) $(MXDRIVERINFO_NAME)
	-$(RM) $(MXMONITOR_NAME)
	-$(RM) $(MXGET_NAME)
	-$(RM) $(MXPUT_NAME)
	-$(RM) $(MXSERIAL_NAME)

mx_distclean: mx_clean
	-$(MAKEDEPEND_CLEAN)

mx_depend:
	$(MAKEDEPEND)

mx_install: util_install

include Makefile.depend

#
#--------------------------------------------------------------------------
#

APP_FLAGS=$(DEFINE)__MX_APP__

mxdriverinfo.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mxdriverinfo.c

mxmonitor.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mxmonitor.c

mxget.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mxget.c

mxput.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mxput.c

