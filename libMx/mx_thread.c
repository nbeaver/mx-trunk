/*
 * Name:    mx_thread.c
 *
 * Purpose: MX thread functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2007, 2010-2011, 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_THREAD_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_thread.h"

static volatile int mx_threads_are_initialized = FALSE;

/*********************** Microsoft Win32 **********************/

#if defined(OS_WIN32)

#include <windows.h>

#if defined(__BORLANDC__) || defined(__GNUC__)
#  include <process.h>
#endif

typedef struct {
	unsigned thread_id;
	HANDLE thread_handle;
	HANDLE stop_event_handle;
} MX_WIN32_THREAD_PRIVATE;

typedef struct {
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
} MX_WIN32_THREAD_ARGUMENTS_PRIVATE;

static DWORD mx_current_thread_index;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static mx_status_type
mx_thread_get_pointers( MX_THREAD *thread,
			MX_WIN32_THREAD_PRIVATE **thread_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_thread_get_pointers()";

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( thread_private == (MX_WIN32_THREAD_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_WIN32_THREAD_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_private;

	if ( (*thread_private) == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_THREAD_PRIVATE pointer for thread %p is NULL.",
			thread );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static HANDLE
mx_get_current_thread_handle( void )
{
	static const char fname[] = "mx_get_current_thread_handle()";

	HANDLE process_pseudo_handle;
	HANDLE thread_pseudo_handle, thread_real_handle;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	process_pseudo_handle = GetCurrentProcess();

	thread_pseudo_handle = GetCurrentThread();

	status = DuplicateHandle( process_pseudo_handle,
				thread_pseudo_handle,
				process_pseudo_handle,
				&thread_real_handle,
				0,
				FALSE,
				DUPLICATE_SAME_ACCESS );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to duplicate a handle for the current thread.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );

		return NULL;
	}

	return thread_real_handle;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_win32_thread_get_handle_and_id( MX_THREAD *thread )
{
	static const char fname[] = "mx_win32_thread_get_handle_and_id()";

	MX_WIN32_THREAD_PRIVATE *thread_private;

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_private;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_WIN32_THREAD_PRIVATE pointer for thread %p is NULL.",
			thread );
	}

	thread_private->thread_id = GetCurrentThreadId();

	thread_private->thread_handle = mx_get_current_thread_handle();

	if ( thread_private->thread_handle == NULL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get a handle for the initial thread." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static unsigned __stdcall
mx_thread_start_function( void *args_ptr )
{
#if MX_THREAD_DEBUG
	static const char fname[] = "mx_thread_start_function()";
#endif

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

	/* Save a thread-specific pointer to the MX_THREAD structure that
	 * can be returned by mx_get_current_thread().
	 */

	mx_status = mx_thread_save_thread_pointer( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return FALSE;

	/* Invoke MX's thread function. */

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));
#endif

	mx_status = (thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	_endthreadex( (int) mx_status.code );

	/* Should never get here. */

	return FALSE;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_initialize( void )
{
	static const char fname[] = "mx_thread_initialize()";

	MX_THREAD *thread;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Create a Thread Local Storage index that we will use to store a
	 * pointer to the current thread for each thread.
	 */

	mx_current_thread_index = TlsAlloc();

	if ( mx_current_thread_index == TLS_OUT_OF_INDEXES ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to allocate a Thread Local Storage index.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	/* Create and populate an MX_THREAD data structure for the
	 * program's initial thread.
	 */

	mx_status = mx_thread_build_data_structures( &thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_win32_thread_get_handle_and_id( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Record the fact that the initialization has been completed.
	 * Only one thread will be running at the time that the flag
	 * is set, so it is not necessary to use a mutex for the
	 * mx_threads_are_initialized flag.
	 */

	mx_threads_are_initialized = TRUE;

	/* Save a thread-specific pointer to the MX_THREAD structure that
	 * can be returned by mx_get_current_thread().
	 *
	 * This must happen _after_ the mx_threads_are_initialized flag
	 * is set to TRUE, since mx_thread_save_thread_pointer() will itself
	 * recursively invoke mx_thread_initialize() if the flag is still
	 * set to FALSE.
	 */

	mx_status = mx_thread_save_thread_pointer( thread );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_build_data_structures( MX_THREAD **thread )
{
	static const char fname[] = "mx_thread_build_data_structures()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	DWORD last_error_code;
	TCHAR message_buffer[100];


	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	/* Allocate the MX_THREAD structure. */

	*thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( *thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	(*thread)->stop_request_handler = NULL;
	(*thread)->stop_request_arguments = NULL;

	/* Allocate the private thread structure. */

	thread_private = (MX_WIN32_THREAD_PRIVATE *)
				malloc( sizeof(MX_WIN32_THREAD_PRIVATE) );

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_WIN32_THREAD_PRIVATE structure." );
	}
	
	(*thread)->thread_private = thread_private;
	(*thread)->stop_request_handler = NULL;

	/* Create an unsignaled event object with manual reset.  This
	 * object is used by other threads to request that the main
	 * thread shut itself down.
	 */

	thread_private->stop_event_handle =
				CreateEvent( NULL, TRUE, FALSE, NULL );

	if ( thread_private->stop_event_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to create a stop event object for main thread %p.  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_free_data_structures( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free_data_structures()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	status = CloseHandle( thread_private->stop_event_handle );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		/* Do not abort here.  We want to attempt to close the
		 * thread handle as well.
		 */

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close the stop event handle for thread %p. "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

	status = CloseHandle( thread_private->thread_handle );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		/* Do not abort here.  We want to free the MX_THREAD and
		 * MX_WIN32_THREAD_PRIVATE structures as well.
		 */

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to close the thread handle for thread %p. "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

	mx_free( thread_private );

	mx_free( thread );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_create( MX_THREAD **thread,
		MX_THREAD_FUNCTION *thread_function,
		void *thread_arguments )
{
	static const char fname[] = "mx_thread_create()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	MX_WIN32_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	unsigned thread_id;
	DWORD last_error_code, suspend_count;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,
	("%s invoked for thread_function = %p, thread_arguments = %p",
		fname, thread_function, thread_arguments));
#endif

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	mx_status = mx_thread_build_data_structures( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private = (MX_WIN32_THREAD_PRIVATE *) (*thread)->thread_private;

	thread_arg_struct = (MX_WIN32_THREAD_ARGUMENTS_PRIVATE *)
			malloc( sizeof(MX_WIN32_THREAD_ARGUMENTS_PRIVATE) );

	if ( thread_arg_struct == (MX_WIN32_THREAD_ARGUMENTS_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate memory for a MX_WIN32_THREAD_ARGUMENTS_PRIVATE struct.");
	}

	thread_arg_struct->thread = *thread;
	thread_arg_struct->thread_function = thread_function;
	thread_arg_struct->thread_arguments = thread_arguments;

	/* Create the thread in a suspended state. */

	thread_private->thread_handle =
			(HANDLE) _beginthreadex( NULL,
						0,
						mx_thread_start_function,
						thread_arg_struct,
						CREATE_SUSPENDED,
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

	/* Allow the thread to start executing. */

	suspend_count = ResumeThread( thread_private->thread_handle );

	if ( suspend_count == (DWORD)(-1) ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to resume thread %p.  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_thread_exit( MX_THREAD *thread,
		long thread_exit_status )
{
	static const char fname[] = "mx_thread_exit()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* Terminate the thread. */

	thread->thread_exit_status = thread_exit_status;

	_endthreadex( (int) thread_exit_status );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_kill( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_kill()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = TerminateThread( thread_private->thread_handle, -1 );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to kill thread %p.  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_stop( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_stop()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Nicely ask the thread to stop. */

	status = SetEvent( thread_private->stop_event_handle );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to stop thread %p.  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_check_for_stop_request( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_check_for_stop_request()";

	MX_THREAD_STOP_REQUEST_HANDLER *stop_request_handler;
	MX_WIN32_THREAD_PRIVATE *thread_private;
	DWORD wait_status, last_error_code;
	TCHAR message_buffer[100];
	int stop_requested;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_private;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

	stop_requested = FALSE;

	wait_status = WaitForSingleObject(
				thread_private->stop_event_handle, 0 );

	switch( wait_status ) {
	case WAIT_ABANDONED:
		return mx_error( MXE_OBJECT_ABANDONED, fname,
			"The stop event object for thread %p has been "
			"abandoned.  This should NEVER happen.", thread );
		break;
	case WAIT_OBJECT_0:
		stop_requested = TRUE;
		break;
	case WAIT_TIMEOUT:
		stop_requested = FALSE;
		break;
	case WAIT_FAILED:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Attempt to check for stop request failed.  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
		break;
	default:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error code from WaitForSingleObject().  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
		break;
	}

	/* If a stop has been requested, invoke the stop request handler. */

	if ( stop_requested ) {
		stop_request_handler = (MX_THREAD_STOP_REQUEST_HANDLER *)
						thread->stop_request_handler;

		if ( stop_request_handler != NULL ) {
			/* Invoke the handler. */

#if MX_THREAD_DEBUG
			MX_DEBUG(-2,("%s: Invoking the stop request handler.",
				fname));
#endif

			(stop_request_handler)( thread,
					thread->stop_request_arguments );
		}

		/* Terminate the thread. */

		(void) mx_thread_exit( thread, 0 );

		mx_warning( "%s: mx_thread_exit() returned to the "
			"calling routine.  This should not happen.", fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_set_stop_request_handler( MX_THREAD *thread,
			MX_THREAD_STOP_REQUEST_HANDLER *stop_request_handler,
			void *stop_request_arguments )
{
	static const char fname[] = "mx_thread_set_stop_request_handler()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	 thread->stop_request_handler = stop_request_handler;
	 thread->stop_request_arguments = stop_request_arguments;

	 return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_wait( MX_THREAD *thread,
		long *thread_exit_status,
		double max_seconds_to_wait )
{
	static const char fname[] = "mx_thread_wait()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD wait_status, dword_exit_status, last_error_code;
	DWORD milliseconds_to_wait;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( max_seconds_to_wait < 0.0 ) {
		milliseconds_to_wait = INFINITE;
	} else {
		milliseconds_to_wait = mx_round( 1000.0 * max_seconds_to_wait );
	}

	wait_status = WaitForSingleObject( thread_private->thread_handle,
					milliseconds_to_wait );

	switch( wait_status ) {
	case WAIT_ABANDONED:
		return mx_error( MXE_OBJECT_ABANDONED, fname,
			"The object for thread %p has been "
			"abandoned.  This should NEVER happen.", thread );
		break;
	case WAIT_OBJECT_0:
		/* The thread has terminated.  Do not return yet. */

		break;
	case WAIT_TIMEOUT:
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after %g seconds of waiting for thread %p "
			"to terminate.", max_seconds_to_wait, thread );
		break;
	case WAIT_FAILED:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Attempt to check for thread termination failed.  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
		break;
	default:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error code from WaitForSingleObject().  "
			"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
		break;
	}

	/* If requested, get the exit status for this thread. */

	if ( thread_exit_status != NULL ) {
		status = GetExitCodeThread( thread_private->thread_handle,
						&dword_exit_status );

		if ( status == 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to get exit status for thread %p.  "
				"Win32 error code = %ld, error_message = '%s'",
				thread, last_error_code, message_buffer );
		}

		*thread_exit_status = (long) dword_exit_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_save_thread_pointer( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_save_thread_pointer()";

	int status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Save a thread-specific pointer to the MX_THREAD structure using
	 * the Thread Local Storage created in mx_thread_initialize().
	 */

	status = TlsSetValue( mx_current_thread_index, thread );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to save the MX_THREAD pointer in "
			"thread local storage.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_get_current_thread( MX_THREAD **thread )
{
	static const char fname[] = "mx_get_current_thread()";

	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	/* A pointer to the MX_THREAD structure is stored in
	 * a thread-specific Thread Local Storage index.
	 */

	*thread = TlsGetValue( mx_current_thread_index );

	if ( (*thread) == (MX_THREAD *) NULL ) {
		last_error_code = GetLastError();

		if ( last_error_code != ERROR_SUCCESS ) {
			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unexpected error code from TlsGetValue().  "
				"Win32 error code = %ld, error_message = '%s'",
				last_error_code, message_buffer );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_show_thread_info( MX_THREAD *thread, char *message )
{
	static const char fname[] = "mx_show_thread_info()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	if ( (message != NULL) && (strlen(message) > 0) ) {
		mx_info( message );
	}

	mx_info( "  thread pointer             = %p", thread );
	mx_info( "  thread_private pointer     = %p", thread_private );
	mx_info( "  Win32 thread id            = %08lx",
				(unsigned long) thread_private->thread_id );
	mx_info( "  Win32 thread handle        = %08lx",
				(unsigned long) thread_private->thread_handle );
	mx_info( "  Win32 stop event handle    = %08lx",
			(unsigned long) thread_private->stop_event_handle );

	return;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT char *
mx_thread_id_string( char *buffer, size_t buffer_length )
{
	MX_THREAD *thread;

	if ( (buffer == NULL) || (buffer_length == 0) ) {
		fprintf( stderr,
		"(No buffer was supplied to write the thread id string to) ");

		return NULL;
	}

	thread = TlsGetValue( mx_current_thread_index );

	snprintf( buffer, buffer_length, "(MX thread = %p, Win32 ID = %08lx) ",
		thread, (unsigned long) GetCurrentThreadId() );

	return buffer;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_alloc( MX_THREAD_LOCAL_STORAGE **key )
{
	static const char fname[] = "mx_tls_alloc()";

	DWORD *tls_index_ptr;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( key == (MX_THREAD_LOCAL_STORAGE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}
*key = (MX_THREAD_LOCAL_STORAGE *) malloc( sizeof(MX_THREAD_LOCAL_STORAGE) );

	if ( (*key) == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate an MX_THREAD_LOCAL_STORAGE structure." );
	}

	tls_index_ptr = (DWORD *) malloc( sizeof(DWORD) );

	if ( tls_index_ptr == (DWORD *) NULL ) {
		mx_free( *key );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate a TLS index object." );
	}

	*tls_index_ptr = TlsAlloc();

	if ( (*tls_index_ptr) == TLS_OUT_OF_INDEXES ) {
		last_error_code = GetLastError();

		mx_free( *key );
		mx_free( tls_index_ptr );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create a new Thread Local Storage index.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	(*key)->tls_private = tls_index_ptr;

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_free( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_free()";

	DWORD *tls_index_ptr;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	BOOL status;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	tls_index_ptr = (DWORD *) key->tls_private;

	if ( tls_index_ptr == (DWORD *) NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			 "The Win32 Thread Local Storage index pointer for "
			"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );
	} else {
		status = TlsFree( *tls_index_ptr );

		if ( status != 0 ) {
			last_error_code = GetLastError();

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to free Thread Local Storage "
				"index %ld.  Win32 error code = %ld, "
				"error message = '%s'.",
					*tls_index_ptr,
					last_error_code, message_buffer );
		}
		mx_free( tls_index_ptr );
	}
	mx_free( key );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void *
mx_tls_get_value( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_get_value()";

	DWORD *tls_index_ptr;
	void *result;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );

		return NULL;
	}

	tls_index_ptr = (DWORD *) key->tls_private;

	if ( tls_index_ptr == (DWORD *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Thread Local Storage index pointer for "
		"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );

		return NULL;
	}

	result = TlsGetValue( *tls_index_ptr );

	if ( result == 0 ) {

		/* It is OK to store a 0 into a TLS key, so the only way to
		 * distinguish this from an error is to see if GetLastError()
		 * returns ERROR_SUCCESS, which means there was no error.
		 */

		last_error_code = GetLastError();

		if ( last_error_code != ERROR_SUCCESS ) {
			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to get the value from Thread Local "
				"Storage index %ld.  Win32 error code = %ld, "
				"error message = '%s'.",
					*tls_index_ptr,
					last_error_code, message_buffer );

			return NULL;
		}
	}

	return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_set_value( MX_THREAD_LOCAL_STORAGE *key, void *value )
{
	static const char fname[] = "mx_tls_set_value()";

	DWORD *tls_index_ptr;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	tls_index_ptr = (DWORD *) key->tls_private;

	if ( tls_index_ptr == (DWORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Thread Local Storage index pointer for "
		"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );
	}

	status = TlsSetValue( *tls_index_ptr, value );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to set the value of Thread Local Storage "
			"index %ld.  Win32 error code = %ld, "
			"error message = '%s'.",
			*tls_index_ptr, last_error_code, message_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*********************** Posix Pthreads **********************/

#elif defined(_POSIX_THREADS) || defined(OS_HPUX) || defined(__OpenBSD__)

/*---*/

/* FIXME: On VAX VMS, we get a mysterious error about redefinition of
 *        'struct timespec' apparently involving <decc_rtldef/timers.h>
 *        The following kludge works around that.
 */

#if defined(OS_VMS) && defined(__VAX) && !defined(_TIMESPEC_T_)
#  define _TIMESPEC_T_
#endif

/*---*/

#include <pthread.h>

typedef struct {
	pthread_t thread_id;
	int kill_requested;
} MX_POSIX_THREAD_PRIVATE;

typedef struct {
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
} MX_POSIX_THREAD_ARGUMENTS_PRIVATE;

static pthread_key_t mx_current_thread_key;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static mx_status_type
mx_thread_get_pointers( MX_THREAD *thread,
			MX_POSIX_THREAD_PRIVATE **thread_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_thread_get_pointers()";

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( thread_private == (MX_POSIX_THREAD_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POSIX_THREAD_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*thread_private = (MX_POSIX_THREAD_PRIVATE *) thread->thread_private;

	if ( (*thread_private) == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POSIX_THREAD_PRIVATE pointer for thread %p is NULL.",
			thread );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void *
mx_thread_start_function( void *args_ptr )
{
#if MX_THREAD_DEBUG
	static const char fname[] = "mx_thread_start_function()";
#endif

	MX_POSIX_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
	mx_status_type mx_status;

	if ( args_ptr == NULL ) {
		return NULL;
	}

	thread_arg_struct = (MX_POSIX_THREAD_ARGUMENTS_PRIVATE *) args_ptr;

	thread = thread_arg_struct->thread;
	thread_function = thread_arg_struct->thread_function;
	thread_arguments = thread_arg_struct->thread_arguments;

	mx_free( thread_arg_struct );
	
	if ( thread == NULL ) {
		return NULL;
	}

	if ( thread_function == NULL ) {
		return NULL;
	}

	/* Save a thread-specific pointer to the MX_THREAD structure that
	 * can be returned by mx_get_current_thread().
	 */

	mx_status = mx_thread_save_thread_pointer( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return NULL;

	/* Invoke MX's thread function. */

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));
#endif

	mx_status = (thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	(void) pthread_exit(NULL);

	/* Should never get here. */

	return NULL;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
mx_thread_stop_request_handler( void *arg )
{
	static const char fname[] = "mx_thread_stop_request_handler()";

	MX_THREAD *thread;
	MX_POSIX_THREAD_PRIVATE *thread_private;
	MX_THREAD_STOP_REQUEST_HANDLER *handler;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( arg == NULL ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The stop request handler was invoked with a NULL pointer "
		"as an argument rather than the MX_THREAD pointer that we "
		"were expecting.  The stop request routine will not be "
		"executed." );

		return;
	}

	thread = (MX_THREAD *) arg;

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	if ( thread_private->kill_requested ) {
#if MX_THREAD_DEBUG
		MX_DEBUG(-2,("%s: mx_thread_kill() was called, so the "
			"stop request handler will not be invoked.", fname ));
#endif

		return;
	}

	handler = thread->stop_request_handler;

	if ( handler == NULL ) {
#if MX_THREAD_DEBUG
		MX_DEBUG(-2,
("%s: No stop request handler was configured, so no handler will be executed.",
			fname));
#endif

		return;
	}

	/* Run the specified stop request handler. */

	(handler)( thread, thread->stop_request_arguments );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_initialize( void )
{
	static const char fname[] = "mx_thread_initialize()";

	MX_THREAD *thread;
	MX_POSIX_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Create a Pthread key that we will use to store a pointer to
	 * the current thread for each thread.
	 *
	 * The function mx_thread_stop_request_handler(), specified as
	 * the second argument, serves as the destructor function for
	 * this key.  mx_thread_stop_request_handler() ensures that
	 * the MX stop request handler will be invoked for each thread
	 * as it exits.
	 */

	status = pthread_key_create( &mx_current_thread_key,
					mx_thread_stop_request_handler );

	if ( status != 0 ) {
		switch( status ) {
		case EAGAIN:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	"Not enough resources available to create a new thread specific key." );

		case ENOMEM:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a new thread specific key." );

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
		"pthread_key_create() returned an unknown error code %d.",
				status );
		}
	}

	/* Create and populate an MX_THREAD data structure for the
	 * program's initial thread.
	 */

	mx_status = mx_thread_build_data_structures( &thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private = (MX_POSIX_THREAD_PRIVATE *) thread->thread_private;

	thread_private->thread_id = pthread_self();

	thread_private->kill_requested = FALSE;

	/* Record the fact that the initialization has been completed.
	 * Only one thread will be running at the time that the flag
	 * is set, so it is not necessary to use a mutex for the
	 * mx_threads_are_initialized flag.
	 */

	mx_threads_are_initialized = TRUE;

	/* Save a thread-specific pointer to the MX_THREAD structure that
	 * can be returned by mx_get_current_thread().
	 *
	 * This must happen _after_ the mx_threads_are_initialized flag
	 * is set to TRUE, since mx_thread_save_thread_pointer() will itself
	 * recursively invoke mx_thread_initialize() if the flag is still
	 * set to FALSE.
	 */

	mx_status = mx_thread_save_thread_pointer( thread );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_build_data_structures( MX_THREAD **thread )
{
	static const char fname[] = "mx_thread_build_data_structures()";

	MX_POSIX_THREAD_PRIVATE *thread_private;

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	/* Allocate the MX_THREAD structure. */

	*thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( *thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	/* Allocate the private thread structure. */

	thread_private = (MX_POSIX_THREAD_PRIVATE *)
				malloc( sizeof(MX_POSIX_THREAD_PRIVATE) );

	if ( thread_private == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_POSIX_THREAD_PRIVATE structure." );
	}
	
	thread_private->kill_requested = FALSE;

	(*thread)->thread_private = thread_private;
	(*thread)->thread_exit_status = -1;
	(*thread)->stop_request_handler = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* mx_thread_free_data_structures() frees all of the data structures allocated
 * for the thread.
 */

MX_EXPORT mx_status_type
mx_thread_free_data_structures( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free_data_structures()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	int pthread_status;
	mx_status_type mx_status;

	thread_private = NULL;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The thread key 'mx_current_thread_key' has a destructor function
	 * called mx_thread_stop_request_handler() that normally will be
	 * invoked when the thread exits.  We do not want that destructor
	 * function to be called after mx_thread_free_data_structures()
	 * has been invoked, since at that point all of the thread pointers
	 * are invalid pointers and may cause segmentation faults.
	 * We can prevent the destructor from being called by setting
	 * 'mx_current_thread_key' to NULL.
	 */

	pthread_status = pthread_setspecific( mx_current_thread_key, NULL );

	MXW_SUPPRESS_SET_BUT_NOT_USED( pthread_status );

#if 1
	/* FIXME - FIXME - FIXME:  Commenting out these two statements
	 * stops the MX server from crashing under Electric Fence, but
	 * it results in a memory leak!
	 */

	mx_free( thread_private );

	mx_free( thread );
#else
	mx_free_pointer( thread_private );

	mx_free_pointer( thread );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_create( MX_THREAD **thread,
		MX_THREAD_FUNCTION *thread_function,
		void *thread_arguments )
{
	static const char fname[] = "mx_thread_create()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	MX_POSIX_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	int status;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,
	("%s invoked for thread_function = %p, thread_arguments = %p",
		fname, thread_function, thread_arguments));
#endif

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_thread_build_data_structures( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private = (MX_POSIX_THREAD_PRIVATE *) (*thread)->thread_private;

	thread_arg_struct = (MX_POSIX_THREAD_ARGUMENTS_PRIVATE *)
			malloc( sizeof(MX_POSIX_THREAD_ARGUMENTS_PRIVATE) );

	if ( thread_arg_struct == (MX_POSIX_THREAD_ARGUMENTS_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate memory for a MX_POSIX_THREAD_ARGUMENTS_PRIVATE struct.");
	}

	thread_arg_struct->thread = *thread;
	thread_arg_struct->thread_function = thread_function;
	thread_arg_struct->thread_arguments = thread_arguments;

	status = pthread_create( &(thread_private->thread_id), NULL, 
				mx_thread_start_function,
				thread_arg_struct );

	if ( status != 0 ) {
		switch( status ) {
		case EAGAIN:
			return mx_error( MXE_TRY_AGAIN, fname,
				"At the moment, there are not enough system "
				"resources to create a thread." );

		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The thread attributes passed to "
				"pthread_create() were invalid." );
#if defined(OS_SOLARIS)
		case -1:
			return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR,fname,
			"pthread_create() returned -1.  If this program "
			"is running on Solaris 9 or before, a return value "
			"of -1 means that you did not include -lpthread "
			"in the linker command line used to build libMx." );
#endif
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_create() returned an unknown error code %d.",
				status );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_thread_exit( MX_THREAD *thread,
		long thread_exit_status )
{
	static const char fname[] = "mx_thread_exit()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* Terminate the thread. */

	thread->thread_exit_status = thread_exit_status;

	/* FIXME: All the documentation I see says that the right thing to
	 *        for sending the exit status to pthread_exit() is to cast
	 *        the integer value to a void pointer.  If this is really
	 *        true, then pthread_exit() is a badly designed API.
	 *
	 *        We could malloc() a structure here to contain the exit
	 *        status, but if nobody ever called mx_thread_wait() to
	 *        find out what the exit status was, then doing this would
	 *        result in a memory leak.  Any suggestions?  (WML)
	 */

	(void) pthread_exit( (void *) thread_exit_status );	/* ICK! */
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_kill( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_kill()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

	thread_private = NULL;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private->kill_requested = TRUE;

	status = pthread_cancel( thread_private->thread_id );

	if ( status != 0 ) {
		switch( status ) {
		case ESRCH:
			return mx_error( MXE_NOT_FOUND, fname,
				"The requested thread was not found." );

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_cancel() returned an unknown error code %d.",
				status );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_stop( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_stop()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

	thread_private = NULL;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = pthread_cancel( thread_private->thread_id );

	if ( status != 0 ) {
		switch( status ) {
		case ESRCH:
			return mx_error( MXE_NOT_FOUND, fname,
				"The requested thread was not found." );

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_cancel() returned an unknown error code %d.",
				status );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_check_for_stop_request( MX_THREAD *thread )
{
#if MX_THREAD_DEBUG
	static const char fname[] = "mx_thread_check_for_stop_request()";

	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	pthread_testcancel();

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_set_stop_request_handler( MX_THREAD *thread,
			MX_THREAD_STOP_REQUEST_HANDLER *stop_request_handler,
			void *stop_request_arguments )
{
	static const char fname[] = "mx_thread_set_stop_request_handler()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	 thread->stop_request_handler = stop_request_handler;
	 thread->stop_request_arguments = stop_request_arguments;

	 return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_wait( MX_THREAD *thread,
		long *thread_exit_status,
		double max_seconds_to_wait )
{
	static const char fname[] = "mx_thread_wait()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	void *value_ptr;
	int status;
	mx_status_type mx_status;

	thread_private = NULL;

	/* FIXME: Currently we pay no attention to 'max_seconds_to_wait'. */

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = pthread_join( thread_private->thread_id, &value_ptr );

	if ( status != 0 ) {
		switch( status ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The requested thread is not joinable." );

		case ESRCH:
			return mx_error( MXE_NOT_FOUND, fname,
				"The requested thread was not found." );

		case EDEADLK:
			return mx_error( MXE_MIGHT_CAUSE_DEADLOCK, fname,
				"Waiting for the requested thread might "
				"cause a deadlock." );

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_join() returned an unknown error code %d.",
				status );
		}
	}

	if ( thread_exit_status != NULL ) {
		/* FIXME: Casting a pointer to an integer is horrid, but look
		 *        at the comment at the end of mx_thread_exit() for
		 *        why we are currently doing this.  (WML)
		 */

		*thread_exit_status = (long) value_ptr;		/* ICK! */
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_save_thread_pointer( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_save_thread_pointer()";

	int status;
	mx_status_type mx_status;

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Save a thread specific pointer to the MX_THREAD structure using
	 * the Pthreads key created in mx_thread_initialize().
	 */

	status = pthread_setspecific( mx_current_thread_key, thread );

	if ( status != 0 ) {
		switch( status ) {
		case ENOMEM:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Insufficient memory is available to associate "
			"the MX_THREAD pointer with the current thread key." );

		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invalid Pthread key specified for pthread_setspecific()." );

		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
		"pthread_setspecific() returned an unknown error code %d.",
				status );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_get_current_thread( MX_THREAD **thread )
{
	static const char fname[] = "mx_get_current_thread()";

	mx_status_type mx_status;

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	/* A pointer to the MX_THREAD structure is stored in
	 * a thread-specific Pthreads key.
	 */

	*thread = pthread_getspecific( mx_current_thread_key );

	if ( (*thread) == (MX_THREAD *) NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to get a pointer to the MX_THREAD structure "
		"for this thread failed for an unknown reason." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_show_thread_info( MX_THREAD *thread, char *message )
{
	static const char fname[] = "mx_show_thread_info()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

	thread_private = NULL;

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	if ( (message != NULL) && (strlen(message) > 0) ) {
		mx_info( "%s", message );
	}

	mx_info( "  thread pointer         = %p", thread );
	mx_info( "  thread_private pointer = %p", thread_private );
	mx_info( "  Posix thread id        = %lx",
				(unsigned long) thread_private->thread_id );

	return;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT char *
mx_thread_id_string( char *buffer, size_t buffer_length )
{
	MX_THREAD *thread;

	if ( (buffer == NULL) || (buffer_length == 0) ) {
		fprintf( stderr,
		"(No buffer was supplied to write the thread id string to) ");

		return NULL;
	}

	thread = pthread_getspecific( mx_current_thread_key );

	snprintf( buffer, buffer_length, "(MX thread = %p, pthread ID = %lx) ",
		thread, (unsigned long) pthread_self() );

	return buffer;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_alloc( MX_THREAD_LOCAL_STORAGE **key )
{
	static const char fname[] = "mx_tls_alloc()";

#if defined(__OpenBSD__)
	void *pthread_key_ptr;
#else
	pthread_key_t *pthread_key_ptr;
#endif
	int status;

	if ( key == (MX_THREAD_LOCAL_STORAGE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	*key = (MX_THREAD_LOCAL_STORAGE *)
			malloc( sizeof(MX_THREAD_LOCAL_STORAGE) );

	if ( (*key) == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate an MX_THREAD_LOCAL_STORAGE structure." );
	}

	pthread_key_ptr = malloc( sizeof(pthread_key_t) );

	if ( pthread_key_ptr == NULL ) {
		mx_free( *key );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate a pthread_key_t object." );
	}

	status = pthread_key_create( pthread_key_ptr, NULL );

	if ( status != 0 ) {
		mx_free( *key );
		mx_free( pthread_key_ptr );

		switch( status ) {
		case EAGAIN:

			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Unable to create a Pthread key since either the system "
		"lacked the necessary resources to create the key or the "
		"PTHREAD_KEYS_MAX limit on the total number of keys per "
		"process has been exceeded." );

		case ENOMEM:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Insufficient memory available to create a new Pthread key." );

		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error returned by pthread_key_create().  "
			"Status = %d, error message = '%s'.",
				status, strerror(status) );
		}
	}

	(*key)->tls_private = pthread_key_ptr;

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_free( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_free()";

	pthread_key_t *pthread_key_ptr;
	int status;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	pthread_key_ptr = (pthread_key_t *) key->tls_private;

	if ( pthread_key_ptr == (pthread_key_t *) NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			 "The Pthread key pointer for "
			"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );
	} else {
		status = pthread_key_delete( *pthread_key_ptr );

		switch( status ) {
		case 0:
			/* Success */
			break;
		case EINVAL:
			mx_status = mx_error( MXE_NOT_FOUND, fname,
			"The specified Pthread key %lu did not exist.",
				(unsigned long) *pthread_key_ptr );
			break;
		default:
			mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error returned by pthread_key_delete().  "
			"Status = %d, error message = '%s'.",
				status, strerror(status) );
			break;
		}
		mx_free( key->tls_private );
	}
	mx_free( key );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void *
mx_tls_get_value( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_get_value()";

	pthread_key_t *pthread_key_ptr;
	void *result;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );

		return NULL;
	}

	pthread_key_ptr = (pthread_key_t *) key->tls_private;

	if ( pthread_key_ptr == (pthread_key_t *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Pthread key pointer for MX_THREAD_LOCAL_STORAGE "
		"pointer %p was NULL.", key );

		return NULL;
	}

	result = pthread_getspecific( *pthread_key_ptr );

	return result;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_set_value( MX_THREAD_LOCAL_STORAGE *key, void *value )
{
	static const char fname[] = "mx_tls_set_value()";

	pthread_key_t *pthread_key_ptr;
	int status;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	pthread_key_ptr = (pthread_key_t *) key->tls_private;

	if ( pthread_key_ptr == (pthread_key_t *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Pthread key pointer for MX_THREAD_LOCAL_STORAGE "
		"pointer %p was NULL.", key );
	}

	status = pthread_setspecific( *pthread_key_ptr, value );

	MXW_SUPPRESS_SET_BUT_NOT_USED( status );

	return MX_SUCCESSFUL_RESULT;
}

/*********************** VxWorks **********************/

#elif defined(OS_VXWORKS)

#include "taskLib.h"
#include "taskVarLib.h"
#include "intLib.h"
#include "smObjLib.h"

typedef struct {
	int task_id;
} MX_VXWORKS_THREAD_PRIVATE;

typedef struct {
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
} MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE;

static int mx_current_task_variable = 0;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static mx_status_type
mx_thread_get_pointers( MX_THREAD *thread,
			MX_VXWORKS_THREAD_PRIVATE **thread_private,
			const char *calling_fname )
{
	static const char fname[] = "mx_thread_get_pointers()";

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed by '%s' is NULL.",
			calling_fname );
	}
	if ( thread_private == (MX_VXWORKS_THREAD_PRIVATE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VXWORKS_THREAD_PRIVATE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*thread_private = (MX_VXWORKS_THREAD_PRIVATE *) thread->thread_private;

	if ( (*thread_private) == (MX_VXWORKS_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VXWORKS_THREAD_PRIVATE pointer for thread %p is NULL.",
			thread );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static int
mx_thread_start_function( int arg1, int arg2, int arg3, int arg4, int arg5,
			int arg6, int arg7, int arg8, int arg9, int arg10 )
{
#if MX_THREAD_DEBUG
	static const char fname[] = "mx_thread_start_function()";
#endif

	MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
	mx_status_type mx_status;

	if ( arg1 == 0 ) {
		return FALSE;
	}

	/* FIXME: Converting an integer to a pointer?  Oh, the horror! */

	thread_arg_struct = (MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE *) arg1;
	
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

	/* Save a thread-specific pointer to the MX_THREAD structure that
	 * can be returned by mx_get_current_thread().
	 */

	mx_status = mx_thread_save_thread_pointer( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return FALSE;

	/* Invoke MX's thread function. */

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));
#endif

	mx_status = (thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	exit( (int) mx_status.code );

	/* Should never get here. */

	return FALSE;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_initialize( void )
{
	static const char fname[] = "mx_thread_initialize()";

	MX_THREAD *thread;
	int status, current_task_id;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	/* Create a VxWorks task variable that we will use to store a
	 * pointer to the current thread for each thread.
	 */

	current_task_id = taskIdSelf();

	status = taskVarAdd( current_task_id, &mx_current_task_variable );

	if ( status != OK ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to create the initial task variable "
		"for task ID %d.", taskIdSelf() );
	}

	/* Create and populate an MX_THREAD data structure for the
	 * program's initial thread.
	 */

	mx_status = mx_thread_build_data_structures( &thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Record the fact that the initialization has been completed.
	 * Only one thread will be running at the time that the flag
	 * is set, so it is not necessary to use a mutex for the
	 * mx_threads_are_initialized flag.
	 */

	mx_threads_are_initialized = TRUE;

	/* Save a thread-specific pointer to the MX_THREAD structure that
	 * can be returned by mx_get_current_thread().
	 *
	 * This must happen _after_ the mx_threads_are_initialized flag
	 * is set to TRUE, since mx_thread_save_thread_pointer() will itself
	 * recursively invoke mx_thread_initialize() if the flag is still
	 * set to FALSE.
	 */

	mx_status = mx_thread_save_thread_pointer( thread );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_build_data_structures( MX_THREAD **thread )
{
	static const char fname[] = "mx_thread_build_data_structures()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	/* Allocate the MX_THREAD structure. */

	*thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( *thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	(*thread)->stop_request_handler = NULL;
	(*thread)->stop_request_arguments = NULL;

	/* Allocate the private thread structure. */

	thread_private = (MX_VXWORKS_THREAD_PRIVATE *)
				malloc( sizeof(MX_VXWORKS_THREAD_PRIVATE) );

	if ( thread_private == (MX_VXWORKS_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_VXWORKS_THREAD_PRIVATE structure.");
	}
	
	(*thread)->thread_private = thread_private;
	(*thread)->stop_request_handler = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_free_data_structures( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free_data_structures()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	/* FIXME: Not sure what we are supposed to do here. */

	mx_free( thread_private );

	mx_free( thread );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_create( MX_THREAD **thread,
		MX_THREAD_FUNCTION *thread_function,
		void *thread_arguments )
{
	static const char fname[] = "mx_thread_create()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	int priority, stack_size, thread_arg, saved_errno;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,
	("%s invoked for thread_function = %p, thread_arguments = %p",
		fname, thread_function, thread_arguments));
#endif

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	mx_status = mx_thread_build_data_structures( thread );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private = (MX_VXWORKS_THREAD_PRIVATE *)
				(*thread)->thread_private;

	thread_arg_struct = (MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE *)
			malloc( sizeof(MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE) );

	if ( thread_arg_struct == (MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Unable to allocate memory for a MX_VXWORKS_THREAD_ARGUMENTS_PRIVATE struct.");
	}

	thread_arg_struct->thread = *thread;
	thread_arg_struct->thread_function = thread_function;
	thread_arg_struct->thread_arguments = thread_arguments;

	/**** Create the thread. ****/
	 
	/* FIXME: What priority should I use? */

	priority = 0;
	stack_size = 20000;

	/* FIXME: Casting a pointer to an integer gives me bad karma. */

	thread_arg = (int) thread_arg_struct;

	/* FIXME: Is there a chance for a race condition here?  What happens
	 * if someone tries to start using thread_private->task_id before
	 * its value has been set?
	 *
	 * If we can find out what the pTcb and pStackBase arguments to
	 * taskInit() should be, then we can use taskInit() and 
	 * taskActivate() so that we can more safely store the value of
	 * thread_private->task_id.
	 */

	thread_private->task_id = taskSpawn( NULL, priority, VX_FP_TASK,
			stack_size, mx_thread_start_function,
			thread_arg, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

	if ( thread_private->task_id == ERROR ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case S_intLib_NOT_ISR_CALLABLE:
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		    "Cannot spawn a task from an interrupt service routine." );
		    	break;
		case S_smObjLib_NOT_INITIALIZED:
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The shared memory object library is not initialized.");
			break;
		case S_memLib_NOT_ENOUGH_MEMORY:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to spawn a new task." );
			break;
		case S_objLib_OBJ_ID_ERROR:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"S_objLib_OBJ_ID_ERROR seen when trying to "
				"spawn a new task." );
			break;
		case S_memLib_BLOCK_ERROR:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
				"S_memLib_BLOCK_ERROR seen when trying to "
				"spawn a new task." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
				"Unexpected errno value %d returned when "
				"trying to spawn a new task.", saved_errno );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_thread_exit( MX_THREAD *thread,
		long thread_exit_status )
{
	static const char fname[] = "mx_thread_exit()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	/* Terminate the thread. */

	exit( (int) thread_exit_status );
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_suspend( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_suspend()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = taskSuspend( thread_private->task_id );

	if ( status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to suspend VxWorks task %#x",
			thread_private->task_id );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_resume( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_resume()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = taskResume( thread_private->task_id );

	if ( status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to resume VxWorks task %#x",
			thread_private->task_id );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_kill( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_kill()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = taskDeleteForce( thread_private->task_id );

	if ( status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to delete task %#x",
				thread_private->task_id );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_stop( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_stop()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	int status;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = taskDelete( thread_private->task_id );

	if ( status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to delete task %#x",
				thread_private->task_id );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_check_for_stop_request( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_check_for_stop_request()";

	MX_THREAD_STOP_REQUEST_HANDLER *stop_request_handler;
	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	int stop_requested;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	thread_private = (MX_VXWORKS_THREAD_PRIVATE *) thread->thread_private;

	if ( thread_private == (MX_VXWORKS_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

	/* Not sure that there is a way to do this under VxWorks. */

	stop_requested = FALSE;

	/* If a stop has been requested, invoke the stop request handler. */

	if ( stop_requested ) {
		stop_request_handler = (MX_THREAD_STOP_REQUEST_HANDLER *)
						thread->stop_request_handler;

		if ( stop_request_handler != NULL ) {
			/* Invoke the handler. */

#if MX_THREAD_DEBUG
			MX_DEBUG(-2,("%s: Invoking the stop request handler.",
				fname));
#endif

			(stop_request_handler)( thread,
					thread->stop_request_arguments );
		}

		/* Terminate the thread. */

		(void) mx_thread_exit( thread, 0 );

		mx_warning( "%s: mx_thread_exit() returned to the "
			"calling routine.  This should not happen.", fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_set_stop_request_handler( MX_THREAD *thread,
			MX_THREAD_STOP_REQUEST_HANDLER *stop_request_handler,
			void *stop_request_arguments )
{
	static const char fname[] = "mx_thread_set_stop_request_handler()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	 thread->stop_request_handler = stop_request_handler;
	 thread->stop_request_arguments = stop_request_arguments;

	 return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_wait( MX_THREAD *thread,
		long *thread_exit_status,
		double max_seconds_to_wait )
{
	static const char fname[] = "mx_thread_wait()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	WIND_TCB *task_tcb;
	STATUS status;
	unsigned long i, wait_ms, max_attempts;
	double max_attempts_d;
	mx_status_type mx_status;

#if MX_THREAD_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	wait_ms = 10;

	max_attempts_d = 1.0 + mx_divide_safely( max_seconds_to_wait,
						(double) wait_ms );

	max_attempts = mx_round( max_attempts_d );

	/* Wait for the task to terminate. */

	for ( i = 0; i < max_attempts; i++ ) {
		status = taskIdVerify( thread_private->task_id );

		if ( status == ERROR ) {
			/* The task no longer exists, so exit the for() loop. */

			break;
		}
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after %g seconds of waiting for thread %p "
			"to terminate.", max_seconds_to_wait, thread );
	}

	/* If requested, get the exit status for this thread. */

	if ( thread_exit_status != NULL ) {
		/* Since the thread has exited, it should be OK to look
		 * at the task's TCB.
		 *
		 * FIXME: Does the task TCB still exist at this point?
		 */

		task_tcb = taskTcb( thread_private->task_id );

		if ( task_tcb == (WIND_TCB *) NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"VxWorks task id %#x is invalid.",
				thread_private->task_id );
		}

		*thread_exit_status = (long) task_tcb->exitCode;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_save_thread_pointer( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_save_thread_pointer()";

	int status, thread_int;
	mx_status_type mx_status;

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Save a thread-specific pointer to the MX_THREAD structure using
	 * the VxWorks task variable created in mx_thread_initialize().
	 */

	/* FIXME: Casting a pointer to an integer can't be good. */

	thread_int = (int) thread;	

	status = taskVarSet( taskIdSelf(),
			&mx_current_task_variable, thread_int );

	if ( status != OK ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to save the MX_THREAD pointer failed." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_get_current_thread( MX_THREAD **thread )
{
	static const char fname[] = "mx_get_current_thread()";

	int task_variable_value;
	mx_status_type mx_status;

	if ( mx_threads_are_initialized == FALSE ) {
		mx_status = mx_thread_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( thread == (MX_THREAD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	task_variable_value = taskVarGet( taskIdSelf(),
					&mx_current_task_variable );

	if ( task_variable_value == ERROR ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unable to get a pointer to the current MX_THREAD structure.");
	}

	/* FIXME: Casting an integer to a pointer is a bad thing. */

	*thread = (MX_THREAD *) task_variable_value;

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void
mx_show_thread_info( MX_THREAD *thread, char *message )
{
	static const char fname[] = "mx_show_thread_info()";

	MX_VXWORKS_THREAD_PRIVATE *thread_private;
	mx_status_type mx_status;

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	if ( (message != NULL) && (strlen(message) > 0) ) {
		mx_info( message );
	}

	mx_info( "  thread pointer             = %p", thread );
	mx_info( "  thread_private pointer     = %p", thread_private );
	mx_info( "  VxWorks task id            = %#x",
						thread_private->task_id );
	return;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT char *
mx_thread_id_string( char *buffer, size_t buffer_length )
{
	MX_THREAD *thread;

	if ( (buffer == NULL) || (buffer_length == 0) ) {
		fprintf( stderr,
		"(No buffer was supplied to write the thread id string to) ");

		return NULL;
	}

	/* FIXME: Casting an integer to a pointer is a bad thing. */

	thread = (MX_THREAD *) taskVarGet( taskIdSelf(),
					&mx_current_task_variable );

	snprintf( buffer, buffer_length, "(MX thread = %p, task ID = %#x) ",
		thread, taskIdSelf() );

	return buffer;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_alloc( MX_THREAD_LOCAL_STORAGE **key )
{
	static const char fname[] = "mx_tls_alloc()";

	int *tls_variable_ptr;
	STATUS status;

	if ( key == (MX_THREAD_LOCAL_STORAGE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	*key = (MX_THREAD_LOCAL_STORAGE *)
			malloc( sizeof(MX_THREAD_LOCAL_STORAGE) );

	if ( (*key) == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate an MX_THREAD_LOCAL_STORAGE structure." );
	}

	tls_variable_ptr = (int *) malloc( sizeof(int) );

	if ( tls_variable_ptr == (int *) NULL ) {
		mx_free( *key );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate a TLS variable object." );
	}

	status = taskVarAdd( taskIdSelf(), tls_variable_ptr );

	if ( status != OK ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate a task variable for task %d.",
			taskIdSelf() );
	}

	(*key)->tls_private = tls_variable_ptr;

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_free( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_free()";

	int *tls_variable_ptr;
	int status;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	tls_variable_ptr = (int *) key->tls_private;

	if ( tls_variable_ptr == (int *) NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			 "The VxWorks variable pointer for "
			"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );
	} else {
		status = taskVarDelete( taskIdSelf(), tls_variable_ptr );

		if ( status != OK ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The MX_THREAD_LOCAL_STORAGE pointer %p "
			"does not correspond to a known task variable "
			"for VxWorks task %#x.", key, taskIdSelf() );
		}
		mx_free( tls_variable_ptr );
	}
	mx_free( key );

	return mx_status;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT void *
mx_tls_get_value( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_get_value()";

	int *tls_variable_ptr;
	int result_int;
	void *result_ptr;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );

		return NULL;
	}

	tls_variable_ptr = (int *) key->tls_private;

	if ( tls_variable_ptr == (int *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Thread Local Storage index pointer for "
		"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );

		return NULL;
	}

	result_int = taskVarGet( taskIdSelf(), tls_variable_ptr );

	if ( result_int == ERROR ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
		"The requested VxWorks task variable for "
		"Thread Local Storage %p is not found.", key );
	}

	/* FIXME: All this casting of integers to pointers makes me ill. */

	result_ptr = (void *) result_int;

	return result_ptr;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_tls_set_value( MX_THREAD_LOCAL_STORAGE *key, void *value )
{
	static const char fname[] = "mx_tls_set_value()";

	int *tls_variable_ptr;
	int value_int;
	STATUS status;

	if ( key == (MX_THREAD_LOCAL_STORAGE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD_LOCAL_STORAGE pointer passed was NULL." );
	}

	tls_variable_ptr = (int *) key->tls_private;

	if ( tls_variable_ptr == (int *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The VxWorks task variable pointer for "
		"MX_THREAD_LOCAL_STORAGE pointer %p was NULL.", key );
	}

	/* FIXME: Casting a pointer to an integer is bad news. */

	value_int = (int) value;

	status = taskVarSet( taskIdSelf(), key->tls_private, value_int );

	if ( status != OK ) {
		(void) mx_error( MXE_NOT_FOUND, fname,
		"The requested VxWorks task variable for "
		"Thread Local Storage %p is not found.", key );
	}

	return MX_SUCCESSFUL_RESULT;
}

/********** Use the following stubs when threads are not supported **********/

#elif defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_thread_initialize( void )
{
	static const char fname[] = "mx_thread_initialize()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_build_data_structures( MX_THREAD **thread )
{
	static const char fname[] = "mx_thread_build_data_structures()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_free_data_structures( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free_data_structures()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_create( MX_THREAD **thread,
		MX_THREAD_FUNCTION *thread_function,
		void *thread_arguments )
{
	static const char fname[] = "mx_thread_create()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT void
mx_thread_exit( MX_THREAD *thread,
		long thread_exit_status )
{
	static const char fname[] = "mx_thread_exit()";

	(void) mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_kill( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_kill()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_stop( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_stop()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_check_for_stop_request( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_check_for_stop_request()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_set_stop_request_handler( MX_THREAD *thread,
			MX_THREAD_STOP_REQUEST_HANDLER *stop_request_handler,
			void *stop_request_arguments )
{
	static const char fname[] = "mx_thread_set_stop_request_handler()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_wait( MX_THREAD *thread,
		long *thread_exit_status,
		double max_seconds_to_wait )
{
	static const char fname[] = "mx_thread_wait()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_thread_save_thread_pointer( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_save_thread_pointer()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_get_current_thread( MX_THREAD **thread )
{
	static const char fname[] = "mx_get_current_thread()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT void
mx_show_thread_info( MX_THREAD *thread, char *message )
{
	static const char fname[] = "mx_show_thread_info()";

	(void) mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT char *
mx_thread_id_string( char *buffer, size_t buffer_length )
{
	static const char fname[] = "mx_thread_id_string()";

	(void) mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );

	return NULL;
}

MX_EXPORT mx_status_type
mx_tls_alloc( MX_THREAD_LOCAL_STORAGE **key )
{
	static const char fname[] = "mx_tls_alloc()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT mx_status_type
mx_tls_free( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_free()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

MX_EXPORT void *
mx_tls_get_value( MX_THREAD_LOCAL_STORAGE *key )
{
	static const char fname[] = "mx_tls_get_value()";

	(void) mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );

	return NULL;
}

MX_EXPORT mx_status_type
mx_tls_set_value( MX_THREAD_LOCAL_STORAGE *key, void *value )
{
	static const char fname[] = "mx_tls_set_value()";

	return mx_error( MXE_UNSUPPORTED, fname,
		"Threads are not supported on this platform." );
}

/*-------------------------------------------------------------------------*/

#else

#error MX thread functions have not yet been defined for this platform.

#endif

/******************* Cross platform functions ******************/

MX_EXPORT MX_THREAD *
mx_get_current_thread_pointer( void )
{
	MX_THREAD *thread;
	mx_status_type mx_status;

	mx_status = mx_get_current_thread( &thread );

	if ( mx_status.code != MXE_SUCCESS ) {
		return NULL;
	} else {
		return thread;
	}
}

