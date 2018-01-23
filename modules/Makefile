#
# Warning: This makefile assumes that Gnu Make is being used to build
#          the modules.  Other versions of Make will probably fail.
#          It also assumes the presence of a Unix-style Bash or Bourne
#          shell.
#
# You enable the building of each module by uncommenting the "MODULES +="
# line for that module below.
#
# You configure each module by editing the Makefile in its subdirectory.
#

all: build

#-------------------------------------------------------------------
# Multiplatform modules (potentially)

MODULES += aviex_pccd		# linux, win32

#MODULES += avt_pvapi		# linux,

#MODULES += avt_vimba		# linux,

MODULES += cbflib		# linux, (not yet finished)

#MODULES += edt			# linux,

#MODULES += epix_xclib		# linux, win32

#MODULES += esone_camac		# linux, win32

MODULES += k8055		# linux,

#MODULES += labjack_ux		# linux,

MODULES += libtiff		# cygwin, linux, win32

MODULES += libusb-0.1		# linux,

MODULES += newport_xps		# linux,

#MODULES += ni488		# linux,

#MODULES += ni_daqmx		# linux, win32

#MODULES += nuvant_ezstat	# linux, win32

MODULES += python		# (Warning: not yet finished)

#MODULES += pmc_mcapi		# linux, win32

MODULES += radicon_taurus	# linux, win32

#MODULES += sis3100		# linux,

#MODULES += vxi_memacc		# linux,

MODULES += xia_handel		# linux, win32

MODULES += xineos_gige		# linux, win32

#-------------------------------------------------------------------
# Linux-only modules.

MODULES += aravis

#MODULES += dalsa_gev

#MODULES += linux_portio

#MODULES += powerpmac

MODULES += v4l2_input

#-------------------------------------------------------------------
# Win32-only modules.

#MODULES += bnc725

#MODULES += driverlinx_portio

#MODULES += fastcam_pcclib

#MODULES += ni_valuemotion

#MODULES += nuvant_ezware2

#MODULES += ortec_umcbi

#MODULES += pleora_iport

#MODULES += radicon_helios

#MODULES += sapera_lt 

#MODULES += u500

#-------------------------------------------------------------------
# EPICS modules.

MODULES += epics

MODULES += epics_aps 

MODULES += epics_pmac_biocat

MODULES += epics_mbc

MODULES += epics_pmac_tc

#-------------------------------------------------------------------
# Site specific modules.
#

#
#   MBC (ALS 4.2.2) RDI detector
#
MODULES += rdi_mbc

#
#   BioCAT (APS-18ID) 6K "toast" controller.
#
MODULES += site_biocat_toast

#---

MAKECMD = $(MAKE) MX_ARCH=$(MX_ARCH) MX_INSTALL_DIR=$(MX_INSTALL_DIR)

build:
	for i in $(MODULES); do \
		cd $$i ; \
		$(MAKECMD) ; \
		if [ $$? != 0 ] ; then \
			exit 1 ; \
		fi ; \
		cd .. ; \
	done
	
clean:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) clean ) \
	done
	-rm */*.o */*.obj */*.mxo
	
distclean:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) distclean ) \
	done
	-rm */*.o */*.obj */*.mxo
	
depend:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) depend ) \
	done
	
install:
	for i in $(MODULES); do \
		( cd $$i ; $(MAKECMD) install ) \
	done
	
