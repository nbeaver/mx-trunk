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
 * Copyright 1999-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_DEBUGGER			FALSE

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_osdef.h"

#if defined( OS_WIN32 )
#  include <windows.h>
#endif

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_signal.h"
#include "mx_stdint.h"
#include "mx_thread.h"
#include "mx_array.h"

#if ( defined(OS_WIN32) && (_WIN32_WINNT >= 0x0400) ) || defined(OS_MACOSX)
#  define USE_MX_DEBUGGER_IS_PRESENT	TRUE
#else
#  define USE_MX_DEBUGGER_IS_PRESENT	FALSE
#endif

/*-------------------------------------------------------------------------*/

static mx_bool_type mxp_just_in_time_debugging_enabled = FALSE;

static char mx_debugger_command[MXU_FILENAME_LENGTH+1] = "";

/*-------------------------------------------------------------------------*/

MX_EXPORT int
mx_just_in_time_debugging_is_enabled( void )
{
	return mxp_just_in_time_debugging_enabled;
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
	/* Make it a bit harder for the optimizer to optimize us away. */

	volatile int i;

	i = 42;

	return i;
}

/*-------------------------------------------------------------------------*/

/* mx_breakpoint() inserts a debugger breakpoint into the code.
 * It is not possible to implement this for all platforms.
 */

static mx_bool_type mxp_debugger_started = FALSE;

/* Note: The argument to mx_set_debugger_started_flag() is an int, rather
 * than an mx_bool_type, so that we will not need to define mx_bool_type
 * in mx_util.h at that point.
 */

MX_EXPORT void
mx_set_debugger_started_flag( int flag )
{
	if ( flag ) {
		mxp_debugger_started = TRUE;
	} else {
		mxp_debugger_started = FALSE;
	}
}

MX_EXPORT int
mx_get_debugger_started_flag( void )
{
	return mxp_debugger_started;
}

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

MX_EXPORT void
mx_breakpoint( void )
{
	mxp_debugger_started = TRUE;

	DebugBreak();
}

/*----------------*/

#elif ( defined(__GNUC__) \
	&& (defined(__i386__) || defined(__x86_64__)) \
	&& (!defined(OS_SOLARIS)) )

MX_EXPORT void
mx_breakpoint( void )
{
	if ( mxp_debugger_started ) {
		__asm__("int3");
	} else {
		mxp_debugger_started = TRUE;

		mx_start_debugger(NULL);
	}
}

/*----------------*/

#else

/* For unsupported platforms, we just "implement" it as
 * a call to mx_breakpoint_helper() above.
 */

MX_EXPORT void
mx_breakpoint( void )
{
	if ( mxp_debugger_started ) {
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
	UINT error_mode;

	mxp_just_in_time_debugging_enabled = FALSE;
	mx_debugger_command[0] = '\0';

	if ( just_in_time_debugging ) {
		mxp_just_in_time_debugging_enabled = TRUE;
	} else {
		mxp_just_in_time_debugging_enabled = FALSE;
	}

	/* Cygwin suppresses crash messages if we are running a Win32
	 * version of MX from a Cygwin process such as mintty.  We do
	 * not want that, so we must explicitly enable crash messages
	 * here.  We do not need to test for Cygwin explicitly, since
	 * we always want these crash messages regardless of how we
	 * are run.
	 */

	/* FIXME: If MX ever runs in a situation without a console, such
	 * as in a Win32 service, then we will need to revisit this
	 * choice.
	 */

	error_mode = SetErrorMode( 0 );

	/* For reference, Cygwin sets the error mode to 0x3, which sets
	 * the SEM_FAILCRITICALERRORS(0x1) and SEM_NOGPFAULTERRORBOX(0x2)
	 * error bits.  That is enough to suppress the message box.
	 */

#if 0
	MX_DEBUG(-2,
		("mx_prepare_for_debugging(): error_mode = %#lx", error_mode));
#endif
	return;
}

MX_EXPORT void
mx_start_debugger( char *command )
{
	fprintf(stderr,
	    "\nWarning: The Visual C++ debugger is being started.\n\n");
	fflush(stderr);

	mxp_debugger_started = TRUE;

	DebugBreak();

	return;
}

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_BSD) || defined(OS_HURD)

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
		mxp_just_in_time_debugging_enabled = TRUE;
	} else {
		mxp_just_in_time_debugging_enabled = FALSE;
	}

	/* FIXME: Why was the following write to mxp_debugger_started here
	 * and why has it never bitten us before?  (WML 2017-12-01)
	 */
#if 0
	mxp_debugger_started = TRUE;
#endif

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
			if ( mx_command_found( "dbxtool" ) ) {

				snprintf( mx_debugger_command,
					sizeof(mx_debugger_command),
					"dbxtool - %lu", pid );
			} else
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
			mxp_just_in_time_debugging_enabled );
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

#if 1
		if ( strncmp( mx_debugger_command, "ddd", 3 ) == 0 ) {
			char *home_name;

			home_name = getenv( "HOME" );

			if ( home_name != NULL ) {
				fprintf( stderr,
"\nWarning: If DDD hangs on startup with an hourglass cursor, try deleting\n"
"         the directory %s/.ddd and then try DDD again.\n", home_name );
			} else {
				fprintf( stderr,
"\nWarning: If DDD hangs on startup with an hourglass cursor, try deleting\n"
"         the directory .ddd in your home directory and then try DDD again.\n");
			}
		}
#endif

#if 0
		spawn_flags = MXF_SPAWN_NEW_SESSION;
#else
		spawn_flags = 0;
#endif

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

#elif defined(OS_CYGWIN)

static mx_bool_type mxp_cygwin_has_console = FALSE;

MX_EXPORT void
mx_prepare_for_debugging( char *command, int just_in_time_debugging )
{
#if 1
	static const char fname[] = "mx_prepare_for_debugging()";
#endif

	char buffer[200];
	char *ptr = NULL;
	FILE *query_session = NULL;

	/* If this Cygwin process is running on the Windows console,
	 * then we can use 'mintty' or 'rxvt' to host the 'gdb'
	 * debugger in another window.  Otherwise, we will have to
	 * wait for the user to attach from another session.
	 *
	 * We use the Windows 'query session' command to find out
	 * whether we are on the console or not.  Our session will
	 * have a '>' character as the first byte in the output of
	 * the 'query.exe' command if that line corresponds to our
	 * session.
	 */

	mx_debugger_command[0] = '\0';

	query_session = popen( "query session", "r" );

	if ( query_session == (FILE *) NULL ) {
		/* We could not find the 'query.exe' program, so we
		 * default to assuming that we are not on the console.
		 */

		return;
	}

	while (TRUE) {
		ptr = mx_fgets( buffer, sizeof(buffer), query_session );

		if ( ptr == (char *) NULL ) {
			break;
		}

		/* Is this our session? */

		if ( buffer[0] == '>' ) {
			/* This line corresponds to our session. */

			ptr = buffer + 1;

			if ( strncmp( ptr, "console", 7 ) == 0 ) {
				/* Kanpai!  We are running on the console! */

				mxp_cygwin_has_console = TRUE;
			}

			/* Only one line in the output of 'query.exe'
			 * corresponds to our session, so we do not
			 * need to read any more of the output from
			 * the 'query.exe' command.
			 */

			break;
		}
	}

	(void) fclose( query_session );

#if 0
	MX_DEBUG(-2,("%s: mxp_cygwin_has_console = %d",
		fname, (int) mxp_cygwin_has_console ));
#endif

	if ( mxp_cygwin_has_console == FALSE ) {
		mx_debugger_command[0] = '\0';

	} else {
		/* Now we figure out which command to use to start
		 * the debugger.
		 */

		if ( command != (char *) NULL ) {
			strlcpy( mx_debugger_command,
				command, sizeof(mx_debugger_command) );
		} else
		if ( mx_command_found( "gdb" ) == FALSE ) {
			/* If we get here, we have a console but we do not
			 * have a copy of 'gdb' to start.
			 */

#if 1
			MX_DEBUG(-2,("%s: 'gdb' not found.", fname));
#endif

			mxp_cygwin_has_console = FALSE;
			return;
		} else
		if ( mx_command_found( "mintty" ) ) {
			snprintf( mx_debugger_command,
				sizeof(mx_debugger_command),
				"mintty -e gdb -tui -p %lu",
				mx_process_id() );
		} else
		if ( mx_command_found( "rxvt" ) ) {
			snprintf( mx_debugger_command,
				sizeof(mx_debugger_command),
				"rxvt -e gdb -tui -p %lu",
				mx_process_id() );
		} else {
			MX_DEBUG(-2,
			("%s: Neither 'mintty' nor 'rxvt' were found, "
			 "so we cannot start 'gdb' in another window.", fname));
		}

#if 0
		MX_DEBUG(-2,("%s: mx_debugger_command = '%s'",
				fname, mx_debugger_command));
#endif
	}

	return;
}

MX_EXPORT void
mx_start_debugger( char *command )
{
	unsigned long spawn_flags;
	mx_status_type mx_status;

	if ( mxp_cygwin_has_console == FALSE ) {
		mx_wait_for_debugger();
		return;
	}

	spawn_flags = 0;

	mx_status = mx_spawn( mx_debugger_command, spawn_flags, NULL );

	mx_wait_for_debugger();

	return;
}

#else

MX_EXPORT void
mx_prepare_for_debugging( char *command, int just_in_time_debugging )
{
	mx_debugger_command[0] = '\0';
	mxp_just_in_time_debugging_enabled = FALSE;
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
	if ( mxp_debugger_started ) {
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
"  2.  Repeatedly type 'finish' until you reach the mx_wait_for_debugger()\n"
"      stack frame.\n"
"  3.  Type 'set loop=0' in the mx_wait_for_debugger() stack frame\n"
"      to break out of the loop.\n"
"  4.  You may now run any debugger commands you want.\n", pid );

	fprintf( stderr,
"\n"
"For GDB, make sure that you have made debugging symbols available to the\n"
"debugger for dynamically loaded libraries or modules such as .mxo files\n"
"using the GDB command 'set solib-search-path'.  You can verify that the\n"
"library or module has been found using the GDB 'sharedlibrary' command.\n\n" );

#if defined(OS_CYGWIN)
	fprintf( stderr,
"If you are using Cygwin Gdb, make sure that you move to the first thread\n"
"using the command 'thread 1', since that is the thread that contains the\n"
"loop in mx_wait_for_debugger().\n\n" );
#endif
		
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

#define MXU_MAX_NUMBERED_BREAKPOINTS	10

static mx_bool_type
mxp_numbered_breakpoint_array[MXU_MAX_NUMBERED_BREAKPOINTS] = {0};

MX_EXPORT void
mx_numbered_breakpoint( unsigned long breakpoint_number )
{
	static const char fname[] = "mx_numbered_breakpoint()";

	if ( breakpoint_number >= MXU_MAX_NUMBERED_BREAKPOINTS ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested breakpoint number %lu is outside the "
		"allowed range of values from 0 to %d",
			breakpoint_number,
			MXU_MAX_NUMBERED_BREAKPOINTS-1 );
		return;
	}

	if ( mxp_numbered_breakpoint_array[breakpoint_number] ) {
		mx_breakpoint();
	}
	return;
}

MX_EXPORT void
mx_set_numbered_breakpoint( unsigned long breakpoint_number,
				int breakpoint_on )
{
	static const char fname[] = "mx_set_numbered_breakpoint()";

	if ( breakpoint_number >= MXU_MAX_NUMBERED_BREAKPOINTS ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested breakpoint number %lu is outside the "
		"allowed range of values from 0 to %d",
			breakpoint_number,
			MXU_MAX_NUMBERED_BREAKPOINTS-1 );
		return;
	}

	mxp_numbered_breakpoint_array[breakpoint_number]
		= (mx_bool_type) breakpoint_on;

	return;
}

MX_EXPORT int
mx_get_numbered_breakpoint( unsigned long breakpoint_number )
{
	static const char fname[] = "mx_set_numbered_breakpoint()";

	int result;

	if ( breakpoint_number >= MXU_MAX_NUMBERED_BREAKPOINTS ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested breakpoint number %lu is outside the "
		"allowed range of values from 0 to %d",
			breakpoint_number,
			MXU_MAX_NUMBERED_BREAKPOINTS-1 );
		return FALSE;
	}

	result = (int) mxp_numbered_breakpoint_array[breakpoint_number];

	return result;
}

/*-------------------------------------------------------------------------*/

/* Verify that the largest watchpoint value is 64 bits or less.  In doing this,
 * we _assume_ that 'double' values are 64 bits (i.e. IEEE floating point).
 *
 * Regrettably, ANSI C does not let you use sizeof() in a preprocessor macro.
 */

#if ( defined(OS_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1400) )
    /* Versions of Visual Studio prior to Visual Studio 2005 do not support
     * 64 bit programs and will choke on the UINTMAX_MAX comparison below,
     * so we do not attempt to make that test.
     */
#else
#   if ( UINTMAX_MAX > 0xFFFFFFFFFFFFFFFF )
#     error The maximum integer size is greater than 64 bits.  This will require enlarging the old_value field of MX_WATCHPOINT in libMx/mx_util.h
#   endif
#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

typedef struct {
	HANDLE data_thread_handle;
	HANDLE debugger_thread_handle;
} MX_WIN32_WATCHPOINT_PRIVATE;

/*---*/

/* NOTE: This can only be implemented as an external process, since _ALL_
 * threads in the process are stopped when WaitForDebugEvent() returns.
 */

static mx_status_type
mxp_win32_set_watchpoint_spectator( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxp_win32_set_watchpoint_spectator()";

	MX_WATCHPOINT *watchpoint = NULL;
	MX_WIN32_WATCHPOINT_PRIVATE *win32_private = NULL;
	HANDLE parent_thread_handle;
	CONTEXT parent_context;
	long last_error_code;
	TCHAR error_message[100];
	BOOL os_status;
	DWORD64 dr7_bits;
	int debug_reg_index;
	size_t value_length;
	void * value_pointer;
	unsigned long flags, debug_extension_flags;
	unsigned long  debug_extension_value, debug_extension_mask;
	unsigned long size_option, size_selector, size_mask;

	watchpoint = (MX_WATCHPOINT *) args;

	win32_private = (MX_WIN32_WATCHPOINT_PRIVATE *)
				watchpoint->watchpoint_private;

	parent_thread_handle = win32_private->data_thread_handle;

	/* Suspend the parent thread so that we can modify its context. */

	os_status = SuspendThread( parent_thread_handle );

	if ( os_status == (DWORD)(-1) ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
					error_message,
					sizeof(error_message) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call SuspendThread() for parent "
		"thread handle %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			parent_thread_handle,
			last_error_code,
			error_message );
	}

	/* We are only interested in the debug registers. */

	parent_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	os_status = GetThreadContext( parent_thread_handle, &parent_context );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
					error_message,
					sizeof(error_message) );

		(void) ResumeThread( parent_thread_handle );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call GetThreadContext() for parent "
		"thread handle %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			parent_thread_handle,
			last_error_code,
			error_message );
	}

	for ( debug_reg_index = 0; debug_reg_index < 4; debug_reg_index++ ) {

		/* We only can use local breakpoints, since global breakpoints
		 * are global to the _entire_ _computer_ and not just the
		 * entire process.  However, we must test for both local and
		 * global breakpoints just in case Windows itself is using
		 * them.
		 */

		dr7_bits = 0x3 >> ( debug_reg_index * 2 );

		if ( ( parent_context.Dr7 & dr7_bits ) == 0 ) {

			/* This debug_reg_index is available. */
			break;
		}
	}

	if ( debug_reg_index >= 4 ) {
		(void) ResumeThread( parent_thread_handle );

		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Cannot set watchpoint %p since all hardware watchpoints "
		"are already in use.", watchpoint );
	}

	/* select the address to watch. */

	value_pointer = watchpoint->value_pointer;

#if ( defined(_MSC_VER) && (_MSC_VER >=1900 ) )
	/* For Visual Studio 2017 and above. */

	switch( debug_reg_index ) {
	case 0: parent_context.Dr0 = (DWORD64) value_pointer; break;
	case 1: parent_context.Dr1 = (DWORD64) value_pointer; break;
	case 2: parent_context.Dr2 = (DWORD64) value_pointer; break;
	case 3: parent_context.Dr3 = (DWORD64) value_pointer; break;
	}
#else
	/* Before Visual Studio 2017 */

#if ( MX_WORDSIZE == 64 )
	switch( debug_reg_index ) {
	case 0: parent_context.Dr0 = (DWORD64) value_pointer; break;
	case 1: parent_context.Dr1 = (DWORD64) value_pointer; break;
	case 2: parent_context.Dr2 = (DWORD64) value_pointer; break;
	case 3: parent_context.Dr3 = (DWORD64) value_pointer; break;
	}
#else
	switch( debug_reg_index ) {
	case 0: parent_context.Dr0 = (DWORD) value_pointer; break;
	case 1: parent_context.Dr1 = (DWORD) value_pointer; break;
	case 2: parent_context.Dr2 = (DWORD) value_pointer; break;
	case 3: parent_context.Dr3 = (DWORD) value_pointer; break;
	}
#endif

#endif

	/* Enable the matching local breakpoint. */

	parent_context.Dr7 |= 0x1 >> ( debug_reg_index * 2 );

	/* Select the condition for the breakpoint. */

	flags = watchpoint->flags;

	/* Note: We do not use debug_extension_flags == 0x2, which
	 * stands for 'break on I/O reads or writes'.  I am not sure
	 * what that one does.
	 */

	if ( flags == X_OK ) {
		/* break on instruction execution only */
		debug_extension_flags = 0x0;
	} else
	if ( flags == W_OK ) {
		/* break on data writes only */
		debug_extension_flags = 0x1;
	} else
	if ( flags & R_OK ) {
		/* break on data reads or writes */
		debug_extension_flags = 0x3;
	} else {
		(void) ResumeThread( parent_thread_handle );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal flags %#lx requested for watchpoint %p.",
			flags, watchpoint );
	}

	debug_extension_value =
		debug_extension_flags >> ( 16 + debug_reg_index * 2 );

	debug_extension_mask = 0x3 >> ( 16 + debug_reg_index * 2 );

	parent_context.Dr7 &= (~debug_extension_mask);

	parent_context.Dr7 |= debug_extension_value;

	/* Figure out the size of the memory location in bytes. */

	value_length = 
		mx_get_scalar_element_size( watchpoint->value_datatype, FALSE );

	switch( value_length ) {
	case 1:
		size_option = 0x0;
		break;
	case 2:
		size_option = 0x1;
		break;
	case 4:
		size_option = 0x3;
		break;
	case 8:
		/* This only checks the first 4 bytes of the value.
		 * Apparently x86 processors do not really handle
		 * the case of 8 bytes.
		 */
		size_option = 0x3;
		break;
	default:
		(void) ResumeThread( parent_thread_handle );

		return mx_error( MXE_UNSUPPORTED, fname,
		"MX scalar element size %ld is not supported.",
			value_length );
		break;
	}

	size_selector = size_option >> ( 18 + debug_reg_index * 2 );
	size_mask = 0x3 >> ( 18 + debug_reg_index * 2 );

	parent_context.Dr7 &= (~size_mask);

	parent_context.Dr7 |= size_selector;

	/* Now set the new parent thread context. */

	os_status = SetThreadContext( parent_thread_handle, &parent_context );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
					error_message,
					sizeof(error_message) );

		(void) ResumeThread( parent_thread_handle );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call SetThreadContext() for parent "
		"thread handle %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			parent_thread_handle,
			last_error_code,
			error_message );
	}

#if 1
	MX_DEBUG(-2,
	("%s: Dr0 = %#lx, Dr1 = %#lx, Dr2 = %#lx, Dr3 = %#lx, Dr7 = %#lx",
		fname, (unsigned long) parent_context.Dr0,
		(unsigned long) parent_context.Dr1,
		(unsigned long) parent_context.Dr2,
		(unsigned long) parent_context.Dr3,
		(unsigned long) parent_context.Dr7 ));
#endif

	/* Resume the parent thread since we are done modifying its context. */

	os_status = ResumeThread( parent_thread_handle );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
					error_message,
					sizeof(error_message) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call ResumeThread() for parent "
		"thread handle %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			parent_thread_handle,
			last_error_code,
			error_message );
	}

	/* We are done so terminate this thread by returning from
	 * this thread function.
	 */

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

/* WARNING: The following definition was copied from mx/libMx/mx_thread.c */

typedef struct {
	unsigned thread_id;
	HANDLE thread_handle;
	HANDLE stop_event_handle;
} MX_WIN32_THREAD_PRIVATE;

static mx_status_type
mxp_win32_show_watchpoint_spectator( MX_THREAD *thread, void *args )
{
	static const char fname[] = "mxp_win32_show_watchpoint_spectator()";

	HANDLE parent_thread_handle;
	CONTEXT parent_context;
	long last_error_code;
	TCHAR error_message[100];
	BOOL os_status;
	unsigned long dr7_register;

	parent_thread_handle = args;

	/* Suspend the parent thread so that we can display its context. */

	os_status = SuspendThread( parent_thread_handle );

	if ( os_status == (DWORD)(-1) ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
					error_message,
					sizeof(error_message) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call SuspendThread() for parent "
		"thread handle %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			parent_thread_handle,
			last_error_code,
			error_message );
	}

	/* We are only interested in the debug registers. */

	parent_context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	os_status = GetThreadContext( parent_thread_handle, &parent_context );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
					error_message,
					sizeof(error_message) );

		(void) ResumeThread( parent_thread_handle );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call GetThreadContext() for parent "
		"thread handle %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			parent_thread_handle,
			last_error_code,
			error_message );
	}

	dr7_register = parent_context.Dr7;

	/* Now show the debug registers that we are interested in. */

	mx_info( "\nProcess ID %lu hardware watchpoints:\n", 0 );

	mx_info( "     local  global  break    len       address" );

	mx_info( "DR0:   %1d      %1d      %#2x      %#2x       %p",
		dr7_register & 0x1,
		( dr7_register >> 1 ) & 0x1,
		( dr7_register >> 16 ) & 0x3,
		( dr7_register >> 18 ) & 0x3,
		parent_context.Dr0 );

	mx_info( "DR1:   %1d      %1d      %#2x      %#2x       %p",
		( dr7_register >> 2 ) & 0x1,
		( dr7_register >> 3 ) & 0x1,
		( dr7_register >> 20 ) & 0x3,
		( dr7_register >> 22 ) & 0x3,
		parent_context.Dr1 );

	mx_info( "DR2:   %1d      %1d      %#2x      %#2x       %p",
		( dr7_register >> 4 ) & 0x1,
		( dr7_register >> 5 ) & 0x1,
		( dr7_register >> 24 ) & 0x3,
		( dr7_register >> 26 ) & 0x3,
		parent_context.Dr2 );

	mx_info( "DR3:   %1d      %1d      %#2x      %#2x       %p",
		( dr7_register >> 6 ) & 0x1,
		( dr7_register >> 7 ) & 0x1,
		( dr7_register >> 28 ) & 0x3,
		( dr7_register >> 30 ) & 0x3,
		parent_context.Dr3 );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT int
mx_set_watchpoint( MX_WATCHPOINT **watchpoint_ptr,
		void *value_pointer,
		long value_datatype,
		unsigned long flags,
		void *callback_function( MX_WATCHPOINT *, void * ),
		void *callback_arguments )
{
	static const char fname[] = "mx_set_watchpoint()";

	MX_WATCHPOINT *watchpoint;
	MX_THREAD *spectator_thread = NULL;
	MX_WIN32_WATCHPOINT_PRIVATE *win32_private = NULL;
	double timeout_seconds;
	mx_status_type mx_status; 

	if ( watchpoint_ptr == (MX_WATCHPOINT **) NULL ) {
		return FALSE;
	}
	if ( value_pointer == NULL ) {
		return FALSE;
	}

	/* Create a watchpoint if one does not yet exist. */

	if ( *watchpoint_ptr == (MX_WATCHPOINT *) NULL ) {
		watchpoint = (MX_WATCHPOINT *)
				calloc( 1, sizeof(MX_WATCHPOINT) );

		if ( watchpoint == (MX_WATCHPOINT *) NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_WATCHPOINT structure." );

			return FALSE;
		}

		*watchpoint_ptr = watchpoint;
	} else {
		watchpoint = *watchpoint_ptr;
	}

	watchpoint->value_pointer = value_pointer;
	watchpoint->value_datatype = value_datatype;
	watchpoint->flags = flags;
	watchpoint->callback_function = callback_function;
	watchpoint->callback_arguments = callback_arguments;

	win32_private = (MX_WIN32_WATCHPOINT_PRIVATE *)
			calloc( 1, sizeof(MX_WIN32_WATCHPOINT_PRIVATE) );

	if ( win32_private == (MX_WIN32_WATCHPOINT_PRIVATE *) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"an MX_WIN32_WATCHPOINT_PRIVATE structure." );
		return FALSE;
	}

	watchpoint->watchpoint_private = win32_private;

	win32_private->data_thread_handle =
			mx_win32_get_current_thread_handle();

	if ( win32_private->data_thread_handle == NULL ) {
		return FALSE;
	}

	/* SetThreadContext() only works correctly if the thread it
	 * is applied to is suspended at the time.  In order to obey
	 * this restriction, we create a 'spectator thread' which will
	 * suspend the thread that mx_set_watchpoint() is running in
	 * and then resume that thread after its Win32 CONTEXT
	 * structure has been updated.
	 */

	mx_status = mx_thread_create( &spectator_thread,
					mxp_win32_set_watchpoint_spectator,
					watchpoint );

	if ( mx_status.code != MXE_SUCCESS )
		return FALSE;

	timeout_seconds = 30.0;

	mx_status = mx_thread_wait( spectator_thread, NULL, timeout_seconds );

	if ( mx_status.code != MXE_SUCCESS ) {
		return FALSE;
	}

	return TRUE;
}

MX_EXPORT int
mx_clear_watchpoint( MX_WATCHPOINT *watchpoint ) {
	int result;

	result = mx_set_watchpoint( &watchpoint, NULL, 0, 0, NULL, NULL );

	return result;
}

MX_EXPORT int
mx_show_watchpoints( void )
{
	static const char fname[] = "mx_show_watchpoints()";

	MX_THREAD *our_thread = NULL;
	MX_THREAD *spectator_thread = NULL;
	double timeout_seconds;
	mx_status_type mx_status;

	/* GetThreadContext() only works correctly if the thread it
	 * is applied to is suspended at the time.  In order to obey
	 * this restriction, we create a 'spectator thread' which will
	 * suspend the thread that mx_show_watchpoints() is running in
	 * and then resume that thread after we have displayed its
	 * watchpoints.
	 */

	mx_status = mx_thread_create( &spectator_thread,
					mxp_win32_show_watchpoint_spectator,
					mx_win32_get_current_thread_handle );

	if ( mx_status.code != MXE_SUCCESS )
		return FALSE;

	timeout_seconds = 30.0;

	mx_status = mx_thread_wait( spectator_thread, NULL, timeout_seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return FALSE;

	return TRUE;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_LINUX)

#  if ( defined(__i386__) || defined(__x86_64__) )

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>

/* The following implementation is inspired by
 *
 *   https://stackoverflow.com/questions/8941711/is-is-possible-to-set-a-gdb-watchpoint-programmatically
 *
 * and
 *
 *   https://stackoverflow.com/questions/40818920/how-to-set-the-value-of-dr7-register-in-order-to-create-a-hardware-breakpoint-on
 *
 */ 

typedef struct {
	unsigned int dr0_local:      1;
	unsigned int dr0_global:     1;
	unsigned int dr1_local:      1;
	unsigned int dr1_global:     1;
	unsigned int dr2_local:      1;
	unsigned int dr2_global:     1;
	unsigned int dr3_local:      1;
	unsigned int dr3_global:     1;
	unsigned int le:             1;
	unsigned int ge:             1;
	unsigned int reserved_10:    1;
	unsigned int rtm:            1;
	unsigned int reserved_12:    1;
	unsigned int gd:             1;
	unsigned int reserved_14_15: 2;
	unsigned int dr0_break:      2;
	unsigned int dr0_len:        2;
	unsigned int dr1_break:      2;
	unsigned int dr1_len:        2;
	unsigned int dr2_break:      2;
	unsigned int dr2_len:        2;
	unsigned int dr3_break:      2;
	unsigned int dr3_len:        2;
} dr7_bits_t;

typedef struct {
	uint32_t value;
	dr7_bits_t bits;
} dr7_t;

/*------*/

typedef struct {
	struct sigaction saved_sigtrap_action;
} MX_LINUX_WATCHPOINT_PRIVATE;

static void
mxp_default_watchpoint_handler( int signum, siginfo_t *info, void *ucontext )
{
	static const char fname[] = "mxp_default_watchpoint_handler()";

	if ( signum != SIGTRAP ) {
		fprintf( stderr, "ERROR: %s called for signal %d\n",
			fname, signum );
		return;
	}

	fprintf( stderr, "%s called. info = %p\n", fname, info );

	if ( info != NULL ) {
		fprintf( stderr,
			"%s: si_code = %d, si_addr = %p\n",
			fname, info->si_code, info->si_addr );

		fprintf( stderr,
		"\nIf present, try 'addr2line -f -e <binary or so name> %p\n",
			info->si_addr );
	}

	mx_stack_traceback();


	return;
}

MX_EXPORT int
mx_set_watchpoint( MX_WATCHPOINT **watchpoint_ptr,
		void *value_pointer,
		long value_datatype,
		unsigned long flags,
		void *callback_function( MX_WATCHPOINT *, void * ),
		void *callback_arguments )
{
	static const char fname[] = "mx_set_watchpoint()";

	MX_WATCHPOINT *watchpoint = NULL;
	MX_LINUX_WATCHPOINT_PRIVATE *linux_private = NULL;
	pid_t child_process;
	pid_t parent_process;
	pid_t waitpid_status;
	int waitpid_child_status;
	int waitpid_parent_status;
	int child_exit_status;
	struct sigaction sigtrap_action;
	unsigned long debug_extension_flags;
	unsigned long value_length, size_option;
	size_t register_offset;

	if ( watchpoint_ptr == (MX_WATCHPOINT **) NULL ) {
		return FALSE;
	}

	/* Select the condition for the breakpoint. */

	/* Note: We do not use debug_extension_flags == 0x2, which
	 * stands for 'break on I/O reads or writes'.  I am not sure
	 * what that one does.
	 */

	if ( flags == X_OK ) {
		/* break on instruction execution only */
		debug_extension_flags = 0x0;
	} else
	if ( flags == W_OK ) {
		/* break on data writes only */
		debug_extension_flags = 0x1;
	} else
	if ( flags & R_OK ) {
		/* break on data reads or writes */
		debug_extension_flags = 0x3;
	} else {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal watchpoint flags %#lx requested.", flags );
		return FALSE;
	}

	/* Figure out the size of the memory location in bytes. */

	value_length = mx_get_scalar_element_size( value_datatype, FALSE );

	switch( value_length ) {
	case 1:
		size_option = 0x0;
		break;
	case 2:
		size_option = 0x1;
		break;
	case 4:
		size_option = 0x3;
		break;
	case 8:
		/* This only checks the first 4 bytes of the value.
		 * Apparently x86 processors do not really handle
		 * the case of 8 bytes.
		 */
		size_option = 0x3;
		break;
	default:
		(void) mx_error( MXE_UNSUPPORTED, fname,
		"MX scalar element size %ld is not supported.",
			value_length );

		return FALSE;
		break;
	}

	/*---*/

	/* FIXME: If value_pointer is NULL, then this should be a request
	 * to clear the watchpoint.
	 */

	MX_DEBUG(-2,("%s: value_pointer == %p", fname, value_pointer));

	/* Create a watchpoint if one does not yet exist. */

	if ( *watchpoint_ptr == (MX_WATCHPOINT *) NULL ) {
		watchpoint = (MX_WATCHPOINT *)
				calloc( 1, sizeof(MX_WATCHPOINT) );

		if ( watchpoint == (MX_WATCHPOINT *) NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_WATCHPOINT structure." );

			return FALSE;
		}

		*watchpoint_ptr = watchpoint;
	} else {
		watchpoint = *watchpoint_ptr;
	}

	watchpoint->value_pointer = value_pointer;
	watchpoint->value_datatype = value_datatype;
	watchpoint->flags = flags;
	watchpoint->callback_function = callback_function;
	watchpoint->callback_arguments = callback_arguments;

	linux_private = (MX_LINUX_WATCHPOINT_PRIVATE *)
			calloc( 1, sizeof(MX_LINUX_WATCHPOINT_PRIVATE) );

	if ( linux_private == (MX_LINUX_WATCHPOINT_PRIVATE *) NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"an MX_LINUX_WATCHPOINT_PRIVATE structure." );

		return FALSE;
	}

	watchpoint->watchpoint_private = linux_private;

	/* Set up our custom SIGTRAP handler. */

	sigaction( SIGTRAP, NULL, &sigtrap_action );

	linux_private->saved_sigtrap_action = sigtrap_action;

	sigtrap_action.sa_sigaction = mxp_default_watchpoint_handler;
	sigtrap_action.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER;

	sigaction( SIGTRAP, &sigtrap_action, NULL );

	/* Now fork off a child process that runs ptrace() on the parent. */

	parent_process = getpid();

	child_process = fork();

	if ( child_process == 0 ) {
		/* We are in the child. */

		int ptrace_status;
		int return_value = EXIT_SUCCESS;

		dr7_t old_dr7;
		dr7_t new_dr7;

		memset( &old_dr7, 0, sizeof(old_dr7) );
		memset( &new_dr7, 0, sizeof(new_dr7) );

		child_process = getpid();

		/* Attach to the parent process. */

		mx_warning(
		"%s: Watchpoint child (%d) is stopping the parent (%d).",
				fname, child_process, parent_process );

		ptrace_status = ptrace( PTRACE_ATTACH, parent_process,
						NULL, NULL );

		if ( ptrace_status != 0 )
			exit( EXIT_FAILURE );

		/* Wait for the parent to stop. */

		waitpid_status = waitpid( parent_process,
				&waitpid_parent_status, 0 );

		if ( waitpid_status < 0 ) {
			fprintf( stderr,
			"Error in child waiting for parent to stop.\n" );
			return TRUE;
		}

		if ( WIFSTOPPED( waitpid_parent_status ) == 0 ) {
			fprintf( stderr,
			"waitpid( parent_process, ... ) returned but "
			"the parent process is not stopped!!!\n" );
			return TRUE;
		}

		/* Get the current contents of the D7 register. */

		errno = 0;

		old_dr7.value = ptrace( PTRACE_PEEKUSER, parent_process,
					offsetof( struct user, u_debugreg[7] ),
					NULL );

		if ( errno != 0 ) {
			return_value = EXIT_FAILURE;
		}

		memcpy( &new_dr7, &old_dr7, sizeof(new_dr7) );

		/* Find an unused watchpoint. */

		if ( (old_dr7.bits.dr0_local == 0)
		  && (old_dr7.bits.dr0_global == 0) )
		{
			new_dr7.bits.dr0_local = 1;
			new_dr7.bits.dr0_break = debug_extension_flags;
			new_dr7.bits.dr0_len = value_length;
			register_offset = offsetof(struct user, u_debugreg[0]);
		} else
		if ( (old_dr7.bits.dr1_local == 0)
		  && (old_dr7.bits.dr1_global == 0) )
		{
			new_dr7.bits.dr1_local = 1;
			new_dr7.bits.dr1_break = debug_extension_flags;
			new_dr7.bits.dr1_len = value_length;
			register_offset = offsetof(struct user, u_debugreg[1]);
		} else
		if ( (old_dr7.bits.dr2_local == 0)
		  && (old_dr7.bits.dr2_global == 0) )
		{
			new_dr7.bits.dr2_local = 1;
			new_dr7.bits.dr2_break = debug_extension_flags;
			new_dr7.bits.dr2_len = value_length;
			register_offset = offsetof(struct user, u_debugreg[2]);
		} else
		if ( (old_dr7.bits.dr3_local == 0)
		  && (old_dr7.bits.dr3_global == 0) )
		{
			new_dr7.bits.dr3_local = 1;
			new_dr7.bits.dr3_break = debug_extension_flags;
			new_dr7.bits.dr3_len = value_length;
			register_offset = offsetof(struct user, u_debugreg[3]);
		} else {
			(void) mx_error( MXE_NOT_AVAILABLE, fname,
			"All debug registers are already in use." );

			return_value = EXIT_FAILURE;
		}

		/* Set the watchpoint address. */

		if ( return_value == EXIT_SUCCESS ) {
			ptrace_status = ptrace( PTRACE_POKEUSER, parent_process,
						register_offset, value_pointer);

			if ( ptrace_status != 0 ) {
				return_value = EXIT_FAILURE;
			}
		}

		/* Enable the watchpoint. */

		if ( return_value == EXIT_SUCCESS ) {
			ptrace_status = ptrace( PTRACE_POKEUSER, parent_process,
					offsetof( struct user, u_debugreg[7] ),
					new_dr7 );

			if ( ptrace_status != 0 ) {
				return_value = EXIT_FAILURE;
			}
		}

		ptrace_status = ptrace( PTRACE_DETACH, parent_process,
						NULL, NULL );

		if ( ptrace_status != 0 ) {
			return_value = EXIT_FAILURE;
		}

		exit( return_value );
	}

	/* We are in the parent process and must wait for the child to finish.*/

	waitpid_status = waitpid( child_process, &waitpid_child_status, 0 );

	if ( waitpid_status < 0 ) {
		fprintf( stderr, "Error in %s: waitpid() returned with %d\n",
			fname, waitpid_status );
		return TRUE;
	}

	if ( WIFEXITED( waitpid_child_status ) == 0 ) {
		fprintf( stderr,
		"Error in %s: child process %d returned abnormally.\n",
			fname, child_process );
		return TRUE;
	}

	child_exit_status = WEXITSTATUS( waitpid_child_status );

	fprintf( stderr, "%s child process %d returned with status = %d\n",
		fname, child_process, child_exit_status );

	return FALSE;
}

MX_EXPORT int
mx_clear_watchpoint( MX_WATCHPOINT *watchpoint )
{
	static const char fname[] = "mx_clear_watchpoint()";

	int result;

	MX_DEBUG(-2,("%s is not yet implemented.", fname));

	result = FALSE;

	return result;
}

MX_EXPORT int
mx_show_watchpoints( void )
{
	static const char fname[] = "mx_show_watchpoints()";

	pid_t child_process;
	pid_t parent_process;
	pid_t waitpid_status;
	int waitpid_child_status;
	int waitpid_parent_status;
	int child_exit_status;
	int i;
	void *value_pointer[4];

	/* This process itself cannot directly look at its own hardware debug
	 * registers using ptrace(), so we create a child process to do the job.
	 */

	parent_process = getpid();

	child_process = fork();

	if ( child_process == 0 ) {
		/* We are in the child. */

		int ptrace_status;
		int return_value = EXIT_SUCCESS;
		dr7_t dr7;

		memset( &dr7, 0, sizeof(dr7) );

		child_process = getpid();

		/* Attach to the parent process. */

		mx_warning(
		"%s: Watchpoint child (%d) is stopping the parent (%d).",
				fname, child_process, parent_process );

		ptrace_status = ptrace( PTRACE_ATTACH, parent_process,
						NULL, NULL );

		if ( ptrace_status != 0 ) 
			exit( EXIT_FAILURE );

		/* Wait for the parent to stop. */

		waitpid_status = waitpid( parent_process,
				&waitpid_parent_status, 0 );

		if ( waitpid_status < 0 ) {
			fprintf( stderr,
			"Error in child waiting for parent to stop.\n" );
			return TRUE;
		}

		if ( WIFSTOPPED( waitpid_parent_status ) == 0 ) {
			fprintf( stderr,
			"waitpid( parent_process, ... ) returned but "
			"the parent process is not stopped!!!\n" );
			return TRUE;
		}

		/* Get the current contents of the D7 register. */

		errno = 0;

		dr7.value = ptrace( PTRACE_PEEKUSER, parent_process,
					offsetof( struct user, u_debugreg[7] ),
					NULL );

		if ( errno != 0 ) {
			return_value = EXIT_FAILURE;
		}

		/* Get the four hardware addresses. */

		for ( i = 0; i < 4; i++ ) {
			errno = 0;

			value_pointer[i] = (void *) ptrace(
					PTRACE_PEEKUSER, parent_process,
					offsetof( struct user, u_debugreg[i] ),
					NULL );

			if ( errno != 0 ) {
				return_value = EXIT_FAILURE;
			}
		}

		/* Now show the results. */

		mx_info( "\nProcess ID %lu hardware watchpoints:\n",
			(unsigned long) parent_process );
		mx_info( "     local  global  break    len       address" );

		mx_info( "DR0:   %1d      %1d      %#2x      %#2x       %p",
			dr7.bits.dr0_local,
			dr7.bits.dr0_global,
			dr7.bits.dr0_break,
			dr7.bits.dr0_len,
			value_pointer[0] );

		mx_info( "DR1:   %1d      %1d      %#2x      %#2x       %p",
			dr7.bits.dr1_local,
			dr7.bits.dr1_global,
			dr7.bits.dr1_break,
			dr7.bits.dr1_len,
			value_pointer[1] );

		mx_info( "DR2:   %1d      %1d      %#2x      %#2x       %p",
			dr7.bits.dr2_local,
			dr7.bits.dr2_global,
			dr7.bits.dr2_break,
			dr7.bits.dr2_len,
			value_pointer[2] );

		mx_info( "DR3:   %1d      %1d      %#2x      %#2x       %p",
			dr7.bits.dr3_local,
			dr7.bits.dr3_global,
			dr7.bits.dr3_break,
			dr7.bits.dr3_len,
			value_pointer[3] );

		mx_warning(
		"%s: Watchpoint child (%d) is resuming the parent (%d).",
				fname, child_process, parent_process );

		ptrace_status = ptrace( PTRACE_DETACH, parent_process,
						NULL, NULL );

		if ( ptrace_status != 0 ) {
			return_value = EXIT_FAILURE;
		}

		exit( return_value );
	}

	/* We are in the parent process and must wait for the child to finish.*/

	waitpid_status = waitpid( child_process, &waitpid_child_status, 0 );

	if ( waitpid_status < 0 ) {
		fprintf( stderr, "Error in %s: waitpid() returned with %d\n",
			fname, waitpid_status );
		return TRUE;
	}

	if ( WIFEXITED( waitpid_child_status ) == 0 ) {
		fprintf( stderr,
		"Error in %s: child process %d returned abnormally.\n",
			fname, child_process );
		return TRUE;
	}

	child_exit_status = WEXITSTATUS( waitpid_child_status );

	if ( child_exit_status == EXIT_FAILURE ) {
		fprintf( stderr,
			"%s child process %d returned a failure status.\n",
			fname, child_process );
	}

	return FALSE;
}

/*---------------------*/

#  elif 1

MX_EXPORT int
mx_set_watchpoint( MX_WATCHPOINT **watchpoint_ptr,
		void *value_pointer,
		long value_datatype,
		unsigned long flags,
		void *callback_function( MX_WATCHPOINT *, void * ),
		void *callback_arguments )
{
	static const char fname[] = "mx_set_watchpoint()";

	mx_error( MXE_UNSUPPORTED, fname,
		"MX watchpoints are not supported for this Linux target." );

	return FALSE;
}

MX_EXPORT int
mx_clear_watchpoint( MX_WATCHPOINT *watchpoint ) {
	return TRUE;
}

MX_EXPORT int
mx_show_watchpoints( void )
{
	static const char fname[] = "mx_show_watchpoints()";

	mx_error( MXE_UNSUPPORTED, fname,
		"MX watchpoints are not supported for this Linux target." );

	return FALSE;
}

#  else 
#  error watchpoints are not defined for this Linux target.
#  endif

#elif defined(OS_MACOSX) || defined(OS_ANDROID) \
	|| defined(OS_CYGWIN) || defined(OS_SOLARIS) || defined(OS_BSD) \
	|| defined(OS_MINIX) || defined(OS_RTEMS) || defined(OS_VXWORKS) \
	|| defined(OS_HURD) || defined(OS_QNX)

/* FIXME: Implement real watchpoints for these build targets. */

MX_EXPORT int
mx_set_watchpoint( MX_WATCHPOINT **watchpoint_ptr,
		void *value_pointer,
		long value_datatype,
		unsigned long flags,
		void *callback_function( MX_WATCHPOINT *, void * ),
		void *callback_arguments )
{
	static const char fname[] = "mx_set_watchpoint()";

	mx_error( MXE_UNSUPPORTED, fname,
		"MX watchpoints are not supported for this build target." );

	return FALSE;
}

MX_EXPORT int
mx_clear_watchpoint( MX_WATCHPOINT *watchpoint ) {
	return TRUE;
}

MX_EXPORT int
mx_show_watchpoints( void )
{
	static const char fname[] = "mx_show_watchpoints()";

	mx_error( MXE_UNSUPPORTED, fname,
		"MX watchpoints are not supported for this build target." );

	return FALSE;
}

#else
#error watchpoints not defined for this build target.
#endif

/*-------------------------------------------------------------------------*/

