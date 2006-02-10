/*
 * Name:    mx_xdr.h
 *
 * Purpose: Header for calling XDR functions from MX.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_XDR_H__
#define __MX_XDR_H__

#if defined(OS_WIN32) || defined(OS_DJGPP)
#   include "../tools/xdr/src/xdr.h"

#elif defined(OS_VMS)
#   include <tcpip$rpcxdr.h>

#else
#   if defined(__BORLANDC__)
         typedef long long int		int64_t;
         typedef unsigned long long int	uint64_t;
#   endif

#   include <limits.h>
#   include <rpc/types.h>
#   include <rpc/xdr.h>
#endif

#endif /* __MX_XDR_H__ */
