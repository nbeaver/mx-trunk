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
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_DEBUGGER			FALSE

#include <stdio.h>
#include <stdlib.h>
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

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_standard_signal_error_handler( int signal_number,
				siginfo_t *siginfo,
				void *ucontext )
{
	static char signal_name[80];
	static char directory_name[ MXU_FILENAME_LENGTH + 1 ];
	static int recursion = 0;

	(void) mx_get_current_directory_name( directory_name,
					      MXU_FILENAME_LENGTH );

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

	switch ( signal_number ) {
#ifdef SIGILL
	case SIGILL:
		strlcpy( signal_name, "SIGILL (illegal instruction)",
		  sizeof(signal_name) );
		break;
#endif
#ifdef SIGTRAP
	case SIGTRAP:
		strlcpy( signal_name, "SIGTRAP",
		  sizeof(signal_name) );
		break;
#endif
#ifdef SIGIOT
	case SIGIOT:
		strlcpy( signal_name, "SIGIOT (I/O trap)",
		  sizeof(signal_name) );
		break;
#endif
#ifdef SIGBUS
	case SIGBUS:
		strlcpy( signal_name, "SIGBUS (bus error)",
		  sizeof(signal_name) );
		break;
#endif
#ifdef SIGFPE
	case SIGFPE:
		strlcpy( signal_name, "SIGFPE (floating point exception)",
		  sizeof(signal_name) );
		break;
#endif
#ifdef SIGSEGV
	case SIGSEGV:
		strlcpy( signal_name, "SIGSEGV (segmentation violation)",
		  sizeof(signal_name) );
		break;
#endif
	default:
		snprintf( signal_name, sizeof(signal_name),
				"%d", signal_number );
		break;
	}

	mx_info( "CRASH: This program has died due to signal %s.\n",
			signal_name );

	mx_info( "Process id = %lu", mx_process_id() );

	/* Print out the stack traceback. */

	mx_stack_traceback();

	if ( mx_just_in_time_debugging_is_enabled() ) {
		mx_start_debugger(NULL);
	} else {
		/* Try to force a core dump. */

		mx_info("Attempting to force a core dump in '%s'.",
				directory_name );

		mx_force_core_dump();
	}
}

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_setup_standard_signal_error_handlers( void )
{
	struct sigaction sa;

	memset( &sa, 0, sizeof(sa) );

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = mx_standard_signal_error_handler;

#ifdef SIGILL
	sigaction( SIGILL, &sa, NULL );
#endif
#ifdef SIGTRAP
	sigaction( SIGTRAP, &sa, NULL );
#endif
#ifdef SIGIOT
	sigaction( SIGIOT, &sa, NULL );
#endif
#ifdef SIGBUS
	sigaction( SIGBUS, &sa, NULL );
#endif
#ifdef SIGFPE
	sigaction( SIGFPE, &sa, NULL );
#endif
#ifdef SIGSEGV
	sigaction( SIGSEGV, &sa, NULL );
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

	mx_info(
	"This should not be able to happen, so we are exiting instead...");

	exit(1);
}

/*-------------------------------------------------------------------------*/
