/*
 * Name:    mx_thread.h
 *
 * Purpose: Header file for MX thread functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_THREAD_H__
#define __MX_THREAD_H__

typedef struct {
	long thread_exit_status;

	void *thread_ptr;
} MX_THREAD;

/* Define a macro for use by mx_thread_wait() to signify that the wait
 * should last forever.
 */

#define MX_THREAD_INFINITE_WAIT		(-1.0)

typedef mx_status_type (MX_THREAD_FUNCTION)( MX_THREAD *, void * );

MX_API mx_status_type mx_thread_initialize( void );

MX_API mx_status_type mx_thread_create( MX_THREAD **thread,
					MX_THREAD_FUNCTION thread_function,
					void *thread_arguments );

MX_API void mx_thread_exit( MX_THREAD *thread, long thread_exit_status );

MX_API mx_status_type mx_thread_free( MX_THREAD *thread );

MX_API mx_status_type mx_thread_kill( MX_THREAD *thread );

MX_API mx_status_type mx_thread_wait( MX_THREAD *thread,
					long *thread_exit_status,
					double max_seconds_to_wait );

MX_API mx_status_type mx_get_current_thread( MX_THREAD **thread );

MX_API void mx_show_thread_info( MX_THREAD *thread, char *message );

#endif /* __MX_THREAD_H__ */

