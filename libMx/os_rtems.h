/*
 * Name:    os_rtems.h
 *
 * Purpose: Standard definitions for MX RTEMS programs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __OS_RTEMS_H__
#define __OS_RTEMS_H__

/*==== Include site dependent configuation file ====*/

#include "os_rtems_config.h"

/*==== Generally you should not have to change anything after this point ====*/

#include <bsp.h>
#include <rtems/tftp.h>

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_INITIAL_MODES ( RTEMS_PREEMPT | \
					RTEMS_NO_TIMESLICE | \
					RTEMS_NO_ASR | \
					RTEMS_INTERRUPT_LEVEL(0) )
#define CONFIGURE_INIT

rtems_task Init( rtems_task_argument argument );

#include <confdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <rtems/rtems_bsdnet.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*======== Stubs for unsupported functions. ========*/

FILE *popen( const char *command, const char *type )
{
	return NULL;
}

int pclose( FILE *stream )
{
	return 0;
}

/*=========== Setup network definitions ============*/

#if defined( RTEMS_SET_ETHERNET_ADDRESS )
static char ethernet_address[6] = MX_RTEMS_ETHERNET_ADDRESS;
#endif

/************* rtems_bsdnet_ifconfig ****************/

static struct rtems_bsdnet_ifconfig netdriver_config = {
	RTEMS_BSP_NETWORK_DRIVER_NAME,		/* Name */
	RTEMS_BSP_NETWORK_DRIVER_ATTACH,	/* Attach function */

	NULL,			/* No next interface.  We only have one. */

#if defined( RTEMS_USE_BOOTP )
	NULL,			/* BOOTP supplies IP address. */
	NULL,			/* BOOTP supplies IP network mask. */
#else
	MX_RTEMS_IP_ADDRESS,	/* IP address. */
	MX_RTEMS_NETWORK_MASK,	/* IP network mask. */
#endif

#if defined( RTEMS_SET_ETHERNET_ADDRESS )
	ethernet_address,	/* Ethernet hardware address. */
#else
	NULL,			/* Driver supplies the hardware address. */
#endif
	0			/* Use default driver parameters. */
};

/************** rtems_bsdnet_config *****************/

struct rtems_bsdnet_config rtems_bsdnet_config = {
	&netdriver_config,

#if defined( RTEMS_USE_BOOTP )
	rtems_bsdnet_do_bootp,
#else
	NULL,
#endif

	0,			/* Default network task priority. */
	0,			/* Default mbuf capacity. */
	0,			/* Default mbuf cluster capacity. */

#if ! defined( RTEMS_USE_BOOTP )
	MX_RTEMS_HOST_NAME,	/* Host name. */
	MX_RTEMS_DOMAIN_NAME,	/* Domain name. */
#if 0
	MX_RTEMS_GATEWAY,	/* Gateway. */
	MX_RTEMS_LOG_HOST	/* Log host. */
	MX_RTEMS_NAME_SERVERS,	/* Name server(s). */
	MX_RTEMS_NTP_SERVERS,	/* NTP server(s). */
#endif
#endif

};

#endif /* __OS_RTEMS_H__ */
