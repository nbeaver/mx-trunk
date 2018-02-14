/*
 * Name:    mx_condition_variable.c
 *
 * Purpose: MX condition variable functions.
 *
 *          These functions are modeled on Posix condition variables.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2014-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_CONDITION_VARIABLE_DEBUG_POINTERS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_osdef.h"
#include "mx_stdint.h"
#include "mx_hrt.h"
#include "mx_dynamic_library.h"
#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"

/************************ Microsoft Win32 ***********************/

#if defined(OS_WIN32)

/* FIXME: EXPLANATION OF IMPLEMENTATION CHOICES (May 10, 2014 - W. Lavender)
 *
 * Windows Vista and above have real condition variables built into the
 * operating system.  Unfortunately, we cannot currently use them in MX.
 * The problem is that Win32 condition variables can work with Win32
 * critical sections or Win32 slim RW locks, but _NOT_ with Win32 mutexes.
 *
 * If MX mutexes were built on critical sections or slim RW locks, then
 * there would be no problem.  However, in their current implementation,
 * MX mutexes _cannot_ use critical sections or slim RW locks.  Instead,
 * they must use Win32 mutexes.  Here is why:
 *
 * 1. Critical Sections - Win32 critical sections cannot be held for
 *      indefinitely large amounts of time.  If you try to do that,
 *      then after a timeout period your program will die with an
 *      EXCEPTION_POSSIBLE_DEADLOCK exception.  Theoretically, there is
 *      a registry value that can be used to increase this timeout.
 *      Unfortunately, this registry entry is not process-specific and
 *      applies to all processes running in the current control set.
 *
 *      Apparently Microsoft thinks that critical sections should never
 *      be held for long periods of time and they choose to enforce this
 *      particular point of view with the large, blunt sledgehammer of
 *      an uncaught exception.  They also say not to try and handle
 *      the exception.
 *
 *      This is enough to make critical sections unsuitable for MX, since
 *      there are places in the MX code that hold mutexes for indefinitely
 *      long periods of time.  Presumably, if those pieces of code were
 *      rewritten to use condition variables directly instead of blocking
 *      in mutexes, then this issue would go away.  However, I do not
 *      currently (May 2014) have time to debug the potential changes in
 *      behavior that might be caused by this issue, so the issue remains.
 *
 * 2. Slim RW locks - This one is easier to explain.  Slim RW locks are
 *      not recursive, but MX mutexes must be recursive.
 *
 *------------------------------------------------------------------------
 *
 * In the meantime, I am using an adaptation of the SignalObjectAndWait()
 * code from the web page http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * titled "Strategies for Implementing POSIX Condition Variables on Win32".
 * This code has the virtue that it will also run on Windows NT 4.0,
 * Windows 2000, and Windows XP.
 *
 */

#include <windows.h>

static mx_bool_type mx_win32_type_tested_for = FALSE;

static mx_bool_type mx_win32_signal_object_and_wait_available = FALSE;

typedef DWORD (*SignalObjectAndWait_type)( HANDLE, HANDLE, DWORD, BOOL );

static SignalObjectAndWait_type
	ptr_SignalObjectAndWait = NULL;

typedef struct {
  int waiters_count_;
  // Number of waiting threads.

  CRITICAL_SECTION waiters_count_lock_;
  // Serialize access to <waiters_count_>.

  HANDLE sema_;
  // Semaphore used to queue up threads waiting for the condition to
  // become signaled. 

  HANDLE waiters_done_;
  // An auto-reset event used by the broadcast/signal thread to wait
  // for all the waiting thread(s) to wake up and be released from the
  // semaphore. 

  size_t was_broadcast_;
  // Keeps track of whether we were broadcasting or signaling.  This
  // allows us to optimize the code if we're just signaling.

} MX_WIN32_CONDITION_VARIABLE;

MX_EXPORT mx_status_type
mx_condition_variable_create( MX_CONDITION_VARIABLE **cv )
{
	static const char fname[] = "mx_condition_variable_create()";

	MX_WIN32_CONDITION_VARIABLE *win32_cv = NULL;
	mx_status_type mx_status;

	if ( mx_win32_type_tested_for == FALSE ) {
	    mx_bool_type is_windows_9x;

	    mx_win32_type_tested_for = TRUE;

	    mx_status = mx_win32_is_windows_9x( &is_windows_9x );

	    if ( mx_status.code != MXE_SUCCESS )
	        return mx_status;

	    if ( is_windows_9x ) {
	        mx_win32_signal_object_and_wait_available = FALSE;
	    } else {
	        int os_major, os_minor, os_update;

	        mx_status = mx_get_os_version( &os_major,
					&os_minor, &os_update );

	        if ( mx_status.code != MXE_SUCCESS )
	            return mx_status;

	        if ( os_major < 4 ) {
	            mx_win32_signal_object_and_wait_available = FALSE;
	        } else {
	            mx_status = mx_dynamic_library_get_library_and_symbol(
	                "kernel32.dll", "SignalObjectAndWait",
	                    NULL, (void **) &ptr_SignalObjectAndWait, 0 );

	            if ( mx_status.code != MXE_SUCCESS ) {
	                mx_win32_signal_object_and_wait_available = FALSE;

	                return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
	                    "SignalObjectAndWait() was not found, even though "
	                    "it _should_ exist for this version of Windows.  "
	                    "Original error = '%s'.",
	                        mx_status.message );
	            }

#if MX_CONDITION_VARIABLE_DEBUG_POINTERS
		    MX_DEBUG(-2,("%s: &ptr_SignalObjectAndWait = %p",
			fname, &ptr_SignalObjectAndWait ));
		    MX_DEBUG(-2,("%s: ptr_SignalObjectAndWait = %p",
			fname, ptr_SignalObjectAndWait ));
#endif

		    if ( ptr_SignalObjectAndWait == NULL ) {
			    mx_win32_signal_object_and_wait_available = FALSE;

			    return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			    "mx_dynamic_library_get_library_and_signal() said "
			    "that it successfully found a pointer to the "
			    "SignalObjectAndWait() API function.  However, "
			    "the pointer returned is NULL.  This makes "
			    "no sense." );
		    }

	            mx_win32_signal_object_and_wait_available = TRUE;
	        }
	    }
	}

	if ( mx_win32_signal_object_and_wait_available == FALSE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"MX condition variables are not supported for Windows "
		"95, 98, ME, or for NT versions before NT 4.0" );
	}

	if ( cv == (MX_CONDITION_VARIABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	*cv = malloc( sizeof(MX_CONDITION_VARIABLE) );

	if ( (*cv) == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Ran out of memory trying to allocate an MX_CONDITION_VARIABLE structure." );
	}

	win32_cv = malloc( sizeof(MX_WIN32_CONDITION_VARIABLE) );

	if ( win32_cv == (MX_WIN32_CONDITION_VARIABLE *) NULL ) {
		mx_free( *cv );
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a "
		"MX_WIN32_CONDITION_VARIABLE struct." );
	}

	/*---*/

	win32_cv->waiters_count_ = 0;
	win32_cv->was_broadcast_ = 0;

	win32_cv->sema_ = CreateSemaphore (NULL,       // no security
	                                   0,          // initially 0
	                                   0x7fffffff, // max count
	                                   NULL);      // unnamed

	InitializeCriticalSection( &(win32_cv->waiters_count_lock_) );

	win32_cv->waiters_done_ = CreateEvent (NULL,  // no security
	                                       FALSE, // auto-reset
	                                       FALSE, // non-signaled initially
	                                       NULL); // unnamed

	/*---*/

	(*cv)->cv_private = win32_cv;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_destroy( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_create()";

	MX_WIN32_CONDITION_VARIABLE *win32_cv;
	BOOL os_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	win32_cv = cv->cv_private;

	if ( win32_cv == NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Win32 condition variable pointer for "
		"MX condition variable %p is NULL.", cv );

		mx_free( cv );
		return mx_status;
	}

	/*---*/

	os_status = CloseHandle( &(win32_cv->waiters_done_) );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to close the waiters_done_ event handle for "
		"MX condition variable %p.  "
		"Win32 error code = %ld, error message = '%s'.",
			cv, last_error_code, message_buffer );
	}

	DeleteCriticalSection( &(win32_cv->waiters_count_lock_) );

	os_status = CloseHandle( &(win32_cv->sema_) );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to close the sema_ semaphore handle for "
		"MX condition variable %p.  "
		"Win32 error code = %ld, error message = '%s'.",
			cv, last_error_code, message_buffer );
	}

	/*---*/

	mx_free( win32_cv );
	mx_free( cv );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_wait( MX_CONDITION_VARIABLE *cv,
				MX_MUTEX *mutex )
{
	struct timespec infinite_ts = {LONG_MAX,LONG_MAX};
	mx_status_type mx_status;

	mx_status = mx_condition_variable_timed_wait( cv, mutex, &infinite_ts );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_condition_variable_timed_wait( MX_CONDITION_VARIABLE *cv,
				MX_MUTEX *mutex,
				const struct timespec *ts )
{
	static const char fname[] = "mx_condition_variable_timed_wait()";

	MX_WIN32_CONDITION_VARIABLE *win32_cv;
	HANDLE *win32_mutex_handle_ptr;
	DWORD dword_timeout;
	double timeout_in_msec;
	int last_waiter;
	DWORD os_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}
	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}
	if ( ts == (const struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The timespec pointer passed was NULL." );
	}

	win32_cv = cv->cv_private;

	if ( win32_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Win32 condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	win32_mutex_handle_ptr = mutex->mutex_ptr;

	if ( win32_mutex_handle_ptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Win32 mutex pointer for MX mutex %p is NULL.", mutex );
	}

	timeout_in_msec =
		1000.0 * mx_convert_high_resolution_time_to_seconds( *ts );

	if ( timeout_in_msec >= (double)INFINITE ) {
		dword_timeout = INFINITE;
	} else {
		dword_timeout = mx_round( timeout_in_msec );
	}

	/*---*/

	// Avoid race conditions

	EnterCriticalSection( &(win32_cv->waiters_count_lock_) );
	win32_cv->waiters_count_++;
	LeaveCriticalSection( &(win32_cv->waiters_count_lock_) );

	if ( ptr_SignalObjectAndWait == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The pointer to the SignalObjectAndWait() Win32 call "
		"has not been found and is set to NULL.  This should "
		"_not_ happen on versions of Windows starting with "
		"Windows NT 4.0 and above!" );
	}

	// This call atomically releases the mutex and waits on the
	// semaphore until "signal" or "broadcast"
	// are called by another thread.

	os_status = ptr_SignalObjectAndWait( *win32_mutex_handle_ptr,
					win32_cv->sema_,
					dword_timeout,
					FALSE );

	// Reacquire lock to avoid race conditions.

	EnterCriticalSection( &(win32_cv->waiters_count_lock_) );

	// We're no longer waiting...
	win32_cv->waiters_count_--;

	// Check to see if we're the last waiter after a "broadcast".
	last_waiter = win32_cv->was_broadcast_
			&& (win32_cv->waiters_count_ == 0);

	LeaveCriticalSection( &(win32_cv->waiters_count_lock_) );

	// If we're the last waiter thread during this particular broadcast
	// then let all the other threads proceed.

	if (last_waiter) {
		// This call atomically signals the <waiters_done_>event and
		// waits until it can acquire *win32_mutex_handle_ptr.
		// This is required to ensure fairness.

		os_status = ptr_SignalObjectAndWait( win32_cv->waiters_done_,
						*win32_mutex_handle_ptr,
						INFINITE,
						FALSE );
	} else {
		// Always regain the external mutex since that's the guarantee
		// we give to our callers.

		os_status = WaitForSingleObject(
				*win32_mutex_handle_ptr, INFINITE );
	}

	MXW_UNUSED( os_status );

	/*---*/

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_signal( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_signal()";

	MX_WIN32_CONDITION_VARIABLE *win32_cv;
	int have_waiters;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	win32_cv = cv->cv_private;

	if ( win32_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Win32 condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	/*---*/

	EnterCriticalSection( &(win32_cv->waiters_count_lock_) );
	have_waiters = (win32_cv->waiters_count_ > 0);
	LeaveCriticalSection( &(win32_cv->waiters_count_lock_) );

	// If there aren't any waiters, then this is a no-op.

	if (have_waiters)
		ReleaseSemaphore( win32_cv->sema_, 1, 0 );

	/*---*/

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_broadcast( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_broadcast()";

	MX_WIN32_CONDITION_VARIABLE *win32_cv;
	int have_waiters;
	DWORD os_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	win32_cv = cv->cv_private;

	if ( win32_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Win32 condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	/*---*/

	// This is needed to ensure that <waiters_count_> and
	// <was_broadcast_> are consistent relative to each other.

	EnterCriticalSection( &(win32_cv->waiters_count_lock_) );
	have_waiters = 0;

	if ( win32_cv->waiters_count_ > 0 ) {
		// We are broadcasting, even if there is just one waiter...
		// Record that we are broadcasting, which helps optimize
		// the "wait" call for the non-broadcast case.

		win32_cv->was_broadcast_ = 1;
		have_waiters = 1;
	}

	if (have_waiters) {
		// Wake up all the waiters atomically.
		ReleaseSemaphore(win32_cv->sema_, win32_cv->waiters_count_, 0);

		LeaveCriticalSection( &(win32_cv->waiters_count_lock_) );

		// Wait for all the awakened threads to acquire the counting
		// semaphore.

		os_status = WaitForSingleObject(
				win32_cv->waiters_done_, INFINITE );

		MXW_UNUSED( os_status );

		// This assignment is okay, even without the
		// <waiters_count_lock_> held because no other waiter threads
		// can wake up to access it.

		win32_cv->was_broadcast_ = 0;
	} else {
		LeaveCriticalSection( &(win32_cv->waiters_count_lock_) );
	}

	/*---*/

	return MX_SUCCESSFUL_RESULT;
}

/************************ Posix pthreads ***********************/

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_BSD) \
	|| defined(OS_SOLARIS) || defined(OS_CYGWIN) || defined(OS_QNX) \
	|| defined(OS_HURD) || defined(OS_VMS) || defined(OS_UNIXWARE) \
	|| defined(OS_RTEMS) || defined(OS_ANDROID) || defined(OS_MINIX)

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

MX_EXPORT mx_status_type
mx_condition_variable_create( MX_CONDITION_VARIABLE **cv )
{
	static const char fname[] = "mx_condition_variable_create()";

	pthread_cond_t *pthread_cv = NULL;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	*cv = malloc( sizeof(MX_CONDITION_VARIABLE) );

	if ( (*cv) == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Ran out of memory trying to allocate an MX_CONDITION_VARIABLE structure." );
	}

	pthread_cv = malloc( sizeof(pthread_cond_t) );

	if ( pthread_cv == (pthread_cond_t *) NULL ) {
		mx_free( *cv );
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate a pthread_cond_t struct." );
	}

	pthread_status = pthread_cond_init( pthread_cv, NULL );

	if ( pthread_status != 0 ) {
		mx_free( pthread_cv );
		mx_free( *cv );
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to initialize the condition variable at %p failed."
		"  Error code = %d, error message = '%s'",
		pthread_cv, pthread_status, strerror(pthread_status) );
	}

	(*cv)->cv_private = pthread_cv;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_destroy( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_destroy()";

	pthread_cond_t *pthread_cv;
	int pthread_status;
	mx_status_type mx_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		mx_status = mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );

		mx_free( cv );
		return mx_status;
	}

	pthread_status = pthread_cond_destroy( pthread_cv );

	if ( pthread_status != 0 ) {
		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to destroy the Posix condition variable "
		"for MX condition variable %p failed.  "
		"Error code = %d, error message = '%s'",
			cv, pthread_status, strerror(pthread_status) );

		mx_free( pthread_cv );
		mx_free( cv );

		return mx_status;
	}

	mx_free( pthread_cv );
	mx_free( cv );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_wait( MX_CONDITION_VARIABLE *cv,
				MX_MUTEX *mutex )
{
	static const char fname[] = "mx_condition_variable_wait()";

	pthread_cond_t *pthread_cv;
	pthread_mutex_t *pthread_mutex;
	int pthread_status;

	MX_THREAD *my_mx_thread;
	unsigned long my_pthread_thread_id;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}
	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_mutex = mutex->mutex_ptr;

	if ( pthread_mutex == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix mutex pointer for MX mutex %p is NULL.", mutex );
	}

	pthread_status = pthread_cond_wait( pthread_cv, pthread_mutex );

	switch( pthread_status ) {
	case 0:
		/* Nothing went wrong. */
		break;
	case EPERM:
		/* This thread does not own the mutex that was passed to it. */

		my_mx_thread = mx_get_current_thread_pointer();

		my_pthread_thread_id = mx_get_thread_id( NULL );

		return mx_error( MXE_PERMISSION_DENIED, fname,
		"This thread %p (ID %#lx) does not own the mutex %p "
		"that you tried to use with pthread_cond_wait() for "
		"condition variable %p.",
			my_mx_thread, my_pthread_thread_id, mutex, cv );

		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to wait for MX condition variable %p using "
		"MX mutex %p failed.  "
		"Posix condition variable = %p, Posix mutex = %p  "
		"Error code = %d, error message = '%s'",
			cv, mutex, pthread_cv, pthread_mutex,
			pthread_status, strerror(pthread_status) );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_timed_wait( MX_CONDITION_VARIABLE *cv,
				MX_MUTEX *mutex,
				const struct timespec *ts )
{
	static const char fname[] = "mx_condition_variable_timed_wait()";

	pthread_cond_t *pthread_cv;
	pthread_mutex_t *pthread_mutex;
	int pthread_status;
	double timeout;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}
	if ( mutex == (MX_MUTEX *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MUTEX pointer passed was NULL." );
	}
	if ( ts == (const struct timespec *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The timespec pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_mutex = mutex->mutex_ptr;

	if ( pthread_mutex == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix mutex pointer for MX mutex %p is NULL.", mutex );
	}

	pthread_status = pthread_cond_timedwait( pthread_cv, pthread_mutex, ts);

	switch( pthread_status ) {
	case 0:
		break;
	case ETIMEDOUT:
		timeout = mx_convert_high_resolution_time_to_seconds( *ts );

		return mx_error( MXE_TIMED_OUT, fname,
		"The wait for MX condition variable %p using MX mutex %p "
		"timed out after %f seconds.", cv, mutex, timeout );
		break;
	default:
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to wait for MX condition variable %p using "
		"MX mutex %p failed.  "
		"Posix condition variable = %p, Posix mutex = %p  "
		"Error code = %d, error message = '%s'",
			cv, mutex, pthread_cv, pthread_mutex,
			pthread_status, strerror(pthread_status) );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_signal( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_signal()";

	pthread_cond_t *pthread_cv;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_status = pthread_cond_signal( pthread_cv );

	if ( pthread_status != 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to signal MX condition variable %p failed.  "
		"Error code = %d, error message = '%s'",
			cv, pthread_status, strerror(pthread_status) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_condition_variable_broadcast( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_broadcast()";

	pthread_cond_t *pthread_cv;
	int pthread_status;

	if ( cv == (MX_CONDITION_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CONDITION_VARIABLE pointer passed was NULL." );
	}

	pthread_cv = cv->cv_private;

	if ( pthread_cv == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The Posix condition variable pointer for "
		"MX condition variable %p is NULL.", cv );
	}

	pthread_status = pthread_cond_broadcast( pthread_cv );

	if ( pthread_status != 0 ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to broadcast to MX condition variable %p failed.  "
		"Error code = %d, error message = '%s'",
			cv, pthread_status, strerror(pthread_status) );
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_DJGPP) || defined(OS_VXWORKS)

/* Platforms that do not provide for condition variables.*/

MX_EXPORT mx_status_type
mx_condition_variable_create( MX_CONDITION_VARIABLE **cv )
{
	static const char fname[] = "mx_condition_variable_create()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Condition variables are not implemented on this platform." );
}

MX_EXPORT mx_status_type
mx_condition_variable_destroy( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_destroy()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Condition variables are not implemented on this platform." );
}

MX_EXPORT mx_status_type
mx_condition_variable_wait( MX_CONDITION_VARIABLE *cv,
					MX_MUTEX *mutex )
{
	static const char fname[] = "mx_condition_variable_wait()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Condition variables are not implemented on this platform." );
}

MX_EXPORT mx_status_type
mx_condition_variable_timed_wait( MX_CONDITION_VARIABLE *cv,
					MX_MUTEX *mutex,
					const struct timespec *ts )
{
	static const char fname[] = "mx_condition_variable_timed_wait()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Condition variables are not implemented on this platform." );
}

MX_EXPORT mx_status_type
mx_condition_variable_signal( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_signal()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Condition variables are not implemented on this platform." );
}

MX_EXPORT mx_status_type
mx_condition_variable_broadcast( MX_CONDITION_VARIABLE *cv )
{
	static const char fname[] = "mx_condition_variable_broadcast()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Condition variables are not implemented on this platform." );
}

#else

#error MX condition variables are not yet implemented on this platform.

#endif

