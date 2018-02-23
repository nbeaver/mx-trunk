/*
 * Name:    xdr_hyper.c
 *
 * Purpose: This file supplies the 64-bit xdr_hyper() and xdr_u_hyper()
 *          functions for those platforms whose XDR implemtations do not
 *          already include them such as old versions of MacOS X.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006, 2014-2016, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_socket.h"
#include "xdr_hyper.h"

#if defined(OS_WIN32) || defined(OS_ANDROID) || defined(OS_DJGPP) \
	|| defined(OS_ECOS) || defined(OS_MINIX) || defined(__riscv)
#  define XDR_GETLONG( xdrs, long_ptr )    mx_xdr_long( xdrs, long_ptr )
#  define XDR_PUTLONG( xdrs, long_ptr )    mx_xdr_long( xdrs, long_ptr )
#endif  /* OS_WIN32 */

MX_EXPORT bool_t
mx_xdr_hyper( XDR *xdrs, quad_t *quad_ptr )
{
	u_quad_t *u_quad_ptr;
	u_long u_long_array[2];
	bool_t status;

	switch( xdrs->x_op) {
	case XDR_ENCODE:
		u_quad_ptr = (u_quad_t *) quad_ptr;

		u_long_array[0] = (u_long) ((*u_quad_ptr >> 32) & 0xffffffff);
		u_long_array[1] = (u_long) ((*u_quad_ptr) & 0xffffffff);

		status = XDR_PUTLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[0] );

		if ( status == FALSE )
			return FALSE;

		status = XDR_PUTLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[1] );

		return status;
	case XDR_DECODE:
		status = XDR_GETLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[0] );

		if ( status == FALSE )
			return FALSE;

		status = XDR_GETLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[1] );

		if ( status == FALSE )
			return FALSE;

		*quad_ptr = (quad_t)( ( ((u_quad_t) u_long_array[0]) << 32 )
				      | ((u_quad_t) u_long_array[1]) );

		return TRUE;
	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}

MX_EXPORT bool_t
mx_xdr_u_hyper( XDR *xdrs, u_quad_t *u_quad_ptr )
{
	u_long u_long_array[2];
	bool_t status;

	switch( xdrs->x_op) {
	case XDR_ENCODE:
		u_long_array[0] = (u_long) ((*u_quad_ptr >> 32) & 0xffffffff);
		u_long_array[1] = (u_long) ((*u_quad_ptr) & 0xffffffff);

		status = XDR_PUTLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[0] );

		if ( status == FALSE )
			return FALSE;

		status = XDR_PUTLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[1] );

		return status;
	case XDR_DECODE:
		status = XDR_GETLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[0] );

		if ( status == FALSE )
			return FALSE;

		status = XDR_GETLONG( xdrs,
			(mx_xdr_getputlong_type *) &u_long_array[1] );

		if ( status == FALSE )
			return FALSE;

		*u_quad_ptr = (u_quad_t)( ( ((u_quad_t) u_long_array[0]) << 32 )
				          | ((u_quad_t) u_long_array[1]) );

		return TRUE;
	case XDR_FREE:
		return TRUE;
	}

	return FALSE;
}

