/*
 * Name:    mx_signal.c
 *
 * Purpose: Here we define MX equivalents for sigaction() on platforms
 *          that do not support it.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2017-2019, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_osdef.h"

#if defined( OS_WIN32 )
#include <windows.h>
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_signal.h"
#include "mx_stdint.h"
#include "mx_thread.h"
#include "mx_debugger.h"

/*-------------------------------------------------------------------------*/

#if ( defined(OS_WIN32) && !defined(__MINGW32__) )

/* The following implements a minimal sigaction() by just redirecting
 * everything to signal().
 */

#define MXP_NUM_SIGNALS		25

typedef void (*mxp_signal_handler_type)(int);

typedef void (*mxp_signal_action_type)(int, siginfo_t *, void *);

#if 0
static mxp_signal_handler_type
mxp_defaults_array[MXP_NUM_SIGNALS] = {NULL};
#endif

static mxp_signal_action_type
mxp_action_array[MXP_NUM_SIGNALS] = {NULL};

static void
mxp_signal_to_sigaction_shim( int signum )
{
	mxp_signal_action_type act = NULL;
	siginfo_t si;

	if ( ( signum < 0 ) || ( signum >= MXP_NUM_SIGNALS) ) {
		return;
	}

	if ( mxp_action_array[signum] == NULL ) {
		return;
	}

	act = mxp_action_array[signum];

	memset( &si, 0, sizeof(si) );

	/* This implementation does not really know much about why
	 * it was called.
	 */

	si.si_signo = signum;

	act( signum, &si, NULL );

	return;
}

MX_EXPORT int
sigaction( int signum,
	const struct sigaction *sa,
	struct sigaction *old_sa )
{
	mxp_signal_handler_type handler_fn = NULL;
	mxp_signal_action_type sigaction_fn = NULL;

	mxp_signal_handler_type old_handler_fn = NULL;
	mxp_signal_action_type old_sigaction_fn = NULL;

	if ( (signum < 0) || (signum >= MXP_NUM_SIGNALS) ) {
	}

	if ( sa != (const struct sigaction *) NULL ) {

		if ( sa->sa_flags & SA_SIGINFO ) {
			sigaction_fn = sa->sa_sigaction;

			old_sigaction_fn = mxp_action_array[signum];

			old_handler_fn = signal( signum,
					mxp_signal_to_sigaction_shim );

			if ( old_handler_fn == SIG_ERR ) {
				return (-1);
			}

			mxp_action_array[signum] = sigaction_fn;
		} else {
			handler_fn = sa->sa_handler;

			old_handler_fn = signal( signum, handler_fn );

			if ( old_handler_fn == SIG_ERR ) {
				return (-1);
			}

			if ( old_sa != (struct sigaction *) NULL ) {
				memset( old_sa, 0, sizeof(struct sigaction) );

				old_sa->sa_handler = old_handler_fn;
			}

			return 0;
		}
	}

	/* Returning the previous signal handler is not implemented
	 * if we did not just install a new handler.
	 */

	if ( old_sa != (struct sigaction *) NULL ) {
		errno = EINVAL;
		return (-1);
	}

	return 0;
}

#endif

/*-------------------------------------------------------------------------*/

static void
mxp_standard_signal_error_handler( int signal_number,
					siginfo_t *info,
					void *ucontext )
{
	static char directory_name[ MXU_FILENAME_LENGTH + 1 ];
	static int recursion = 0;

	if ( recursion ) {
		mx_warning( "We seem to be crashing inside the signal handler, "
				"so it is best to give up now.  Exiting...");

#if defined(OS_VXWORKS)
		exit(1);
#else
		_exit(1);
#endif
	}

	recursion++;

#if 1
	MX_DEBUG(-2,
    ("siginfo_t: si_signo = %d, si_errno = %d, si_code = %d, si_addr = %p\n",
		info->si_signo, info->si_errno, info->si_code, info->si_addr));
#endif

	switch ( signal_number ) {
#ifdef SIGILL
	case SIGILL:
		mx_info( "CRASH: This program has died due to signal SIGILL.\n"
			"  Illegal instruction at ptr = %p \n",
			info->si_addr );
		break;
#endif
#ifdef SIGTRAP
	case SIGTRAP:
		mx_info( "CRASH: This program has died due to signal SIGTRAP.\n"
			"  Trap at ptr = %p \n",
			info->si_addr );
		break;
#endif
#ifdef SIGIOT
	case SIGIOT:
		mx_info( "CRASH: This program has died due to signal SIGIOT.\n"
			"  I/O trap \n" );
		break;
#endif
#ifdef SIGBUS
	case SIGBUS:
		mx_info( "CRASH: This program has died due to signal SIGBUS.\n"
			"  Bus error at ptr = %p \n",
			info->si_addr );
		break;
#endif
#ifdef SIGFPE
	case SIGFPE:
		mx_info( "CRASH: This program has died due to signal SIGFPE.\n"
			"  Floating point exception at ptr = %p \n",
			info->si_addr );
		break;
#endif

/*--------------------*/

#ifdef SIGSEGV
	case SIGSEGV:
		mx_info( "CRASH: This program has died due to signal SIGSEGV.\n"
			"  Segmentation violation at ptr = %p",
			info->si_addr );

		switch( info->si_code ) {
#  ifdef SEGV_MAPERR
		case SEGV_MAPERR:
			mx_info( "  Error code = SEGV_MAPERR (%d), "
				"(Address not mapped to object).\n",
						info->si_code );
			break;
#  endif
#  ifdef SEGV_ACCERR
		case SEGV_ACCERR:
			mx_info( "  Error code = SEGV_ACCERR (%d), "
				"(Invalid permissions for mapped object).\n",
						info->si_code );
			break;
#  endif
#  ifdef SEGV_BNDERR
		case SEGV_BNDERR:
			mx_info( "  Error code = SEGV_BNDERR (%d), "
				"(Failed address bound checks).\n",
						info->si_code );
			break;
#  endif
#  ifdef SEGV_PKUERR
		case SEGV_PKUERR:
			mx_info( "  Error code = SEGV_PKUERR (%d), "
			    "(Access was denied by memory protection keys).\n",
						info->si_code );
			break;
#  endif
		default:
			mx_info( " Error code = %d", info->si_code );
			break;
		}
		break;

#endif /* SIGSEGV */

/*--------------------*/

	default:
		mx_info( "CRASH: This program has died due to signal %d.\n",
			signal_number );
		break;
	}

	mx_info( "Process id = %lu", mx_process_id() );

	/* Print out the stack traceback. */

	mx_stack_traceback();

	if ( mx_just_in_time_debugging_is_enabled() ) {
		mx_start_debugger(NULL);
	} else {
		/* Try to force a core dump. */

		(void) mx_get_current_directory_name( directory_name,
					      MXU_FILENAME_LENGTH );

		mx_info("Attempting to force a core dump in '%s'.",
				directory_name );

		mx_force_core_dump();
	}
}

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_setup_default_signal_handler( int signum )
{
	static const char fname[] = "mx_setup_default_signal_handler()";

	struct sigaction act;
	int signal_status, saved_errno;

	memset( &act, 0, sizeof(act) );

	act.sa_flags = SA_SIGINFO;

	act.sa_sigaction = mxp_standard_signal_error_handler;

	signal_status = sigaction( signum, &act, NULL );

	if ( signal_status != 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt by sigaction() to set up the MX standard signal "
		"handler for signal %d failed with "
		"errno = %d, error message = '%s'.",
			signum, saved_errno, strerror(saved_errno) );
	}

	return;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_setup_standard_signal_error_handlers( void )
{
#ifdef SIGILL
	mx_setup_default_signal_handler( SIGILL );
#endif
#ifdef SIGTRAP
	mx_setup_default_signal_handler( SIGTRAP );
#endif
#ifdef SIGIOT
	mx_setup_default_signal_handler( SIGIOT );
#endif
#ifdef SIGBUS
	mx_setup_default_signal_handler( SIGBUS );
#endif
#ifdef SIGFPE
	mx_setup_default_signal_handler( SIGFPE );
#endif
#ifdef SIGSEGV
	mx_setup_default_signal_handler( SIGSEGV );
#endif
	return;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_force_core_dump( void )
{
#if defined( SIGABRT )
	signal( SIGABRT, SIG_DFL );

	raise(SIGABRT);

#elif defined( SIGABORT )
	signal( SIGABORT, SIG_DFL );

	raise(SIGABORT);

#elif defined( SIGQUIT )
	signal( SIGQUIT, SIG_DFL );

	raise(SIGQUIT);

#else
	abort();
#endif

	mx_info("The attempt to force a core dump did not work!!!.");

	mx_info("Therefore, we are exiting instead...");

	exit(1);
}

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_force_immediate_exit( void )
{

#if defined( OS_WIN32 )
	TerminateProcess( GetCurrentProcess(), 0 );

#elif defined( OS_LINUX )
	_exit(1);
#else
#  error mx_force_immediate_exit() is not yet implemented for this build target.
#endif

}

/*-------------------------------------------------------------------------*/
