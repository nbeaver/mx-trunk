/*
 * Name:    Socket_MX.h
 *
 * Purpose: MX functions for controlling Newport XPS controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SOCKET_MX_H__
#define __SOCKET_MX_H__

#ifdef __cplusplus
extern "C" {
#endif

int    mxp_newport_xps_get_comm_debug_flag( void );
void   mxp_newport_xps_set_comm_debug_flag( int debug_flag );
double mxp_newport_xps_get_comm_delay( void );
void   mxp_newport_xps_set_comm_delay( double delay );

#ifdef __cplusplus
}
#endif

#endif /* __SOCKET_MX_H__ */

