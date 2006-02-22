/*
 * Name:    xdr.c
 *
 * Purpose: This is an incomplete version of XDR for platforms that do not
 *          automatically include XDR.
 *
 * Author:  William Lavender
 *
 * Warning: Only those parts of XDR that are currently used by MX are
 *          actually implemented.  Please note that allocation and
 *          deallocation of memory are not supported.
 *
 *          At present, this package has only been tested on Win32 and
 *          DJGPP platforms, since they are the only MX-supported platforms
 *          that do not come bundled with a version of XDR.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_bit.h"
#include "mx_socket.h"

#include "xdr.h"

MX_EXPORT void
mx_xdrmem_create( MX_XDR *xdrs,
		char *xdr_buffer_address,
		u_int maximum_buffer_size,
		enum xdr_op op )
{
	xdrs->x_op = op;

	xdrs->xdr_buffer_address = xdr_buffer_address;

	xdrs->xdr_data_pointer = xdrs->xdr_buffer_address;
	xdrs->buffer_bytes_left = maximum_buffer_size;
}

MX_EXPORT void
mx_xdr_destroy( MX_XDR *xdrs )
{
	/* Nothing to do for an mx_xdrmem_create() created stream. */
}

MX_EXPORT bool_t
mx_xdr_string( MX_XDR *xdrs,
		char **ptr_to_string_ptr,
		u_int max_string_size )
{
	char *string_ptr;
	u_int string_size;
	bool_t status;

	/* This cut down version of xdr_string() does not allocate memory. */

	if ( ptr_to_string_ptr == NULL )
		return FALSE;

	string_ptr = *ptr_to_string_ptr;

	if ( string_ptr == NULL )
		return FALSE;

	/* The string length is at the start of the array. */

	if ( xdrs->x_op == XDR_ENCODE ) {
		string_size = strlen( string_ptr );
	}

	status = mx_xdr_u_int( xdrs, &string_size );

	if ( status == FALSE )
		return FALSE;

	/* Make sure we are null terminated. */

	if ( xdrs->x_op == XDR_DECODE ) {
		string_ptr[ string_size ] = '\0';
	}

	status = mx_xdr_opaque( xdrs, string_ptr, string_size );

	return status;
}

MX_EXPORT bool_t
mx_xdr_array( XDR *xdrs,
		char **array_ptr,
		u_int *num_elements_ptr,
		u_int maximum_num_elements,
		u_int element_size_in_bytes,
		xdrproc_t element_proc )
{
	u_int i, num_elements;
	char *element_ptr;
	bool_t status;

	/* This cut down version of xdr_array() does not allocate memory. */

	if ( array_ptr == NULL )
		return FALSE;

	/* The array length is at the start of the array. */

	status = mx_xdr_u_int( xdrs, num_elements_ptr );

	if ( status == FALSE )
		return FALSE;

	num_elements = *num_elements_ptr;

	if ( num_elements > maximum_num_elements )
		return FALSE;

	/* Process each element of the array with XDR. */

	element_ptr = *array_ptr;

	for ( i = 0; i < num_elements; i++ ) {
		status = (*element_proc)(xdrs, element_ptr, UINT_MAX);

		if ( status == FALSE )
			return FALSE;

		element_ptr += element_size_in_bytes;
	}

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_opaque( MX_XDR *xdrs, char *opaque_ptr, u_int size_in_bytes )
{
	char *xdr_ptr;
	u_int i, total_size_in_bytes, num_null_bytes, remainder;

	xdr_ptr = xdrs->xdr_data_pointer;

	/* If 'size_in_bytes' is not a multiple of 4, the end of the XDR
	 * buffer must be padded with nulls out to a multiple of 4 bytes
	 * in size.
	 */

	remainder = size_in_bytes % 4;

	if ( remainder == 0 ) {
		total_size_in_bytes = size_in_bytes;
		num_null_bytes = 0;
	} else {
		total_size_in_bytes = 4 * ( 1 + size_in_bytes / 4 );
		num_null_bytes = 4 - remainder;
	}

	if ( total_size_in_bytes > xdrs->buffer_bytes_left )
		return FALSE;

	if ( xdrs->x_op == XDR_ENCODE ) {

		for ( i = 0; i < size_in_bytes; i++ ) {
			xdr_ptr[i] = opaque_ptr[i];
		}

		xdr_ptr += size_in_bytes;

		for ( i = 0; i < num_null_bytes; i++ ) {
			xdr_ptr[i] = '\0';
		}
	} else {	/* XDR_DECODE */

		for ( i = 0; i < size_in_bytes; i++ ) {
			opaque_ptr[i] = xdr_ptr[i];
		}
	}

	xdrs->xdr_data_pointer  += total_size_in_bytes;
	xdrs->buffer_bytes_left -= total_size_in_bytes;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_char( MX_XDR *xdrs, char *char_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *char_ptr );
	} else {
		*char_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_u_char( MX_XDR *xdrs, u_char *u_char_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *u_char_ptr );
	} else {
		*u_char_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_short( MX_XDR *xdrs, short *short_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *short_ptr );
	} else {
		*short_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_u_short( MX_XDR *xdrs, u_short *u_short_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *u_short_ptr );
	} else {
		*u_short_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_int( MX_XDR *xdrs, int *int_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *int_ptr );
	} else {
		*int_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_u_int( MX_XDR *xdrs, u_int *u_int_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *u_int_ptr );
	} else {
		*u_int_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_long( MX_XDR *xdrs, long *long_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( (u_long) *long_ptr );
	} else {
		*long_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_u_long( MX_XDR *xdrs, u_long *u_long_ptr )
{
	u_long *xdr_ptr;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	if ( xdrs->x_op == XDR_ENCODE ) {
		*xdr_ptr = htonl( *u_long_ptr );
	} else {
		*u_long_ptr = ntohl( *xdr_ptr );
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

MX_EXPORT bool_t
mx_xdr_float( MX_XDR *xdrs, float *float_ptr )
{
	u_long *xdr_ptr, *u_long_ptr;
	u_long u_long_temp;

	if ( xdrs->buffer_bytes_left < 4 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	/* The binary bit pattern for the floating point number is
	 * transmitted as if it were a 32-bit unsigned integer.
	 */

	if ( xdrs->x_op == XDR_ENCODE ) {

		if ( sizeof(float) == sizeof(long) ) {

			u_long_ptr = (u_long *) float_ptr;

			*xdr_ptr = htonl( *u_long_ptr );

		} else if ( sizeof(float) == sizeof(int) ) {

			u_long_temp = *(int *) float_ptr;

			*xdr_ptr = htonl( u_long_temp );
		} else {
			return FALSE;
		}
	} else {	/* XDR_DECODE */

		if ( sizeof(float) == sizeof(long) ) {

			u_long_ptr = (u_long *) float_ptr;

			*u_long_ptr = ntohl( *xdr_ptr );

		} else if ( sizeof(float) == sizeof(int) ) {

			u_long_temp = ntohl( *xdr_ptr );

			*(int *) float_ptr = u_long_temp;
		} else {
			return FALSE;
		}
	}

	xdrs->xdr_data_pointer  += 4;
	xdrs->buffer_bytes_left -= 4;

	return TRUE;
}

#if MX_DATAFMT_NATIVE == MX_DATAFMT_LITTLE_ENDIAN
#  define HIGH_LONGWORD	1
#  define LOW_LONGWORD	0
#else
#  define HIGH_LONGWORD	0
#  define LOW_LONGWORD	1
#endif

MX_EXPORT bool_t
mx_xdr_double( MX_XDR *xdrs, double *double_ptr )
{
	u_long *xdr_ptr, *u_long_ptr;
	u_long u_long_temp_array[2];
	u_int *u_int_ptr;

	if ( xdrs->buffer_bytes_left < 8 )
		return FALSE;

	xdr_ptr = (u_long *) xdrs->xdr_data_pointer;

	/* The binary bit patterns for the high order and low order halves
	 * of the double value are transmitted as if they were 32-bit
	 * unsigned integers with the high order half being sent first.
	 */

	if ( xdrs->x_op == XDR_ENCODE ) {

		if ( 2*sizeof(long) == sizeof(double) ) {

			u_long_ptr = (u_long *) double_ptr;

			xdr_ptr[0] = htonl( u_long_ptr[ HIGH_LONGWORD ] );
			xdr_ptr[1] = htonl( u_long_ptr[ LOW_LONGWORD ] );

		} else if ( 2*sizeof(int) == sizeof(double) ) {

			u_int_ptr = (u_int *) double_ptr;

			u_long_temp_array[0] = u_int_ptr[ HIGH_LONGWORD ];
			u_long_temp_array[1] = u_int_ptr[ LOW_LONGWORD ];

			xdr_ptr[0] = htonl( u_long_temp_array[0] );
			xdr_ptr[1] = htonl( u_long_temp_array[1] );

		} else {
			return FALSE;
		}

	} else {	/* XDR_DECODE */

		if ( 2*sizeof(long) == sizeof(double) ) {

			u_long_ptr = (u_long *) double_ptr;

			u_long_ptr[ HIGH_LONGWORD ] = ntohl( xdr_ptr[0] );
			u_long_ptr[ LOW_LONGWORD ]  = ntohl( xdr_ptr[1] );

		} else if ( 2*sizeof(int) == sizeof(double) ) {

			u_int_ptr = (u_int *) double_ptr;

			u_long_temp_array[0] = ntohl( xdr_ptr[0] );
			u_long_temp_array[1] = ntohl( xdr_ptr[1] );

			u_int_ptr[ HIGH_LONGWORD ] = u_long_temp_array[0];
			u_int_ptr[ LOW_LONGWORD ]  = u_long_temp_array[1];

		} else {
			return FALSE;
		}
	}

	xdrs->xdr_data_pointer  += 8;
	xdrs->buffer_bytes_left -= 8;

	return TRUE;
}

