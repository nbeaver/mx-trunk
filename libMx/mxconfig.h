/*
 * mxconfig.h --- Header file for site dependent definitions.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MXCONFIG_H_
#define _MXCONFIG_H_

/* ======== Optional software packages. ======== */

/* None at present. */

/* ======== Optional hardware drivers. ======== */

/* The following devices use, in part, drivers that do not
 * come with the basic MX package, either because the
 * drivers are proprietary or because the drivers are
 * distributed by someone else.  For each case, we indicate
 * where the driver may be obtained from.
 */

/*****************************************************************************
 *
 * EPICS is a large experimental control system created originally
 * by Los Alamos National Laboratory and Argonne National Laboratory.
 * The EPICS home page may be found at http://www.aps.anl.gov/epics/.
 * If you do not know what it is, set HAVE_EPICS to 0.
 */

#define HAVE_EPICS			0

/*****************************************************************************
 *
 * Win32 and Linux library for using EPIX, Inc. video imaging boards.
 *
 * Website: http://www.epixinc.com/
 *
 * Note: EPIX and EPICS have absolutely nothing in common.  The naming is
 * just a coincidence.
 *
 */

#define HAVE_EPIX_XCLIB			0

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
 * The Delta Tau Power PMAC is a PowerPC-based motion controller that uses a
 * Linux-based realtime operating system.  This define enables support for
 * MX drivers running on the Power PMAC itself that communicate with the
 * Delta Tau motion control software there.
 *
 * WARNING: This driver is a prototype and has not yet been tested with a
 * real Power PMAC system.
 */

#define HAVE_POWER_PMAC			0

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
 * The DriverLINX Port I/O driver allows a user mode program running under
 * Windows NT to access Intel x86 I/O ports from 0x0100 to 0xFFFF.  This
 * driver was written by Scientific Software Tools, Inc. and may be downloaded
 * from http://www.sstnet.com/dnload/dnload.htm.  This driver also works
 * under Windows 95/98, but on those machines it is probably simpler to
 * just use the 'dos_portio' driver.
 */

#define HAVE_DRIVERLINX_PORTIO		0

/*****************************************************************************
 *
 * m68k Linux MX driver for controlling Oregon Microsystems VME-58 motor
 * controllers from a VME crate running Linux 2.0 on its system controller.
 * This driver uses a Linux device driver that is available from ESRF at
 * http://www.esrf.fr/computing/cs/sysadmin/rtk/emlinux/oregon.htm.
 * The ESRF driver is also supposed to support the PC-based OMS PC-58
 * and PC-68 motor controller cards, but I have not tested this yet.
 *
 * The source itself is at
 * ftp://ftp.esrf.fr/pub/cs/ess/linux/drivers/oms/oms-2.0.tgz
 */

#define HAVE_JORWAY_CAMAC		0

/*****************************************************************************
 *
 * Linux and Win32 drivers (sjyLX and sjyWIN) written by Fermilab for Jorway
 * CAMAC controllers (Jorway 73A, etc.)
 *
 */

#define HAVE_VME58_ESRF			0

/*****************************************************************************
 *
 * Win32 driver for the National Instruments stepper and servo boards
 * that were originally made by nuLogic.  This includes the ValueMotion
 * series of motor controllers.  The PCMOTION32.DLL and PCMOTION32.LIB
 * files required are normally provided with the board at the time of
 * purchase.
 */

#define HAVE_PCMOTION32			0

/*****************************************************************************
 *
 * Win32 driver for the Aerotech Unidex 500 series of motor controllers.
 */

#define HAVE_U500			1

/*****************************************************************************
 *
 * Win32 driver for the Precision MicroControl Motion Control API (MCAPI)
 * for their series of motor controllers.
 */

#define HAVE_PMC_MCAPI			0

/*****************************************************************************
 *
 * Win32 driver for the EG&G Ortec Unified MCB Interface for 32 Bits
 * programmer's toolkit (Part # A11-B32) under Microsoft Windows 95.
 * Note that this driver does not support the 16 bit version of the MCB
 * interface software.
 */

#define HAVE_ORTEC_UMCBI		0

/*****************************************************************************
 *
 * X-ray Instrumentation Associates Handel library for their DXP series of
 * multichannel analyzer systems.  The driver is downloadable from the page
 * http://www.xia.com/DXP_Software.html.  At this time, the corresponding
 * MX drivers have been successfully tested with the XIA Saturn (DXP-X10P).
 *
 */

#define HAVE_XIA_HANDEL			0

/*****************************************************************************
 *
 * X-ray Instrumentation Associates Xerxes library.  This is a lower level
 * XIA library that is invoked by Handel.  The HAVE_XIA_XERXES flag is
 * used to specify whether or not the Xerxes headers and export libraries
 * are available since not all modern Handel distributions include them.
 *
 */

#define HAVE_XIA_XERXES			0

#endif /* _MXCONFIG_H_ */

