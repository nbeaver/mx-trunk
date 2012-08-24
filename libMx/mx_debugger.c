/*
 * Name:    mx_debugger.c
 *
 * Purpose: Functions for handling the interaction of MX programs with
 *          external debuggers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_DEBUGGER			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "mx_osdef.h"

#if defined( OS_WIN32 )
#include <windows.h>
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_stdint.h"

#if ( defined(OS_WIN32) && (_WIN32_WINNT >= 0x0400) ) || defined(OS_MACOSX)
#  define USE_MX_DEBUGGER_IS_PRESENT	TRUE
#else
#  define USE_MX_DEBUGGER_IS_PRESENT	FALSE
#endif

/*-------------------------------------------------------------------------*/

static mx_bool_type mx_just_in_time_debugging_enabled = FALSE;

static char mx_debugger_command[MXU_FILENAME_LENGTH+1] = "";

/*-------------------------------------------------------------------------*/

MX_EXPORT void
mx_standard_signal_error_handler( int signal_number )
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

	if ( mx_just_in_time_debugging_enabled ) {
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

/* mx_breakpoint_helper() is a tiny function that can be used as the
 * target of a debugger breakpoint, assuming it hasn't been optimized
 * away by compiler optimization.  It exists because some versions of
 * GDB have problems with setting breakpoints in C++ constructors.
 * By adding a call to the empty mx_breakpoint_helper() function, it
 * now becomes easy to set a breakpoint in a constructor.
 */

MX_EXPORT int
mx_breakpoint_helper( void )
{
	/* Make it harder for the optimizer to optimize us away. */

	volatile int i;

	i = 42;

	return i;
}

/*-------------------------------------------------------------------------*/

/* mx_breakpoint() inserts a debugger breakpoint into the code.
 * It is not possible to implement this for all platforms.
 */

static mx_bool_type mx_debugger_started = FALSE;

/* Note: The argument to mx_set_debugger_started_flag() is an int, rather
 * than an mx_bool_type, so that we will not need to define mx_bool_type
 * in mx_util.h at that point.
 */

MX_EXPORT void
mx_set_debugger_started_flag( int flag )
{
	if ( flag ) {
		mx_debugger_started = TRUE;
	} else {
		mx_debugger_started = FALSE;
	}
}

MX_EXPORT int
mx_get_debugger_started_flag( void )
{
	return mx_debugger_started;
}

/*----------------*/

#if defined(OS_WIN32)

MX_EXPORT void
mx_breakpoint( void )
{
	mx_debugger_started = TRUE;

	DebugBreak();
}

#elif ( defined(__GNUC__) \
	&& (defined(__i386__) || defined(__x86_64__)) \
	&& (!defined(OS_SOLARIS)) )

MX_EXPORT void
mx_breakpoint( void )
{
	if ( mx_debugger_started ) {
		__asm__("int3");
	} else {
		mx_start_debugger(NULL);
	}
}

#else

/* For unsupported platforms, we just "implement" it as
 * a call to mx_breakpoint_helper() above.
 */

MX_EXPORT void
mx_breakpoint( void )
{
	if ( mx_debugger_started ) {
		mx_warning( "Calling mx_breakpoint_helper()." );

		mx_breakpoint_helper();
	} else {
		mx_warning(
		"This platform does not directly support mx_breakpoint() "
		"and will use mx_breakpoint_helper() instead.  You should "
		"set a breakpoint for mx_breakpoint_helper() now "
		"for proper debugging." );

		mx_start_debugger(NULL);
	}
}

#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

MX_EXPORT void
mx_prepare_for_debugging( char *command, int just_in_time_debugging )
{
	mx_just_in_time_debugging_enabled = FALSE;
	mx_debugger_command[0] = '\0';

	if ( just_in_time_debugging ) {
		mx_just_in_time_debugging_enabled = TRUE;
	} else {
		mx_just_in_time_debugging_enabled = FALSE;
	}
	return;
}

MX_EXPORT void
mx_start_debugger( char *command )
{
	fprintf(stderr,
	    "\nWarning: The Visual C++ debugger is being started.\n\n");
	fflush(stderr);

	mx_debugger_started = TRUE;

	DebugBreak();

	return;
}

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_HURD)

MX_EXPORT void
mx_prepare_for_debugging( char *command, int just_in_time_debugging )
{
	static const char fname[] = "mx_prepare_for_debugging()";

	char terminal[80];
	char *ptr;
	unsigned long pid;
	mx_bool_type x_windows_available;

	static char unsafe_fmt[] =
    "Unsafe command line '%s' requested for %s.  The command will be ignored.";

#if DEBUG_DEBUGGER
	fprintf( stderr, "\n%s: command = '%s', just_in_time_debugging = %d\n",
		fname, command, just_in_time_debugging );
#endif

	if ( just_in_time_debugging ) {
		mx_just_in_time_debugging_enabled = TRUE;
	} else {
		mx_just_in_time_debugging_enabled = FALSE;
	}

	mx_debugger_started = TRUE;

	pid = mx_process_id();

	/* We try several things in order here. */

	/* If the command line pointer is NULL, see if we can use the contents
	 * of the environment variable MX_DEBUGGER instead.
	 */

	if ( command == NULL ) {
		command = getenv( "MX_DEBUGGER" );
	}

	/* If a command line has been specified, attempt to use it.  If the
	 * format specifier %lu appears in the command line, it will be
	 * replaced by the process id.
	 */

	if ( command != (char *) NULL ) {

		/* Does the command line include any % characters? */

		ptr = strchr( command, '%' );

		if ( ptr == NULL ) {
			strlcpy( mx_debugger_command, command,
					sizeof(mx_debugger_command) );
		} else {
			/* If so, then we must check to see if the command
			 * line is safe to use as a format string.
			 */

			/* Does the command contain more than one % character?
			 * If so, that is prohibited.
			 */

			if ( strchr( ptr+1, '%' ) != NULL ) {
				mx_warning( unsafe_fmt, command, fname );
				return;
			}

			/* Is the conversion specifier anything other than
			 * '%lu'?.  If so, that is prohibited.
			 */

			if ( strncmp( ptr, "%lu", 3 ) != 0 ) {
				mx_warning( unsafe_fmt, command, fname );
				return;
			}

			/* The format string appears to be safe, so let's use
			 * it to format the command to execute.
			 */

			snprintf( mx_debugger_command,
				sizeof(mx_debugger_command),
					command, pid );
		}
	} else {
		/* If no command line was specified, then we try several
		 * commands in turn.
		 */

		mx_debugger_command[0] = '\0';

		if ( getenv("DISPLAY") != NULL ) {
			x_windows_available = TRUE;
		} else {
			x_windows_available = FALSE;
		}

		if ( x_windows_available ) {

			/* Look for an appropriate terminal emulator. */

			/* xterm is almost always available. */

			strlcpy( terminal, "xterm -geometry 95x52 -e",
					sizeof(terminal) );

			/* Look for an appropriate debugger. */

#if defined(OS_LINUX)
			if ( mx_command_found( "ddd" ) ) {

				snprintf( mx_debugger_command,
					sizeof(mx_debugger_command),
					"ddd --gdb -p %lu", pid );
			} else
#endif
#if defined(OS_SOLARIS)
			if ( mx_command_found( "sunstudio" ) ) {

				snprintf( mx_debugger_command,
					sizeof(mx_debugger_command),
					"sunstudio -A %lu", pid );
			} else
#endif
			if ( mx_command_found( "gdbtui" ) ) {

				snprintf( mx_debugger_command,
					sizeof(mx_debugger_command),
					"%s gdbtui -p %lu", terminal, pid );
			} else
			if ( mx_command_found( "gdb" ) ) {

				snprintf( mx_debugger_command,
					sizeof(mx_debugger_command),
					"%s gdb -p %lu", terminal, pid );
			} else
			if ( mx_command_found( "dbx" ) ) {

				snprintf( mx_debugger_command,
					sizeof(mx_debugger_command),
					"%s dbx - %lu", terminal, pid );
			}
		}
	}

#if DEBUG_DEBUGGER
	fprintf( stderr, "debugger command = '%s'\n",
			mx_debugger_command );
#endif
}

MX_EXPORT void
mx_start_debugger( char *command )
{
	unsigned long spawn_flags;
	mx_status_type mx_status;

	/* If we specify a new command line here, then we override any
	 * previously specified debugger command line.
	 */

	if ( ( command != NULL ) || ( mx_debugger_command[0] == '\0' ) ) {

#if DEBUG_DEBUGGER
		fprintf( stderr,
		"\nWarning: Replacing previous debugger command.\n" );
#endif

		mx_prepare_for_debugging( command,
			mx_just_in_time_debugging_enabled );
	}
#if DEBUG_DEBUGGER
	else {
		fprintf( stderr,
		"\nWarning: Keeping previous debugger command.\n" );
	}
#endif

	if ( mx_debugger_command[0] != '\0' ) {

		/* If a debugger has been found, start it. */

		fputs( "\nWarning: Starting a debugger using the command '",
			stderr );
		fputs( mx_debugger_command, stderr );
		fputs( "'.\n", stderr );

		spawn_flags = 0;

#if defined(OS_LINUX)
		spawn_flags |= MXF_SPAWN_NO_PRELOAD;
#endif

		mx_status = mx_spawn( mx_debugger_command, spawn_flags, NULL );

		/* See if starting the debugger succeeded. */

		if ( mx_status.code == MXE_SUCCESS ) {

			/* Wait for the debugger here. */

			mx_wait_for_debugger();

			return;
		}

		/* If starting the debugger did not succeed,
		 * try the backup strategy.
		 */
	}

	/* If no suitable debugger was found, wait for the debugger here. */

	fprintf( stderr, "\nWarning: No suitable GUI debugger was found.\n\n" );

	mx_wait_for_debugger();

	return;
}

#else

MX_EXPORT void
mx_prepare_for_debugging( char *command, int just_in_time_debugging )
{
	mx_debugger_command[0] = '\0';
	mx_just_in_time_debugging_enabled = FALSE;

	fprintf(stderr, "\n"
"Warning: Starting a debugger is not currently supported on this platform.\n"
"         The request to start a debugger will be ignored.  Sorry...\n\n" );
	fflush(stderr);
	return;
}

MX_EXPORT void
mx_start_debugger( char *command )
{
	fprintf(stderr, "\n"
"Warning: Starting a debugger is not currently supported on this platform.\n"
"         The request to start a debugger will be ignored.  Sorry...\n\n" );
	fflush(stderr);
	return;
}

#endif

/*-------------------------------------------------------------------------*/

/* Note: mx_debugger_is_present() is not designed to be used for 
 * copy protection purposes, so it does not make extreme attempts
 * to detect a debugger that is trying to hide itself.
 */

#if defined(OS_WIN32) && (_WIN32_WINNT >= 0x0400)

MX_EXPORT int
mx_debugger_is_present( void )
{
	int result;

	result = (int) IsDebuggerPresent();

	return result;
}

#elif defined(OS_MACOSX)

/* Based on
 *   http://www.wodeveloper.com/omniLists/macosx-dev/2004/June/msg00166.html
 */

#include <sys/sysctl.h>

MX_EXPORT int
mx_debugger_is_present( void )
{
	static const char fname[] = "mx_debugger_is_present()";

	int mib[4];
	int p_flag, os_status, saved_errno;
	struct kinfo_proc pinfo;
	size_t pinfo_size = sizeof(pinfo);

	/* Get process info. */

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = getpid();

	os_status = sysctl( mib, 4, &pinfo, &pinfo_size, NULL, 0 );

	if ( os_status == -1 ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get the kinfo_proc structure for this process.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );

		return FALSE;
	}

	/* Look for the P_TRACED flag. */

	p_flag = pinfo.kp_proc.p_flag;

	if ( p_flag & P_TRACED ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#else

MX_EXPORT int
mx_debugger_is_present( void )
{
	if ( mx_debugger_started ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#endif

/*-------------------------------------------------------------------------*/

#if USE_MX_DEBUGGER_IS_PRESENT

MX_EXPORT void
mx_wait_for_debugger( void )
{
	unsigned long pid;
	int present;

	pid = mx_process_id();

	fprintf( stderr,
	"\nProcess %lu is now waiting to be attached to by a debugger...\n\n",
		pid );

	while (1) {
		mx_msleep(10);

		present = mx_debugger_is_present();

		if ( present ) {
			break;
		}
	}

	return;
}

/*------*/

#else

/* This version always work, but is inelegant. */

MX_EXPORT void
mx_wait_for_debugger( void )
{
	volatile int loop;
	unsigned long pid;

	pid = mx_process_id();

	fprintf( stderr,
"\n"
"Process %lu is now waiting in an infinite loop for you to attach to it\n"
"with a debugger.\n\n", pid );

	fprintf( stderr,
"If you are using GDB, follow this procedure:\n"
"  1.  If not already attached, attach to the process with the command\n"
"          'gdb -p %lu'\n"
"  2.  Type 'set loop=0' in the mx_wait_for_debugger() stack frame\n"
"      to break out of the loop.\n"
"  3.  Use the command 'finish' to run functions to completion.\n\n", pid );
		
#if defined(OS_SOLARIS) || defined(OS_HPUX) || defined(OS_IRIX)
	fprintf( stderr,
"If you are using DBX, follow this procedure:\n"
"  1.  If not already attached, attach to the process with the command\n"
"          'dbx - %lu'\n"
"  2.  Type 'assign loop=0' in the mx_wait_for_debugger() stack frame\n"
"      to break out of the loop.\n"
"  3.  Use the command 'step up' to run functions to completion.\n\n", pid );
#endif
		
	fprintf( stderr, "Waiting...\n\n" );

	/* Synchronize with the debugger by waiting for the debugger
	 * to reset the value of 'loop' to 0.
	 */

	loop = 1;

	while ( loop ) {
		mx_msleep(100);
	}

	return;
}

#endif

/*-------------------------------------------------------------------------*/

