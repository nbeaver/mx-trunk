/*
 * Name:    mx_xdr.h
 *
 * Purpose: Header for calling XDR functions from MX.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2011, 2014-2018, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_XDR_H__
#define __MX_XDR_H__

#include "mx_private_version.h"

#if defined(OS_WIN32) || defined(OS_DJGPP) || defined(OS_ECOS) \
	|| defined(OS_ANDROID) || defined(OS_MINIX) || defined(__riscv)

#   include "../tools/xdr/src/xdr.h"
#   include "../tools/xdr/src/xdr_hyper.h"

#elif defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(__OpenBSD__) \
	|| defined(OS_UNIXWARE) || defined(MX_MUSL_VERSION)
#   include "../tools/xdr/src/xdr_hyper.h"

#elif defined(OS_VMS)
#   include <tcpip$rpcxdr.h>

#else
#   if defined(__BORLANDC__)
       typedef long long int		int64_t;
       typedef unsigned long long int	uint64_t;
#   endif

#   if defined(OS_MACOSX) || defined(OS_HPUX) || defined(OS_QNX)
#      define xdr_int8_t	xdr_char
#      define xdr_uint8_t	xdr_u_char
#      define xdr_int16_t	xdr_short
#      define xdr_uint16_t	xdr_u_short
#      define xdr_int32_t	xdr_int
#      define xdr_uint32_t	xdr_u_int
#      define xdr_int64_t	xdr_hyper
#      define xdr_uint64_t	xdr_u_hyper

#   elif defined(OS_CYGWIN) && ( MX_CYGWIN_VERSION < 1007006 )
#      define xdr_int8_t	xdr_char
#      define xdr_uint8_t	xdr_u_char
#      define xdr_int16_t	xdr_short
#      define xdr_uint16_t	xdr_u_short
#      define xdr_int32_t	xdr_int
#      define xdr_uint32_t	xdr_u_int
#      define xdr_int64_t	xdr_int64_t
#      define xdr_uint64_t	xdr_u_int64_t

#      define xdr_hyper		xdr_int64_t
#      define xdr_u_hyper	xdr_u_int64_t

#   endif

#   include <limits.h>
#   include <rpc/types.h>
#   include <rpc/xdr.h>

#   if defined(OS_MACOSX) && ( ! defined(__clang__) )
#      include "../tools/xdr/src/xdr_hyper.h"
#   endif

#   if defined(OS_QNX)
#      include "../tools/xdr/src/xdr_hyper.h"
#   endif

#   if defined(MX_GLIBC_VERSION) && (MX_GLIBC_VERSION < 2001000L)
#      include "../tools/xdr/src/xdr_hyper.h"
#   endif

#endif

#endif /* __MX_XDR_H__ */

