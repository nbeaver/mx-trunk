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

#ifndef _MXCONFIG_H_
#define _MXCONFIG_H_

/**************************************************************************
 *                                                                        *
 * NOTICE: MX 1.5.5 is in the process of moving all of the optionally     *
 *         compiled drivers that traditionally are compiled into libMx    *
 *         into separate dynamically loaded modules with their source     *
 *         code in subdirectories of the mx/modules directory.  As each   *
 *         set of drivers moves to the mx/modules directory, the matching * 
 *         macros in mx/libMx/mxconfig.h and mx/libMx/Makehead.* are      *
 *         deleted.                                                       *
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
 *    epics                 Formerly enabled by HAVE_EPICS                *
 *    epix_xclib            Formerly enabled by HAVE_EPIX_XCLIB           *
 *    esone_camac           Formerly enabled by HAVE_JORWAY_CAMAC         *
 *    ni_valuemotion        Formerly enabled by HAVE_PCMOTION32           *
 *    ortec_umcbi           Formerly enabled by HAVE_ORTEC_UMCBI          *
 *    powerpmac             Formerly enabled by HAVE_POWERPMAC_LIBRARY    *
 *    u500                  Formerly enabled by HAVE_U500                 *
 *    xia_handel            Formerly enabled by HAVE_XIA_HANDEL           *
 *                                                                        *
 **************************************************************************/

/* ======== Optional hardware drivers. ======== */

/* The following devices use, in part, drivers that do not
 * come with the basic MX package, either because the
 * drivers are proprietary or because the drivers are
 * distributed by someone else.  For each case, we indicate
 * where the driver may be obtained from.
 */

/*****************************************************************************
 *
 * Win32, Linux, and Solaris library for using EDT video imaging boards.
 *
 * Website: http://www.edt.com/
 *
 */

#define HAVE_EDT			0

/*****************************************************************************
 *
 * Driver for National Intruments GPIB interface cards.
 *
 * The Linux version of this driver is downloadable from
 * http://www.ni.com/linux/
 */

#define HAVE_NI488			0

/*****************************************************************************
 *
 * Driver for the Linux GPIB driver which is available from
 * http://linux-gpib.sourceforge.net/.
 *
 * The Linux GPIB driver uses function names that are the same as many of
 * the functions in the National Instruments driver.  Because of this,
 * setting both HAVE_NI488 and HAVE_LINUX_GPIB to 1 is not allowed since
 * this would lead to name conflicts at link time.
 */

#define HAVE_LINUX_GPIB			0

/*****************************************************************************
 *
 * Drivers for the National Instruments implementation of the Virtual
 * Instrument System Architecture (VISA).  The NI-VISA package must
 * be purchased from National Instruments in order to use this driver.
 *
 * Thus far, this driver has only been tested with the Linux version of
 * NI-VISA, so I don't know if it works on other operating systems.
 */

#define HAVE_NI_VISA			0

/*****************************************************************************
 *
 * Driver for the Struck SIS1100/3100 PCI-to-VME bus interface.
 *
 * Thus far, only the Linux version of this driver has been tested.
 */

#define HAVE_SIS3100			0
 
/*****************************************************************************
 *
 * Libusb is a low level library for accessing USB devices.  It is currently
 * available on Linux, MacOS X, FreeBSD, NetBSD, OpenBSD, and Microsoft
 * Windows.  I am not sure that you can always depend on it being present
 * on those platforms, which is the reason for making this configurable.
 * More information may be found at http://libusb.sourceforge.net/ and,
 * for the Win32 version, at http://libusb-win32.sourceforge.net/.
 */

#define HAVE_LIBUSB			0
 
/*****************************************************************************
 *
 * Video4linux version 2 is a video input/output API for Linux.  All systems
 * based on the Linux 2.6 kernel should have it.  Some, but not all, Linux 2.4
 * systems have it as well.  Please note that this driver does not support 
 * the obsolete Video4linux version 1 API.
 */

#define HAVE_VIDEO_4_LINUX_2		0

/*****************************************************************************
 *
 * Linux driver for performing hardware port I/O from user mode to
 * a restricted range of ports without requiring that the user mode
 * program be setuid root or setgid kmem.  The current version (2.0)
 * of this driver runs under Linux 2.2.x and 2.4.x.  Use the older
 * version 0.3 if you need to run under Linux 2.0.x.
 *
 * Available at the MX web site as portio-2.0.tar.gz
 */

#define HAVE_LINUX_PORTIO		0

/*****************************************************************************
 *
 * Linux and Win32 drivers for the Precision MicroControl Motion Control API
 * (MCAPI) for their series of motor controllers.
 */

#define HAVE_PMC_MCAPI			0

#endif /* _MXCONFIG_H_ */

