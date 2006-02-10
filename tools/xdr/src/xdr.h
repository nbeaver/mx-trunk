/*
 * Name:    xdr.h
 *
 * Purpose: Header file for an incomplete version of XDR for platforms that
 *          do not automatically include XDR.
 *
 * Author:  William Lavender
 *
 * Warning: Only those parts of XDR that are currently used by MX are
 *          actually implemented.  Please note that allocation and
 *          deallocation of memory are not supported.
 *
 *          At present, this package has only been tested on Win32 and
 *          DJGPP platforms, since they are the only MX-supported platforms
 *          that does not come bundled with a version of XDR.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __XDR_H__
#define __XDR_H__

#if defined(OS_WIN32) || defined(OS_DJGPP)
#  define MX_DATAFMT_NATIVE	MX_DATAFMT_LITTLE_ENDIAN
#else
#error Native data format not yet defined.
#endif

#if defined(OS_WIN32)
   typedef __int64 			quad_t;
   typedef unsigned __int64		u_quad_t;

#elif defined(OS_DJGPP)
   typedef long long			quad_t;

#  if !defined(HAVE_U_QUAD_T)	/* Avoid a collision with Watt-32. */
#     define HAVE_U_QUAD_T
      typedef unsigned long long	u_quad_t;
#  endif

#else
#error quad_t and u_quad_t not yet defined.
#endif

/* XDR_FREE is not implemented. */

enum xdr_op {
	XDR_ENCODE = 0,
	XDR_DECODE = 1
};

typedef int bool_t;

typedef struct {
	enum xdr_op x_op;

	char * xdr_buffer_address;
	char * xdr_data_pointer;
	u_int buffer_bytes_left;
} MX_XDR;

typedef bool_t (*xdrproc_t) (MX_XDR *, void *, ...);

#define XDR		MX_XDR

#define xdrmem_create	mx_xdrmem_create
#define xdr_destroy	mx_xdr_destroy

#define xdr_array	mx_xdr_array
#define xdr_string	mx_xdr_string

#define xdr_float	mx_xdr_float
#define xdr_double	mx_xdr_double

#define xdr_char	mx_xdr_char
#define xdr_u_char	mx_xdr_u_char
#define xdr_short	mx_xdr_short
#define xdr_u_short	mx_xdr_u_short
#define xdr_int		mx_xdr_int
#define xdr_u_int	mx_xdr_u_int
#define xdr_long	mx_xdr_long
#define xdr_u_long	mx_xdr_u_long
#define xdr_hyper	mx_xdr_hyper
#define xdr_u_hyper	mx_xdr_u_hyper

#define xdr_int8_t	mx_xdr_char
#define xdr_uint8_t	mx_xdr_u_char
#define xdr_int16_t	mx_xdr_short
#define xdr_uint16_t	mx_xdr_u_short
#define xdr_int32_t	mx_xdr_int
#define xdr_uint32_t	mx_xdr_u_int
#define xdr_int64_t	mx_xdr_hyper
#define xdr_uint64_t	mx_xdr_u_hyper

MX_API void mx_xdrmem_create( MX_XDR *xdrs,
			char *addr,
			u_int size,
			enum xdr_op op );

MX_API void mx_xdr_destroy( MX_XDR *xdrs );

MX_API bool_t mx_xdr_string( MX_XDR *xdrs,
			char **ptr_to_string_ptr,
			u_int max_string_size );

MX_API bool_t mx_xdr_array( MX_XDR *xdrs,
			char **array_ptr,
			u_int *num_elements_ptr,
			u_int maximum_num_elements,
			u_int element_size_in_bytes,
			xdrproc_t element_proc );

MX_API bool_t mx_xdr_opaque( MX_XDR *xdrs,
			char *opaque_ptr,
			u_int size_in_bytes );

MX_API bool_t mx_xdr_float( MX_XDR *xdrs, float *float_ptr );

MX_API bool_t mx_xdr_double( MX_XDR *xdrs, double *double_ptr );

MX_API bool_t mx_xdr_char( MX_XDR *xdrs, char *char_ptr );

MX_API bool_t mx_xdr_u_char( MX_XDR *xdrs, u_char *u_char_ptr );

MX_API bool_t mx_xdr_short( MX_XDR *xdrs, short *short_ptr );

MX_API bool_t mx_xdr_u_short( MX_XDR *xdrs, u_short *u_short_ptr );

MX_API bool_t mx_xdr_int( MX_XDR *xdrs, int *int_ptr );

MX_API bool_t mx_xdr_u_int( MX_XDR *xdrs, u_int *u_int_ptr );

MX_API bool_t mx_xdr_long( MX_XDR *xdrs, long *long_ptr );

MX_API bool_t mx_xdr_u_long( MX_XDR *xdrs, u_long *u_long_ptr );

MX_API bool_t mx_xdr_hyper( MX_XDR *xdrs, quad_t *quad_ptr );

MX_API bool_t mx_xdr_u_hyper( MX_XDR *xdrs, u_quad_t *u_quad_ptr );

#endif /* __XDR_H__ */

