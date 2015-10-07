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

command.$(OBJ): command.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) command.c

marea_detector.$(OBJ): marea_detector.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) marea_detector.c

mcopy.$(OBJ): mcopy.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mcopy.c

mcopyfile.$(OBJ): mcopyfile.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mcopyfile.c

mcount.$(OBJ): mcount.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mcount.c

mdelete.$(OBJ): mdelete.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mdelete.c

mdialog.$(OBJ): mdialog.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mdialog.c

mdisplay.$(OBJ): mdisplay.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mdisplay.c

mexec.$(OBJ): mexec.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mexec.c

mgpib.$(OBJ): mgpib.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mgpib.c

mheader.$(OBJ): mheader.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mheader.c

mhelp.$(OBJ): mhelp.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mhelp.c

mhome.$(OBJ): mhome.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mhome.c

minit.$(OBJ): minit.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) minit.c

mjog.$(OBJ): mjog.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mjog.c

mkill.$(OBJ): mkill.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mkill.c

mload.$(OBJ): mload.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mload.c

mmca.$(OBJ): mmca.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmca.c

mmcs.$(OBJ): mmcs.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmcs.c

mmeasure.$(OBJ): mmeasure.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmeasure.c

mmodify.$(OBJ): mmodify.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmodify.c

mmove_absolute.$(OBJ): mmove_absolute.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmove_absolute.c

mmove_relative.$(OBJ): mmove_relative.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mmove_relative.c

motor.$(OBJ): motor.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) motor.c

moverwrite.$(OBJ): moverwrite.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) moverwrite.c

mparameter.$(OBJ): mparameter.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mparameter.c

mptz.$(OBJ): mptz.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mptz.c

mrelay.$(OBJ): mrelay.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mrelay.c

mresync.$(OBJ): mresync.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mresync.c

mrs232.$(OBJ): mrs232.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mrs232.c

msample_changer.$(OBJ): msample_changer.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msample_changer.c

msave.$(OBJ): msave.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msave.c

msca.$(OBJ): msca.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msca.c

mscan.$(OBJ): mscan.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan.c

mscan_common.$(OBJ): mscan_common.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_common.c

mscan_area_detector.$(OBJ): mscan_area_detector.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_area_detector.c

mscan_linear.$(OBJ): mscan_linear.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_linear.c

mscan_list.$(OBJ): mscan_list.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_list.c

mscan_quick.$(OBJ): mscan_quick.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_quick.c

mscan_xafs.$(OBJ): mscan_xafs.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscan_xafs.c

mscript.$(OBJ): mscript.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mscript.c

mset.$(OBJ): mset.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mset.c

mset_motor.$(OBJ): mset_motor.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mset_motor.c

msetup.$(OBJ): msetup.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msetup.c

mshow.$(OBJ): mshow.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mshow.c

mstart.$(OBJ): mstart.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mstart.c

mstop.$(OBJ): mstop.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mstop.c

msystem.$(OBJ): msystem.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) msystem.c

mtest.$(OBJ): mtest.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mtest.c

mvinput.$(OBJ): mvinput.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mvinput.c

mwvout.$(OBJ): mwvout.c
	$(COMPILE) $(CFLAGS) $(APP_FLAGS) mwvout.c

