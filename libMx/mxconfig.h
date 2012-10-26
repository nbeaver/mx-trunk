/*
 * mxconfig.h --- Header file for site dependent definitions.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#error mxconfig.h is obsolete

/**************************************************************************
 *                                                                        *
 * NOTICE: MX 1.6.0 has moved all of the optionally compiled drivers that *
 *         traditionally are compiled into libMx into separate dynamically*
 *         loaded modules with their source code in subdirectories of the *
 *         mx/modules directory.  The matching macros in libMx/mxconfig.h *
 *         and mx/libMx/Makehead.* have been deleted.                     *
 *                                                                        *
 *         In support of this, new Makefile targets have been added to    *
 *         the top level makefile mx/Makefile.  These include             *
 *                                                                        *
 *             make modules-distclean                                     *
 *             make modules                                               *
 *             make modules-install                                       *
 *                                                                        *
 *         Commands like 'make' and 'make install' do _NOT_ automatically *
 *         build the corresponding module related targets.  Instead, you  *
 *         must build the modules after building the core libMx library.  *
 *                                                                        *
 *------------------------------------------------------------------------*
 *                                                                        *
 *         Using the EPIX XCLIB drivers as an example, all of the files   *
 *         in mx/libMx with *epix* in their names have now been moved to  *
 *         the new mx/modules/epix_xclib directory.  The HAVE_EPIX_XCLIB  *
 *         macro in mx/libMx/mxconfig.h has been deleted as well as all   *
 *         references to EPIX in the files mx/libMx/Makehead.linux and    *
 *         mx/libMx/Makehead.win32 have been deleted.                     *
 *                                                                        *
 *         In this new arrangement, you enable the EPIX XCLIB drivers by  *
 *         uncommenting the references to them in mx/modules/Makefile and *
 *         configure them in mx/modules/epix_xclib/Makefile.              *
 *                                                                        *
 *         You will also need to add a line that looks like this:         *
 *                                                                        *
 *             !include epix_xclib                                        *
 *                                                                        *
 *         to the top of any MX databases that make use of EPIX XCLIB     *
 *         -related drivers.                                              *
 *                                                                        *
 **************************************************************************/

/**************************************************************************
 * Sets of drivers that have been converted to modules include:           *
 *                                                                        *
 *    driverlinx_portio     Formerly enabled by HAVE_DRIVERLINX_PORTIO    *
 *    edt                   Formerly enabled by HAVE_EDT                  *
 *    epics                 Formerly enabled by HAVE_EPICS                *
 *    epix_xclib            Formerly enabled by HAVE_EPIX_XCLIB           *
 *    esone_camac           Formerly enabled by HAVE_JORWAY_CAMAC         *
 *    libusb-0.1            Formerly enabled by HAVE_LIBUSB               *
 *    linux_gpib            Formerly enabled by HAVE_LINUX_GPIB           *
 *    ni_valuemotion        Formerly enabled by HAVE_PCMOTION32           *
 *    ni488                 Formerly enabled by HAVE_NI488                *
 *    ortec_umcbi           Formerly enabled by HAVE_ORTEC_UMCBI          *
 *    pmc_mcapi             Formerly enabled by HAVE_PMC_MCAPI            *
 *    powerpmac             Formerly enabled by HAVE_POWERPMAC_LIBRARY    *
 *    sis3100               Formerly enabled by HAVE_SIS3100              *
 *    u500                  Formerly enabled by HAVE_U500                 *
 *    v4l2_input            Formerly enabled by HAVE_VIDEO_4_LINUX_2      *
 *    vxi_memacc            Formerly enabled by HAVE_NI_VISA              *
 *    xia_handel            Formerly enabled by HAVE_XIA_HANDEL           *
 *                                                                        *
 **************************************************************************/

