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
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_thread.h"

static volatile int mx_threads_are_initialized = FALSE;

/*********************** Microsoft Win32 **********************/

#if defined(OS_WIN32)

#include <windows.h>

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
	static const char fname[] = "mx_thread_start_function()";

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

	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));

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
	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

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
	mx_status_type mx_status;


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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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
			thread, last_error_code, message_buffer );
		break;
	default:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error code from WaitForSingleObject().  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
		break;
	}

	/* If a stop has been requested, invoke the stop request handler. */

	if ( stop_requested ) {
		stop_request_handler = (MX_THREAD_STOP_REQUEST_HANDLER *)
						thread->stop_request_handler;

		if ( stop_request_handler != NULL ) {
			/* Invoke the handler. */

			MX_DEBUG(-2,("%s: Invoking the stop request handler.",
				fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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
			"to terminate.", max_seconds_to_wait );
		break;
	case WAIT_FAILED:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Attempt to check for thread termination failed.  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
		break;
	default:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unexpected error code from WaitForSingleObject().  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
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
				thread, last_error_code, message_buffer );
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
	mx_info( "  Win32 thread id            = %lu",
				(unsigned long) thread_private->thread_id );
	mx_info( "  Win32 thread handle        = %lu",
				(unsigned long) thread_private->thread_handle );
	mx_info( "  Win32 stop event handle    = %lu",
			(unsigned long) thread_private->stop_event_handle );

	return;
}

/*********************** Posix Pthreads **********************/

#elif defined(_POSIX_THREADS)

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
	static const char fname[] = "mx_thread_start_function()";

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

	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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
		MX_DEBUG(-2,("%s: mx_thread_kill() was called, so the "
			"stop request handler will not be invoked.", fname ));

		return;
	}

	handler = thread->stop_request_handler;

	if ( handler == NULL ) {
		MX_DEBUG(-2,
("%s: No stop request handler was configured, so no handler will be executed.",
			fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

	/* Create a Pthread key that we will use to store a pointer to
	 * the current thread for each thread.  The stop request handler
	 * will be invoked for each thread as it exits.
	 */

	status = pthread_key_create( &mx_current_thread_key,
					mx_thread_stop_request_handler );

	if ( status != 0 ) {
		switch( status ) {
		case EAGAIN:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	"Not enough resources available to create a new thread specific key." );
			break;
		case ENOMEM:
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for a new thread specific key." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
		"pthread_key_create() returned an unknown error code %d.",
				status );
			break;
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
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_free( thread_private );

	mx_free( thread );

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_cancel() returned an unknown error code %d.",
				status );
			break;
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

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = pthread_cancel( thread_private->thread_id );

	if ( status != 0 ) {
		switch( status ) {
		case ESRCH:
			return mx_error( MXE_NOT_FOUND, fname,
				"The requested thread was not found." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_cancel() returned an unknown error code %d.",
				status );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

MX_EXPORT mx_status_type
mx_thread_check_for_stop_request( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_check_for_stop_request()";

	MX_DEBUG(-2,("%s invoked.", fname));

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

	MX_DEBUG(-2,("%s invoked.", fname));

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

	/* FIXME: Currently we pay no attention to 'max_seconds_to_wait'. */

	MX_DEBUG(-2,("%s invoked.", fname));

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status = pthread_join( thread_private->thread_id, &value_ptr );

	if ( status != 0 ) {
		switch( status ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The requested thread is not joinable." );
			break;
		case ESRCH:
			return mx_error( MXE_NOT_FOUND, fname,
				"The requested thread was not found." );
			break;
		case EDEADLK:
			return mx_error( MXE_MIGHT_CAUSE_DEADLOCK, fname,
				"Waiting for the requested thread might "
				"cause a deadlock." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
			"pthread_join() returned an unknown error code %d.",
				status );
			break;
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
			break;
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invalid Pthread key specified for pthread_setspecific()." );
			break;
		default:
			return mx_error( MXE_UNKNOWN_ERROR, fname,
		"pthread_setspecific() returned an unknown error code %d.",
				status );
			break;
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

	mx_status = mx_thread_get_pointers( thread, &thread_private, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return;

	if ( (message != NULL) && (strlen(message) > 0) ) {
		mx_info( message );
	}

	mx_info( "  thread pointer         = %p", thread );
	mx_info( "  thread_private pointer = %p", thread_private );
	mx_info( "  Posix thread id        = %lu",
				(unsigned long) thread_private->thread_id );

	return;
}

/*-------------------------------------------------------------------------*/

#else

#error MX thread functions have not yet been defined for this platform.

#endif

