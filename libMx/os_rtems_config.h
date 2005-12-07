/*
 * Name:    os_rtems_config.h
 *
 * Purpose: RTEMS specific parameters such as network configuration should
 *          be set here.  If you are not using RTEMS, then you do not need
 *          to do anything to this header file.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __OS_RTEMS_CONFIG_H__
#define __OS_RTEMS_CONFIG_H__

/*========== General definitions ==========*/

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 20
#define CONFIGURE_USE_IMFS_AS_BASE_FILESYSTEM

#define CONFIGURE_EXECUTIVE_RAM_SIZE	(512*1024)
#define CONFIGURE_MAXIMUM_SEMAPHORES	20
#define CONFIGURE_MAXIMUM_TASKS		20

#define CONFIGURE_MICROSECONDS_PER_TICK	10000

#define CONFIGURE_INIT_TASK_STACK_SPACE	(10*1024)
#define CONFIGURE_INIT_TASK_PRIORITY	100

/*========== Network definitions ==========*/

#define MX_RTEMS_IP_ADDRESS	"192.168.137.55"
#define MX_RTEMS_NETWORK_MASK	"255.255.255.0"
#define MX_RTEMS_HOST_NAME	"test1"
#define MX_RTEMS_DOMAIN_NAME	"home"
#define MX_RTEMS_SERVER_ADDRESS	"192.168.137.3"

/* Define the following if you want to use BOOTP instead. */

/* #define RTEMS_USE_BOOTP  */

/* i386
 *   PC486, PC686, etc. BSPs
 *
 * 	There are currently three supported network adapter types:
 *
 *	DEC Tulip family - BSP_DEC21140_NETWORK_DRIVER_...
 * 	NE2000           - BSP_NE2000_NETWORK_DRIVER_...
 * 	WD8003           - BSP_WD8003_NETWORK_DRIVER_...
 *
 * For real hardware, the Tulip cards are probably the best choice.
 * If you want to use the QEMU emulator, you should choose the NE2000
 * adapter type instead.
 *
 */

#if defined(MX_RTEMS_BSP_PC486)

#  define RTEMS_BSP_NETWORK_DRIVER_NAME    BSP_NE2000_NETWORK_DRIVER_NAME
#  define RTEMS_BSP_NETWORK_DRIVER_ATTACH  BSP_NE2000_NETWORK_DRIVER_ATTACH

#endif

/* m68k
 *   MVME162 BSP
 *
 * 	No special definitions are necessary for the MVME162.  However, one
 * 	must link with a truncated version of the libMx library, since the
 * 	complete set of drivers is too big to fit into the available memory
 * 	of the MVME162.
 */

/* powerpc
 *   MVME2307 BSP (tested on an MVME2700 with an MVME761)
 *
 *      No special definitions are necessary for the MVME2307.  However,
 *	you _must_ make the ENV command modifications described in the
 *	file c/src/lib/lib/libbsp/powerpc/motorola_powerpc/README.MVME2300.
 *	If you do not, the generated RTEMS binaries will crash while
 *	starting up.
 *
 */

#endif /* __OS_RTEMS_CONFIG_H__ */

