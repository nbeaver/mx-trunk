/*
 * Name:    mx_stack.c
 *
 * Purpose: MX functions to support run-time stack tracebacks.
 *
 *          This functionality is not available on all platforms.  Even if
 *          it is, it may not work reliably if the program is in the process
 *          of crashing when this function is invoked.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"

#include "mx_util.h"

#if defined( OS_SOLARIS )

#include <unistd.h>

/* Solaris 2 has a stack traceback program called 'ptrace' that uses the
 * /proc filesystem to do the traceback.
 */

MX_EXPORT void
mx_stack_traceback( void )
{
	FILE *file;
	static char buffer[200];

	sprintf( buffer, "/usr/proc/bin/pstack %ld", (long) getpid() );

	file = popen( buffer, "r" );

	if ( file == NULL ) {
		mx_info(
    "Unable to run /usr/proc/bin/pstack to get stack traceback information" );
		return;
	}

	fgets( buffer, sizeof buffer, file );

	while ( !feof( file ) ) {

		mx_info( buffer );

		fgets( buffer, sizeof buffer, file );
	}

	pclose(file);

	return;
}

#elif defined( OS_IRIX )

#   if !defined(__GNUC__)

    /* trace_back_stack_and_print() seems to work right for code compiled
     * with SGI's own C compiler, but not for GCC.
     */

#   include <libexc.h>

    MX_EXPORT void
    mx_stack_traceback( void )
    {
	(void) trace_back_stack_and_print();
    }
#   else /* __GNUC__ */

    MX_EXPORT void
    mx_stack_traceback( void )
    {
	mx_info("Stack traceback is not available on Irix "
		"for code compiled with GCC.");
    }
#   endif /* __GNUC__ */

#elif defined( OS_HPUX )

/* Reputedly, the undocumented function U_STACK_TRACE() will do the trick
 * on HP/UX.  I have not been able to test this though.  U_STACK_TRACE()
 * is found in libcl and requires adding -lcl to all link commands.
 */

extern void U_STACK_TRACE();

MX_EXPORT void
mx_stack_traceback( void )
{
	U_STACK_TRACE();
}

#elif defined( OS_DJGPP )

#include <signal.h>

MX_EXPORT void
mx_stack_traceback( void )
{
	__djgpp_traceback_exit( SIGABRT );
}

#elif defined( OS_WIN32 )

MX_EXPORT void
mx_stack_traceback( void )
{
	mx_info(
"Stack traceback has not yet been implemented for Microsoft Win32 programs." );

	return;
}

#elif ( (__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1)) )

/* GNU Glibc 2.1 introduces the backtrace() family of functions. */

/* MAXDEPTH is the maximum number of stack frames that will be dumped. */

#define MAXDEPTH	50

#include <execinfo.h>

MX_EXPORT void
mx_stack_traceback( void )
{
	static void *addresses[ MAXDEPTH ];
	int i, num_addresses;
	char **names;

	num_addresses = backtrace( addresses, MAXDEPTH );

	if ( num_addresses <= 0 ) {
		mx_info(
	"\nbacktrace() failed to return any stack traceback information." );

		return;
	}

	names = backtrace_symbols( addresses, num_addresses );

	mx_info( "\nStack traceback:\n" );

	for ( i = 0; i < num_addresses; i++ ) {
		mx_info( "%d: %s", i, names[i] );
	}

	if ( num_addresses < MAXDEPTH ) {
		mx_info( "\nStack traceback complete." );
	} else {
		mx_info(
		"\nStack traceback exceeded the maximum level of %d.",
			MAXDEPTH );
	}

	free( names );

	return;
}

#else

MX_EXPORT void
mx_stack_traceback( void )
{
	mx_info( "Stack traceback is not available." );

	return;
}

#endif

/*--------------------------------------------------------------------------*/

#if 1

MX_EXPORT int
mx_stack_check( void )
{
	return TRUE;
}

#endif
