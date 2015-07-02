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

MOTOR_SRCS = command.c marea_detector.c mcopy.c mcopyfile.c mcount.c \
	mdelete.c mdialog.c mdisplay.c mexec.c mgpib.c \
	mheader.c mhelp.c mhome.c \
	minit.c mjog.c mkill.c mload.c mmca.c mmcs.c mmeasure.c mmodify.c \
	mmove_absolute.c mmove_relative.c motor.c moverwrite.c \
	mparameter.c mptz.c mrelay.c mresync.c mrs232.c \
	msample_changer.c msave.c msca.c \
	mscan.c mscan_common.c mscan_area_detector.c \
	mscan_linear.c mscan_list.c mscan_quick.c mscan_xafs.c \
	mscript.c mset.c mset_motor.c msetup.c mshow.c \
	mstart.c mstop.c msystem.c mtest.c mvinput.c mwvout.c

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
#--------------------------------------------------------------------------
#
# FIXME: There has got to be a better way of doing this without enumerating
#        each and every filename.
#

APP_FLAGS=$(DEFINE)__MX_APP__

command.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) command.c

marea_detector.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) marea_detector.c

mcopy.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mcopy.c

mcopyfile.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mcopyfile.c

mcount.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mcount.c

mdelete.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mdelete.c

mdialog.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mdialog.c

mdisplay.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mdisplay.c

mexec.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mexec.c

mgpib.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mgpib.c

mheader.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mheader.c

mhelp.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mhelp.c

mhome.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mhome.c

minit.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) minit.c

mjog.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mjog.c

mkill.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mkill.c

mload.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mload.c

mmca.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmca.c

mmcs.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmcs.c

mmeasure.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmeasure.c

mmodify.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmodify.c

mmove_absolute.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmove_absolute.c

mmove_relative.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmove_relative.c

motor.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) motor.c

moverwrite.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) moverwrite.c

mparameter.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mparameter.c

mptz.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mptz.c

mrelay.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mrelay.c

mresync.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mresync.c

mrs232.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mrs232.c

msample_changer.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msample_changer.c

msave.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msave.c

msca.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msca.c

mscan.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan.c

mscan_common.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_common.c

mscan_area_detector.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_area_detector.c

mscan_linear.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_linear.c

mscan_list.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_list.c

mscan_quick.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_quick.c

mscan_xafs.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_xafs.c

mscript.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscript.c

mset.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mset.c

mset_motor.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mset_motor.c

msetup.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msetup.c

mshow.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mshow.c

mstart.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mstart.c

mstop.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mstop.c

msystem.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msystem.c

mtest.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mtest.c

mvinput.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mvinput.c

mwvout.$(OBJ):
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mwvout.c

