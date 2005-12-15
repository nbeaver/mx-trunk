/*
 * Name:    mx_signal.c
 *
 * Purpose: Functions for managing the use of signals.
 *
 * Author:  William Lavender
 *
 * WARNING: These functions are advisory only in nature.  In general, they
 *          have no mechanism for preventing a thread that does not use these
 *          functions from manipulating a signal.
 *
 * Note:    At present, the primary use of these functions is to allocate
 *          signals to use as the signal generated when a Posix timer expires.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SIGNAL_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "mx_osdef.h"
#include "mx_unistd.h"
#include "mx_util.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_signal.h"

#define MX_SIGNAL_MUTEX_UNLOCK \
		do {							\
			int mt_status;					\
									\
			mt_status = mx_mutex_unlock( mx_signal_mutex );	\
									\
			if ( mt_status != MXE_SUCCESS ) {		\
				(void) mx_error( mt_status, fname,	\
				"Unable to unlock mx_signal_mutex." );	\
			}						\
		} while(0)

/*--------------------------------------------------------------------------*/

#if (defined( _POSIX_REALTIME_SIGNALS ) && ( _POSIX_REALTIME_SIGNALS >= 0 )) \
	|| defined( OS_CYGWIN )

#define MX_NUM_SIGNALS	SIGRTMAX

static int *mx_signal_array = NULL;
static int mx_num_signals;

static MX_MUTEX *mx_signal_mutex = NULL;

static void
mx_signal_dummy_handler( int signal_number,
			siginfo_t *siginfo,
			void *cruft )
{
	return;
}

MX_EXPORT mx_status_type
mx_signal_initialize( void )
{
	static const char fname[] = "mx_signal_initialize()";

	int i, status, saved_errno;
	struct sigaction sa, old_sa;
	sigset_t set;
	mx_status_type mx_status;

	mx_num_signals = MX_NUM_SIGNALS;

#if MX_SIGNAL_DEBUG
	MX_DEBUG(-2,("%s: mx_num_signals = %d", fname, mx_num_signals));
#endif

	/* Create an array to store the allocation status of each signal. */

	mx_signal_array = (int *) calloc(mx_num_signals, sizeof(int));

	if ( mx_signal_array == (int *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
  "Unable to allocate memory for a %d element mx_signal_array.",
			mx_num_signals );
	}

	/* See if any of the non-realtime signals are already in use. */

	for ( i = 1; i < SIGRTMIN; i++ ) {
		/* See if the signal is already in use. */

		status = sigaction( i, NULL, &old_sa );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:
			case ENOSYS:
#if defined(OS_IRIX)
			case 0:
#endif
				/* There may be gaps in the supported
				 * signal numbers, so mark this signal
				 * number as in use and continue on to
				 * the next signal.
				 */

				mx_signal_array[i-1] = TRUE;

#if MX_SIGNAL_DEBUG
				MX_DEBUG(-2,("%s: Signal %d is invalid.",
					fname, i));
#endif
				continue;  /* Go to the top of the for() loop.*/
			}

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get the signal handling status for "
			"signal %d (case 1).  "
			"Errno = %d, error message = '%s'.",
				i, saved_errno, strerror(saved_errno) );
		}

		if ( old_sa.sa_handler != SIG_DFL ) {
			/* This signal is not using the default handler,
			 * so we assume that it is already being used by 
			 * something else.
			 */

			mx_signal_array[i-1] = TRUE;

#if MX_SIGNAL_DEBUG
			MX_DEBUG(-2,("%s: Signal %d is already in use.",
				fname, i));
#endif
		}
	}

	/* Just in case there are signal numbers higher than SIGRTMAX. */

	for ( i = SIGRTMAX+1; i <= mx_num_signals; i++ ) {
		/* See if the signal is already in use. */

		status = sigaction( i, NULL, &old_sa );

		if ( status != 0 ) {
			saved_errno = errno;

			switch( saved_errno ) {
			case EINVAL:
			case ENOSYS:
				/* There may be gaps in the supported
				 * signal numbers, so mark this signal
				 * number as in use and continue on to
				 * the next signal.
				 */

				mx_signal_array[i-1] = TRUE;

#if MX_SIGNAL_DEBUG
				MX_DEBUG(-2,("%s: Signal %d is invalid.",
					fname, i));
#endif
				continue;  /* Go to the top of the for() loop.*/
			}

			MX_DEBUG(-2,("%s: MARKER 2", fname));

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get the signal handling status for "
			"signal %d (case 2).  "
			"Errno = %d, error message = '%s'.",
				i, saved_errno, strerror(saved_errno) );
		}

		if ( old_sa.sa_handler != SIG_DFL ) {
			/* This signal is not using the default handler,
			 * so we assume that it is already being used by 
			 * something else.
			 */

			mx_signal_array[i-1] = TRUE;

#if MX_SIGNAL_DEBUG
			MX_DEBUG(-2,("%s: Signal %d is already in use.",
				fname, i));
#endif
		}
	}

	/* Install dummy signal handlers for the realtime signals and mask
	 * off delivery of the signals.  Usenet posts by David Butenhof say
	 * that the signal handlers need to be here for sigwait(),
	 * sigwaitinfo(), and sigtimedwait() to work correctly on all
	 * platforms.
	 */

	status = sigemptyset( &set );

	if ( status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"A call to sigemptyset() failed with errno = %d, "
			"error message = '%s'.",
				saved_errno, strerror(saved_errno) );
	}

	for ( i = SIGRTMIN; i <= SIGRTMAX; i++ ) {
		/* See if the signal is already in use. */

		status = sigaction( i, NULL, &old_sa );

		if ( status != 0 ) {
			saved_errno = errno;

			MX_DEBUG(-2,("%s: MARKER 3", fname));

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get the signal handling status for realtime "
			"signal %d (case 3).  "
			"Errno = %d, error message = '%s'.",
				i, saved_errno, strerror(saved_errno) );
		}

		if ( old_sa.sa_handler != SIG_DFL ) {
			/* This signal is not using the default handler,
			 * so we assume that it is already being used by 
			 * something else.
			 */

			mx_signal_array[i-1] = TRUE;

#if MX_SIGNAL_DEBUG
			MX_DEBUG(-2,
			("%s: Realtime signal %d is already in use.",
				fname, i));
#endif

			continue;    /* Go back to the top of the for() loop. */
		}

#if MX_SIGNAL_DEBUG
		MX_DEBUG(-2,
		("%s: Installing dummy signal handler for realtime signal %d.",
			fname, i));
#endif

		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = mx_signal_dummy_handler;

		status = sigaction( i, &sa, NULL );

		if ( status != 0 ) {
			saved_errno = errno;

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to install signal handler for realtime "
			"signal %i.  Errno = %d, error message = '%s'.",
				i, saved_errno, strerror(saved_errno) );
		}

		/* Add the signal to the set of signals to be blocked. */

		status = sigaddset( &set, i );

		if ( status != 0 ) {
			saved_errno = errno;

			if ( saved_errno == EINVAL ) {
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"sigaddset() says that signal %d is an invalid "
				"or unsupported signal number.", i );
			} else {
				return mx_error(
				MXE_OPERATING_SYSTEM_ERROR, fname,
				"Unable to add signal %d to the blocked signal "
				"set with sigaddset().  "
				"Errno = %d, error message = '%s'.",
					i, saved_errno, strerror(saved_errno) );
			}
		}
	}	

	/* Block the unused realtime signals. */

	status = pthread_sigmask( SIG_BLOCK, &set, NULL );

	if ( status != 0 ) {
		switch( status ) {
		case EINVAL:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The first argument to pthread_sigmask() was invalid.");
			break;
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to block signals using pthread_sigmask().  "
			"Errno = %d, error message = '%s'.",
				status, strerror(status) );
			break;
		}
	}

	/* Create the mutex that is used to manage access to mx_signal_array. */

	mx_status = mx_mutex_create( &mx_signal_mutex );

	return mx_status;
}

MX_EXPORT int
mx_signals_are_initialized( void )
{
	if ( mx_signal_mutex != NULL ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

MX_EXPORT mx_status_type
mx_signal_allocate( int requested_signal_number,
			int *allocated_signal_number )
{
	static const char fname[] = "mx_signal_allocate()";

	int i;
	long status;
	mx_status_type mx_status;

	if ( mx_signals_are_initialized() == FALSE ) {
		mx_status = mx_signal_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	status = mx_mutex_lock( mx_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_signal_mutex." );
	}

	if ( requested_signal_number != MXF_ANY_REALTIME_SIGNAL ) {

		i = requested_signal_number - 1;

		if ( mx_signal_array[i] != FALSE ) {
			MX_SIGNAL_MUTEX_UNLOCK;
			return mx_error( MXE_NOT_AVAILABLE, fname,
				"Signal %d is already in use.",
				requested_signal_number );
		}

		*allocated_signal_number = requested_signal_number;
	} else {
		/* Search for an available realtime signal. */

		for ( i = (SIGRTMIN - 1); i < SIGRTMAX; i++ ) {
			if ( mx_signal_array[i] == FALSE ) {
				break;		/* Exit the for() loop. */
			}
		}

		if ( i >= SIGRTMAX ) {
			MX_SIGNAL_MUTEX_UNLOCK;
			return mx_error( MXE_NOT_AVAILABLE, fname,
		"All of the Posix realtime signals are already in use." );
		}

		*allocated_signal_number = i + 1;
	}

	mx_signal_array[i] = TRUE;

	MX_SIGNAL_MUTEX_UNLOCK;

#if MX_SIGNAL_DEBUG
	MX_DEBUG(-2,("%s: Allocated signal number %d",
			fname, *allocated_signal_number));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_signal_free( int signal_number )
{
	static const char fname[] = "mx_signal_free()";

	int i, was_in_use;
	long status;

	if ( mx_signals_are_initialized() == FALSE ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Attempted to free signal %d when signals had not yet "
		"been initialized.", signal_number );
	}

	i = signal_number - 1;

	if ( (i < 0) || (i >= mx_num_signals) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested signal number %d is outside "
		"the allowed range of %d to %d.", signal_number,
						1, MX_NUM_SIGNALS );
	}

	status = mx_mutex_lock( mx_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_signal_mutex." );
	}

	if ( mx_signal_array[i] == FALSE ) {
		was_in_use = FALSE;
	} else {
		was_in_use = TRUE;
	}

	mx_signal_array[i] = FALSE;

	MX_SIGNAL_MUTEX_UNLOCK;

	if ( was_in_use == FALSE ) {
		mx_warning("%s: Freeing realtime signal number %d "
		"although it was not in use.", fname, signal_number );
	}

#if MX_SIGNAL_DEBUG
	MX_DEBUG(-2,("%s: Freed signal number %d.", fname, signal_number));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

#elif defined( OS_VXWORKS )

/* For this platform, the only signal available is SIGALRM and we can use
 * sigaction() to see if SIGALRM is already in use.
 */

static MX_MUTEX *mx_signal_mutex = NULL;

static int mx_sigalrm_in_use = FALSE;

MX_EXPORT mx_status_type
mx_signal_initialize( void )
{
	mx_status_type mx_status;

	/* Create the mutex to manage checking of the sigalrm_in_use flag. */

	mx_status = mx_mutex_create( &mx_signal_mutex );

	return mx_status;
}

MX_EXPORT int
mx_signals_are_initialized( void )
{
	if ( mx_signal_mutex != NULL ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

MX_EXPORT mx_status_type
mx_signal_allocate( int requested_signal_number,
			int *allocated_signal_number )
{
	static const char fname[] = "mx_signal_allocate()";

	struct sigaction old_sa;
	int sigaction_status, saved_errno;
	long mutex_status;
	mx_status_type mx_status;

	if ( mx_signals_are_initialized() == FALSE ) {
		mx_status = mx_signal_initialize();

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	switch( requested_signal_number ) {
	case SIGALRM:
	case MXF_ANY_REALTIME_SIGNAL:
		/* SIGALRM is the only signal available. */
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Signal %d cannot be managed by MX on this platform.",
			requested_signal_number );
		break;
	}

	mutex_status = mx_mutex_lock( mx_signal_mutex );

	if ( mutex_status != MXE_SUCCESS ) {
		return mx_error( mutex_status, fname,
			"Unable to lock mx_signal_mutex." );
	}

	if ( mx_sigalrm_in_use ) {
		MX_SIGNAL_MUTEX_UNLOCK;
		return mx_error( MXE_NOT_AVAILABLE, fname,
			"Signal SIGALRM is already in use." );
	}

	/* Is SIGALRM already in use by something outside of MX? */

	sigaction_status = sigaction( SIGALRM, NULL, &old_sa );

	if ( sigaction_status < 0 ) {
		saved_errno = errno;
		MX_SIGNAL_MUTEX_UNLOCK;
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"An attempt to invoke sigaction() for signal SIGALRM "
			"failed with errno = %d, error message = '%s'.",
			saved_errno, strerror( saved_errno ) );
	}

	if ( old_sa.sa_handler != SIG_DFL ) {
		/* SIGALRM is not using the default handler, so we assume
		 * that it is already being used by something else.
		 */

		mx_sigalrm_in_use = TRUE;
		MX_SIGNAL_MUTEX_UNLOCK;
		return mx_error( MXE_NOT_AVAILABLE, fname,
			"Signal SIGALRM is already in use." );
	}

	/* Mark SIGALRM as in use. */

	*allocated_signal_number = SIGALRM;

	mx_sigalrm_in_use = TRUE;

	MX_SIGNAL_MUTEX_UNLOCK;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_signal_free( int signal_number )
{
	static const char fname[] = "mx_signal_free()";

	long status;

	if ( mx_signals_are_initialized() == FALSE ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Attempted to free signal %d when signals had not yet "
		"been initialized.", signal_number );
	}

	if ( signal_number != SIGALRM ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Signal %d cannot be managed by MX on this platform.",
			signal_number );
	}

	status = mx_mutex_lock( mx_signal_mutex );

	if ( status != MXE_SUCCESS ) {
		return mx_error( status, fname,
			"Unable to lock mx_signal_mutex." );
	}

	if ( mx_sigalrm_in_use == FALSE ) {
		mx_warning("%s: Freeing signal SIGALRM "
		"although it was not in use.", fname );
	}

	mx_sigalrm_in_use = FALSE;

	MX_SIGNAL_MUTEX_UNLOCK;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

#elif 0

/* For this platform, no signals are available, so attempts
 * to allocate signals fail.
 */

MX_EXPORT mx_status_type
mx_signal_initialize( void )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT int
mx_signals_are_initialized( void )
{
	return TRUE;
}

MX_EXPORT mx_status_type
mx_signal_allocate( int requested_signal_number,
			int *allocated_signal_number )
{
	static const char fname[] = "mx_signal_allocate()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Signal allocation is not supported on this platform" );
}

MX_EXPORT mx_status_type
mx_signal_free( int signal_number )
{
	static const char fname[] = "mx_signal_free()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Signal allocation is not supported on this platform" );
}

/*--------------------------------------------------------------------------*/

#else

#error Signal allocation functions are not yet defined for this platform.

#endif

