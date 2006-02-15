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

#define xdr_hyper		mx_xdr_hyper
#define xdr_u_hyper		mx_xdr_u_hyper

#if defined(OS_QNX)
typedef long long		quad_t;
typedef unsigned long long	u_quad_t;
#endif

MX_API bool_t mx_xdr_hyper( XDR *xdrs, quad_t *quad_ptr );

MX_API bool_t mx_xdr_u_hyper( XDR *xdrs, u_quad_t *u_quad_ptr );

#endif /* __XDR_HYPER_H__ */

