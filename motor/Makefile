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
# List all of the source code files used to build motor.
#

MOTOR_SRCS = command.c marea_detector.c mccd.c mcopy.c mcopyfile.c mcount.c \
	mdelete.c mdialog.c mdisplay.c mexec.c mgpib.c mheader.c mhome.c \
	minit.c mjog.c mkill.c mload.c mmca.c mmcs.c mmeasure.c mmodify.c \
	mmove_absolute.c mmove_relative.c motor.c moverwrite.c \
	mparameter.c mptz.c mrelay.c mresync.c mrs232.c \
	msample_changer.c msave.c msca.c \
	mscan.c mscan_common.c \
	mscan_linear.c mscan_list.c mscan_quick.c mscan_xafs.c \
	mset.c mset_motor.c msetup.c mshow.c \
	mstart.c mstop.c msystem.c mtake.c mvinput.c mwvout.c

#
# This variable specifies the name of the directory containing the
# 'motor' source code.
#

mx_directory_name = "motor"

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

mx_build: $(MOTOR_NAME)

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
	-$(RM) core*
	-$(RM) $(MOTOR_NAME) $(MOTOR_NAME2)

mx_distclean: mx_clean
	-$(MAKEDEPEND_CLEAN)

mx_depend:
	$(MAKEDEPEND)

mx_install: motor_install

#
# Include standard header file dependencies here.  If 'Makefile.depend' does
# not already exist for some reason, then just create a zero length file
# called 'Makefile.depend' and then run "make mx_depend" to create the real 
# 'Makefile.depend' file.
#

include Makefile.depend

#
# Now, list targets with non-standard rules or dependencies.
#

command.o:
	$(COMPILE) $(CFLAGS) command.c

