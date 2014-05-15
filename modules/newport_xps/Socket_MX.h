/*
 * Name:    Socket_MX.h
 *
 * Purpose: MX definitions needed to allow Socket.cpp to compile on
 *          non-Windows systems.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __SOCKET_MX_H__
#define __SOCKET_MX_H__

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_socket.h"

typedef mx_bool_type	BOOL;
typedef unsigned long	DWORD;

#define WSAEADDRINUSE		EADDRINUSE
#define WSAEADDRNOTAVAIL	EADDRNOTAVAIL
#define WSAEAFNOSUPPORT		EAFNOSUPPORT
#define WSAECONNREFUSED		ECONNREFUSED
#define WSAEDESTADDRREQ		EDESTADDRREQ
#define WSAEFAULT		EFAULT
#define WSAEINVAL		EINVAL
#define WSAEINPROGRESS		EINPROGRESS
#define WSAEISCONN		EISCONN
#define WSAEMFILE		EMFILE
#define WSAENETDOWN		ENETDOWN
#define WSAENETUNREACH		ENETUNREACH
#define WSAENOBUFS		ENOBUFS
#define WSAENOTSOCK		ENOTSOCK
#define WSAEPROTONOSUPPORT	EPROTONOSUPPORT
#define WSAEPROTOTYPE		EPROTOTYPE
#define WSAESOCKTNOSUPPORT	ESOCKTNOSUPPORT
#define WSAETIMEDOUT		ETIMEDOUT
#define WSAEWOULDBLOCK		EWOULDBLOCK

#define WSANOTINITIALIZED	(1000000L)

#endif /* __SOCKET_MX_H__ */
