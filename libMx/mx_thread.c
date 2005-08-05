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

static HANDLE
mx_get_current_thread_handle( void )
{
	static const char fname[] = "mx_get_current_thread_handle()";

	HANDLE process_pseudo_handle;
	HANDLE thread_pseudo_handle, thread_real_handle;
	BOOL status;

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
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to duplicate a handle for the current thread.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );

		return INVALID_HANDLE_VALUE;
	}

	return thread_real_handle;
}

static unsigned __stdcall
mx_thread_start_function( void *args_ptr )
{
	static const char fname[] = "mx_thread_start_function()";

	MX_WIN32_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
	BOOL status;
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

	/* Save a thread-specific pointer to the MX_THREAD structure using
	 * the Thread Local Storage created in mx_thread_initialize().
	 */

	status = TlsSetValue( mx_current_thread_index, thread );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to save the MX_THREAD pointer in "
			"thread local storage.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );

		return FALSE;
	}

	/* Invoke MX's thread function. */

	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));

	mx_status = (thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	_endthreadex( (int) mx_status.code );

	/* Should never get here. */

	return FALSE;
}

MX_EXPORT mx_status_type
mx_thread_initialize( void )
{
	static const char fname[] = "mx_thread_initialize()";

	MX_THREAD *thread;
	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

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

	/* Allocate the MX_THREAD structure. */

	thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	/* Allocate the private thread structure. */

	thread_private = (MX_WIN32_THREAD_PRIVATE *)
				malloc( sizeof(MX_WIN32_THREAD_PRIVATE) );

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Unable to allocate memory for a MX_WIN32_THREAD_PRIVATE structure." );
	}
	
	thread->thread_ptr = thread_private;

	thread_private->thread_id = GetCurrentThreadId();

	thread_private->thread_handle = mx_get_current_thread_handle();

	if ( thread_private->thread_handle == INVALID_HANDLE_VALUE ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get a handle for the initial thread." );
	}

	/* Save a thread-specific pointer to the MX_THREAD structure by
	 * using the Pthreads key we created above.
	 */

	status = TlsSetValue( mx_current_thread_index, thread );

	if ( status == 0 ) {
		DWORD last_error_code;
		TCHAR message_buffer[100];

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to save the MX_THREAD pointer in "
			"thread local storage.  "
			"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	/* Record the fact that the initialization has been completed.
	 * Only one thread will be running at the time that the flag
	 * is set, so it is not necessary to use a mutex for the
	 * mx_threads_are_initialized_flag.
	 */

	mx_threads_are_initialized = TRUE;

	return MX_SUCCESSFUL_RESULT;
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

	/* Allocate the MX_THREAD structure. */

	*thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( *thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an MX_THREAD structure." );
	}

	/* Allocate the private thread structure. */

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

	/* Create an unsignaled event object with manual reset.  This
	 * object is used by other threads to request that this thread
	 * shut itself down.
	 */

	thread_private->stop_event_handle =
				CreateEvent( NULL, TRUE, FALSE, NULL );

	if ( thread_private->stop_event_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to create stop event object for thread %p.  "
			"Win32 error code = %ld, error_message = '%s'",
			thread, last_error_code, message_buffer );
	}

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

MX_EXPORT void
mx_thread_exit( MX_THREAD *thread,
		long thread_exit_status )
{
	static const char fname[] = "mx_thread_exit()";

	MX_WIN32_THREAD_PRIVATE *thread_private;

	MX_DEBUG(-2,("%s invoked.", fname));

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

	_endthreadex( (int) thread_exit_status );
}

MX_EXPORT mx_status_type
mx_thread_free( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free()";

	MX_WIN32_THREAD_PRIVATE *thread_private;

	MX_DEBUG(-2,("%s invoked.", fname));

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

MX_EXPORT mx_status_type
mx_thread_kill( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_kill()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

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

MX_EXPORT mx_status_type
mx_thread_stop( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_stop()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

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

MX_EXPORT mx_status_type
mx_thread_check_for_stop_request( MX_THREAD *thread,
				int *stop_requested )
{
	static const char fname[] = "mx_thread_check_for_stop_request()";

	MX_WIN32_THREAD_PRIVATE *thread_private;
	BOOL status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	if ( stop_requested == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The stop_requested pointer passed was NULL." );
	}

	thread_private = (MX_WIN32_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_WIN32_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

	status = WaitForSingleObject( thread_private->stop_event_handle, 0 );

	switch( status ) {
	case WAIT_ABANDONED:
		return mx_error( MXE_OBJECT_ABANDONED, fname,
			"The stop event object for thread %p has been "
			"abandoned.  This should NEVER happen.", thread );
		break;
	case WAIT_OBJECT_0:
		*stop_requested = TRUE;
		break;
	case WAIT_TIMEOUT:
		*stop_requested = FALSE;
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

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_thread_wait( MX_THREAD *thread,
		long *thread_exit_status,
		double max_seconds_to_wait )
{
	static const char fname[] = "mx_thread_wait()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Not yet implemented." );
}

/*********************** Posix Pthreads **********************/

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

static pthread_key_t mx_current_thread_key;

static void *
mx_thread_start_function( void *args_ptr )
{
	static const char fname[] = "mx_thread_start_function()";

	MX_POSIX_THREAD_ARGUMENTS_PRIVATE *thread_arg_struct;
	MX_THREAD *thread;
	MX_THREAD_FUNCTION *thread_function;
	void *thread_arguments;
	int status;
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

	/* Save a thread specific pointer to the MX_THREAD structure using
	 * the Pthreads key created in mx_thread_initialize().
	 */

	status = pthread_setspecific( mx_current_thread_key, thread );

	if ( status != 0 ) {
		switch( status ) {
		case ENOMEM:
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Insufficient memory is available to associate "
			"the MX_THREAD pointer with the current thread key." );

			return NULL;
			break;
		case EINVAL:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invalid Pthread key specified for pthread_setspecific()." );

			return NULL;
			break;
		default:
			(void) mx_error( MXE_UNKNOWN_ERROR, fname,
		"pthread_setspecific() returned an unknown error code %d.",
				status );

			return NULL;
			break;
		}
	}

	/* Invoke MX's thread function. */

	MX_DEBUG(-2,("%s: thread_function = %p", fname, thread_function));

	mx_status = (thread_function)( thread, thread_arguments );

	/* End the thread when the MX thread function terminates. */

	thread->thread_exit_status = mx_status.code;

	(void) pthread_exit(NULL);

	/* Should never get here. */

	return NULL;
}

MX_EXPORT mx_status_type
mx_thread_initialize( void )
{
	static const char fname[] = "mx_thread_initialize()";

	MX_THREAD *thread;
	MX_POSIX_THREAD_PRIVATE *thread_private;
	int status;

	MX_DEBUG(-2,("%s invoked.", fname));

	/* Create a Pthread key that we will use to store a pointer to
	 * the current thread for each thread.
	 */

	status = pthread_key_create( &mx_current_thread_key, NULL );

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

	/* Allocate the MX_THREAD structure. */

	thread = (MX_THREAD *) malloc( sizeof(MX_THREAD) );

	if ( thread == (MX_THREAD *) NULL ) {
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
	
	thread->thread_ptr = thread_private;

	thread_private->thread_id = pthread_self();

	/* Save a thread-specific pointer to the MX_THREAD structure by
	 * using the Pthreads key we created above.
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

	/* Record the fact that the initialization has been completed.
	 * Only one thread will be running at the time that the flag
	 * is set, so it is not necessary to use a mutex for the
	 * mx_threads_are_initialized flag.
	 */

	mx_threads_are_initialized = TRUE;

	return MX_SUCCESSFUL_RESULT;
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
mx_thread_exit( MX_THREAD *thread,
		long thread_exit_status )
{
	static const char fname[] = "mx_thread_exit()";

	MX_POSIX_THREAD_PRIVATE *thread_private;

	MX_DEBUG(-2,("%s invoked.", fname));

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

/* mx_thread_free() frees all of the data structures allocated
 * for the thread.
 */

MX_EXPORT mx_status_type
mx_thread_free( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_free()";

	MX_POSIX_THREAD_PRIVATE *thread_private;

	MX_DEBUG(-2,("%s invoked.", fname));

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

MX_EXPORT mx_status_type
mx_thread_kill( MX_THREAD *thread )
{
	static const char fname[] = "mx_thread_kill()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	int status;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_POSIX_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

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

MX_EXPORT mx_status_type
mx_thread_wait( MX_THREAD *thread,
		long *thread_exit_status,
		double max_seconds_to_wait )
{
	static const char fname[] = "mx_thread_wait()";

	MX_POSIX_THREAD_PRIVATE *thread_private;
	void *value_ptr;
	int status;

	/* FIXME: Currently we pay no attention to 'max_seconds_to_wait'. */

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( thread == (MX_THREAD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_THREAD pointer passed was NULL." );
	}

	thread_private = (MX_POSIX_THREAD_PRIVATE *) thread->thread_ptr;

	if ( thread_private == (MX_POSIX_THREAD_PRIVATE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The thread_private field for the MX_THREAD pointer passed was NULL.");
	}

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

MX_EXPORT void
mx_show_thread_info( MX_THREAD *thread, char *message )
{
	static const char fname[] = "mx_show_thread_info()";

	MX_POSIX_THREAD_PRIVATE *thread_private;

	if ( (message != NULL) && (strlen(message) > 0) ) {
		mx_info( message );
	}

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

