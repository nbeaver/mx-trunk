/*
 * Name:    mx_net_socket.h 
 *
 * Purpose: Functions for managing MX protocol connections that run on
 *          top of TCP network sockets.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NET_SOCKET_H__
#define __MX_NET_SOCKET_H__


MX_API mx_status_type mx_network_socket_receive_message( MX_SOCKET *mx_socket,
						double timeout,
						unsigned long buffer_length,
						void *buffer );

MX_API mx_status_type mx_network_socket_send_message( MX_SOCKET *mx_socket,
						double timeout,
						void *buffer );

MX_API mx_status_type mx_network_socket_send_error_message(MX_SOCKET *mx_socket,
						long return_message_type,
						mx_status_type error_message );

#endif /* __MX_NET_SOCKET_H__ */

