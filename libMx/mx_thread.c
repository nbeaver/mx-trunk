/*
 * Name:    mx_thread.c
 *
 * Purpose: MX thread functions.
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

#include <stdio.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_thread.h"

#if defined(OS_WIN32)

#include <windows.h>

typedef struct {
	HANDLE thread_handle;
	unsigned thread_id;
} MX_WIN32_THREAD_PRIVATE;

typedef struct {
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
} MX_WIN32_THREAD_ARGUMENTS_PRIVATE;

static unsigned __stdcall
mx_thread_start_function( void *args_ptr )
{
	MX_WIN32_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
	mx_status_type mx_status;

	if ( args_ptr == NULL ) {
		return FALSE;
	}

	thread_arg_struct = (MX_WIN32_THREAD_ARGUMENTS_PRIVATE *) args_ptr;

	
	thread = thread_arg_struct->thread;
	thread_function = thread_arg_struct->thread_function;
	thread_arguments = thread_arg_struct->thread_arguments;

	mx_free( thread_arg_struct );
	
	if ( thread == NULL ) {
		return FALSE;
	}

	if ( thread_function == NULL ) {
		return FALSE;
	}

	/* Invoke MX's thread function. */

	mx_status = (*thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	_endthreadex( (int) mx_status.code );

	/* Should never get here. */

	return FALSE;
}


MX_EXPORT mx_status_type
mx_thread_create( MX_THREAD **thread,
		MX_THREAD_FUNCTION *thread_function,
		void *thread_arguments )
{
	static const char fname[] = "mx_thread_create()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	MX_WIN32_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	unsigned thread_id;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	*thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( *thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *)
				malloc( sizeof(MX_WIN32_THREAD_PRIVATE) );

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_WIN32_THREAD_PRIVATE structure." );
	}
	
	(*thread)->thread_ptr = thread_private;

	thread_arg_struct = (MX_WIN32_THREAD_ARGUMENTS_PRIVATE *)
			malloc( sizeof(MX_WIN32_THREAD_ARGUMENTS_PRIVATE) );

	if ( thread_arg_struct == (MX_WIN32_THREAD_ARGUMENTS_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate memory for a MX_WIN32_THREAD_ARGUMENTS_PRIVATE struct.");
	}

	thread_arg_struct->thread = *thread;
	thread_arg_struct->thread_function = thread_function;
	thread_arg_struct->thread_arguments = thread_arguments;

	thread_private->thread_handle =
			(HANDLE) _beginthreadex( NULL,
						0,
						mx_thread_start_function,
						thread_arg_struct,
						0,
						&thread_id );

	if ( thread_private->thread_handle == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create a thread.  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
	}

	thread_private->thread_id = thread_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_thread_end( MX_THREAD *thread,
		int thread_exit_status )
{
	static const char fname[] = "mx_thread_end()";

	MX_WIN32_THREAD_PRIVATE *thread_private;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );

		return;
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");

		return;
	}

	/* Terminate the thread. */

	thread->thread_exit_status = thread_exit_status;

	_endthreadex( thread_exit_status );
}

MX_EXPORT mx_status_type
mx_thread_free( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free()";

	MX_WIN32_THREAD_PRIVATE *thread_private;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

	mx_free( thread_private );

	mx_free( thread );

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(_POSIX_THREADS)

#include <pthread.h>

typedef struct {
	pthread_t thread_id;
} MX_POSIX_THREAD_PRIVATE;

typedef struct {
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
} MX_POSIX_THREAD_ARGUMENTS_PRIVATE;

static void *
mx_thread_start_function( void *args_ptr )
{
	MX_POSIX_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
	mx_status_type mx_status;

	if ( args_ptr == NULL ) {
		return FALSE;
	}

	thread_arg_struct = (MX_POSIX_THREAD_ARGUMENTS_PRIVATE *) args_ptr;

	
	thread = thread_arg_struct->thread;
	thread_function = thread_arg_struct->thread_function;
	thread_arguments = thread_arg_struct->thread_arguments;

	mx_free( thread_arg_struct );
	
	if ( thread == NULL ) {
		return FALSE;
	}

	if ( thread_function == NULL ) {
		return FALSE;
	}

	/* Invoke MX's thread function. */

	mx_status = (*thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	(void) pthread_exit(NULL);

	/* Should never get here. */

	return FALSE;
}


MX_EXPORT mx_status_type
mx_thread_create( MX_THREAD **thread,
		MX_THREAD_FUNCTION *thread_function,
		void *thread_arguments )
{
	static const char fname[] = "mx_thread_create()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	MX_POSIX_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	pthread_t thread_id;
	int status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	*thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( *thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	thread_private = (MX_POSIX_THREAD_PRIVATE *)
				malloc( sizeof(MX_POSIX_THREAD_PRIVATE) );

	if ( thread_private == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_POSIX_THREAD_PRIVATE structure." );
	}
	
	(*thread)->thread_ptr = thread_private;

	thread_arg_struct = (MX_POSIX_THREAD_ARGUMENTS_PRIVATE *)
			malloc( sizeof(MX_POSIX_THREAD_ARGUMENTS_PRIVATE) );

	if ( thread_arg_struct == (MX_POSIX_THREAD_ARGUMENTS_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate memory for a MX_POSIX_THREAD_ARGUMENTS_PRIVATE struct.");
	}

	thread_arg_struct->thread = *thread;
	thread_arg_struct->thread_function = thread_function;
	thread_arg_struct->thread_arguments = thread_arguments;

	status = pthread_create( &thread_id, NULL, 
				mx_thread_start_function,
				thread_arg_struct );

	if ( status != 0 ) {
		switch( status ) {
		case EAGAIN:
			return mx_error( MXE_TRY_AGAIN, fname,
				"At the moment, there are not enough system "
				"resources to create a thread." );
			break;
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The thread attributes passed to "
				"pthread_create() were invalid." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_create() returned an unknown error code %d.",
				status );
			break;
		}
	}

	thread_private->thread_id = thread_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_thread_end( MX_THREAD *thread,
		int thread_exit_status )
{
	static const char fname[] = "mx_thread_end()";

	MX_POSIX_THREAD_PRIVATE *thread_private;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );

		return;
	}

	thread_private = (MX_POSIX_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");

		return;
	}

	/* Terminate the thread. */

	thread->thread_exit_status = thread_exit_status;

	(void) pthread_exit(NULL);
}

MX_EXPORT mx_status_type
mx_thread_free( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free()";

	MX_POSIX_THREAD_PRIVATE *thread_private;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_POSIX_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

	mx_free( thread_private );

	mx_free( thread );

	return MX_SUCCESSFUL_RESULT;
}


/*-------------------------------------------------------------------------*/

#else

#error MX thread functions have not yet been defined for this platform.

#endif

