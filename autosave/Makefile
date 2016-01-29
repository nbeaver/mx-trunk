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
# List all of the source code files used to build mxautosave.
#

AUTOSAVE_SRCS = ms_autosave.c

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

mx_build: $(AUTOSAVE_NAME)

mx_clean:
	-$(RM) *.$(OBJ)
	-$(RM) *.bak
	-$(RM) *.bin
	-$(RM) *.bt
	-$(RM) *.dbg
	-$(RM) *.exe
	-$(RM) *.ilk
	-$(RM) *.manifest
	-$(RM) *.map
	-$(RM) *.num
	-$(RM) *.nxe
	-$(RM) *.obj
	-$(RM) *.pdb
	-$(RM) *.tds
	-$(RM) *.res
	-$(RM) *.RES
	-$(RM) core*
	-$(RM) $(AUTOSAVE_NAME)

mx_distclean: mx_clean
	-$(MAKEDEPEND_CLEAN)

mx_depend:
	$(MAKEDEPEND)

mx_install: autosave_install

include Makefile.depend

#
#--------------------------------------------------------------------------
#

ms_autosave.$(OBJ): ms_autosave.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) ms_autosave.c

