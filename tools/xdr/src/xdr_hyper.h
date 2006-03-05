/*
 * Name:    xdr_hyper.h
 *
 * Purpose: This header file supplies the 64-bit xdr_hyper() and xdr_u_hyper()
 *          functions for those platforms whose XDR implementation do not 
 *          already include them such as MacOS X.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __XDR_HYPER_H__
#define __XDR_HYPER_H__

#if defined(OS_WIN32) || defined(OS_DJGPP)
#  include "xdr.h"
#else
#  include <rpc/types.h>
#  include <rpc/xdr.h>
#endif

#define xdr_hyper		mx_xdr_hyper
#define xdr_u_hyper		mx_xdr_u_hyper

#if defined(OS_WIN32)
   typedef __int64		quad_t;
   typedef unsigned __int64	u_quad_t;

#elif defined(OS_QNX) || defined(OS_CYGWIN)
   typedef long long		quad_t;
   typedef unsigned long long	u_quad_t;

#elif defined(OS_DJGPP)
   typedef long long		quad_t;

#  if !defined(HAVE_U_QUAD_T)	/* To protect us from Watt-32. */
#     define HAVE_U_QUAD_T
      typedef unsigned long long	u_quad_t;
#  endif

#endif

MX_API bool_t mx_xdr_hyper( XDR *xdrs, quad_t *quad_ptr );

MX_API bool_t mx_xdr_u_hyper( XDR *xdrs, u_quad_t *u_quad_ptr );

#endif /* __XDR_HYPER_H__ */

