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
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __OS_RTEMS_CONFIG_H__
#define __OS_RTEMS_CONFIG_H__

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
 * Of these, the Tulip cards are probably the best choice.
 *
 */

#if 0
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
 * 	The MVME2307 BSP for some reason does not define a network driver
 *	in its <bsp.h> header file, so this must be done explicitly here.
 *
 *	Also, you _must_ make the ENV command modifications described in the
 *	file c/src/lib/lib/libbsp/powerpc/motorola_powerpc/README.MVME2300.
 *	If you do not, the generated RTEMS binaries will crash while
 *	starting up.
 *
 */

#if 1
   extern int
   rtems_dec21140_driver_attach( struct rtems_bsdnet_ifconfig *, int );

#  define RTEMS_BSP_NETWORK_DRIVER_NAME    "dc1"
#  define RTEMS_BSP_NETWORK_DRIVER_ATTACH  rtems_dec21140_driver_attach
#endif

#endif /* __OS_RTEMS_CONFIG_H__ */

