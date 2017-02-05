/*
 * Name:    mx_signal_alloc.h
 *
 * Purpose: Header file for managing the use of signals.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005, 2007, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SIGNAL_ALLOC_H__
#define __MX_SIGNAL_ALLOC_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXF_ANY_REALTIME_SIGNAL		(-1)

MX_API mx_status_type mx_signal_alloc_initialize( void );

MX_API int mx_signal_alloc_is_initialized( void );

MX_API mx_status_type mx_signal_allocate( int requested_signal_number,
					int *allocated_signal_number );

MX_API mx_status_type mx_signal_free( int signal_number );

/*---*/

MX_API int mx_maximum_signal_number( void );

MX_API mx_status_type mx_get_signal_allocation( int max_signals,
						int *mx_signal_array );

#ifdef __cplusplus
}
#endif

#endif /* __MX_SIGNAL_ALLOC_H__ */

