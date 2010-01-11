/*
 * Name:    mx_coprocess.c
 *
 * Purpose: Create a coprocess connected to the current process by a pair
 *          of pipes.  One pipe is used to send to the coprocess and the
 *          other pipe is used to receive from the coprocess.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003-2004, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_COPROCESS		FALSE

#include <stdio.h>

#include "mx_osdef.h"

#ifdef OS_UNIX

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_clock.h"
#include "mx_coprocess.h"

MX_EXPORT mx_status_type
mx_coprocess_open( MX_COPROCESS **coprocess, char *command_line )
{
	static const char fname[] = "mx_coprocess_open()";

	pid_t fork_pid;
	int fd1[2], fd2[2];
	int result, os_status, saved_errno;
	int i, argc, envc;
	char **argv, **envp;

	if ( coprocess == (MX_COPROCESS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null coprocess pointer passed." );
	}
	if ( command_line == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null command_line pointer passed." );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s invoked for command '%s'", fname, command_line));
#endif

	/* Allocate an MX_COPROCESS structure for the caller. */

	(*coprocess) = malloc( sizeof(MX_COPROCESS) );

	if ( (*coprocess) == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Ran out of memory trying to allocate an MX_COPROCESS structure." );
	}

	(*coprocess)->from_coprocess = NULL;
	(*coprocess)->to_coprocess = NULL;
	(*coprocess)->coprocess_pid = 0;

	/* Create a pair of pipes. */

	result = pipe( fd1 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
"Attempt to create pipe #1 for child failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	result = pipe( fd2 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
"Attempt to create pipe #2 for child failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	/* Create the child process. */

	fork_pid = fork();

	if ( fork_pid == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Attempt to fork() failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	if ( fork_pid == 0 ) {
		/********** Child process. **********/

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: This is child process %lu.",
			fname, mx_process_id()));
#endif

		/* Try to redirect standard input for the external command. */

		close( fd1[1] );

		if ( fd1[0] != STDIN_FILENO ) {
			result = dup2( fd1[0], STDIN_FILENO );

			if ( result != STDIN_FILENO ) {
				saved_errno = errno;

				fprintf( stderr,
	"Attempt to duplicate standard input file descriptor failed.  "
	"Errno = %d, error message = '%s'",
					saved_errno, strerror( saved_errno ));

				exit(1);
			}
		}

		/* Try to redirect standard output for the external command. */

		close( fd2[0] );

		if ( fd2[1] != STDOUT_FILENO ) {
			result = dup2( fd2[1], STDOUT_FILENO );

			if ( result != STDOUT_FILENO ) {
				saved_errno = errno;

				fprintf( stderr,
	"Attempt to duplicate standard output file descriptor failed.  "
	"Errno = %d, error message = '%s'",
					saved_errno, strerror( saved_errno ));

				exit(1);
			}
		}

		/* Parse the command line. */

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: command_line = '%s'",
			fname, command_line));
#endif

		result = mx_parse_command_line( command_line,
					&argc, &argv, &envc, &envp );

		if ( result != 0 ) {
			saved_errno = errno;

			fprintf( stderr,
			"Attempt to parse the command line '%s' failed.  "
			"Errno = %d, error message = '%s'",
			    command_line, saved_errno, strerror( saved_errno ));

			exit(1);
		}

#if DEBUG_COPROCESS
		for ( i = 0; i < argc; i++ ) {
			MX_DEBUG(-2,("%s: argv[%d] = '%s'",
				fname, i, argv[i]));
		}
		MX_DEBUG(-2,("%s: argv[%d] = '%s'",
				fname, i, argv[i]));
#endif

		/* Add the environment variables found to the environment. */

		for ( i = 0; i < envc; i++ ) {

#if DEBUG_COPROCESS
			MX_DEBUG(-2,("%s: envp[%d] = '%s'",
				fname, i, envp[i]));
#endif

			result = putenv( envp[i] );

			if ( result != 0 ) {
				saved_errno = errno;

				fprintf( stderr,
"Attempt to add the enviroment variable '%s' to the environment failed.  "
"Errno = %d, error message = '%s'",
				envp[i], saved_errno, strerror( saved_errno ));

				exit(1);
			}
		}

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: envp[%d] = '%s'", fname, i, envp[i]));
#endif
		/* Make ourselves a process group leader.
		 *
		 * By doing this, we make it easy for later calls of
		 * the mx_coprocess_close() function to kill all of
		 * the children created by the coprocess, by sending
		 * a SIGKILL to the process group.
		 */

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: calling setpgid(0,0).", fname));
#endif

		os_status = setpgid( 0, 0 );

		if ( os_status != 0 ) {
			saved_errno = errno;

			fprintf( stderr,
"The attempt by child process %lu to become a process group leader failed.  "
"Errno = %d, error message = '%s'", mx_process_id(),
				saved_errno, strerror( saved_errno ) );

			exit(1);
		}

		/* Now execute the external command. */

#if DEBUG_COPROCESS
		MX_DEBUG(-2,
		("%s: About to execute the external command.", fname));
#endif

		(void) execvp( argv[0], argv );

		/* If execvp() succeeds, then we should not be able
		 * to get here, since this process image will have
		 * been replaced by the new process image.
		 */

		saved_errno = errno;

		fprintf( stderr,
	"Can't execute command '%s'.  Error text = '%s'\n",
			argv[0], strerror( saved_errno ) );

		/* The only safe thing we can do now is exit. */

		exit(1);
	} else {
		/********** Parent process. **********/

		close( fd1[0] );
		close( fd2[1] );

		(*coprocess)->coprocess_pid = fork_pid;

		/* Connect the file handles to FILE pointers and
		 * change stdio buffering to line buffered.
		 */

		(*coprocess)->from_coprocess = fdopen( fd2[0], "r" );

		if ( (*coprocess)->from_coprocess == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
"Attempt to invoke fdopen() on 'from_coprocess' file handle failed.  "
"Errno = %d, error message = '%s'",  saved_errno, strerror( saved_errno ) );
		}

		(*coprocess)->to_coprocess = fdopen( fd1[1], "w" );

		if ( (*coprocess)->to_coprocess == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
"Attempt to invoke fdopen() on 'to_coprocess' file handle failed.  "
"Errno = %d, error message = '%s'",  saved_errno, strerror( saved_errno ) );
		}

		/* Nothing else to do at this point. */
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_coprocess_close( MX_COPROCESS *coprocess, double timeout_in_seconds )
{
	static const char fname[] = "mx_coprocess_close()";

	MX_CLOCK_TICK current_tick, finish_tick, timeout_in_ticks;
	pid_t coprocess_pid;
	int kill_status, wait_status, os_status, saved_errno, comparison;
	mx_bool_type timed_out;

	if ( coprocess == (MX_COPROCESS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null coprocess pointer passed." );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s invoked for PID %lu with a timeout of %g seconds.",
		fname, (unsigned long) coprocess->coprocess_pid,
		timeout_in_seconds));
#endif

	if ( coprocess->from_coprocess != NULL ) {
		fclose( coprocess->from_coprocess );
	}
	coprocess->from_coprocess = NULL;

	if ( coprocess->to_coprocess != NULL ) {
		fclose( coprocess->to_coprocess );
	}
	coprocess->to_coprocess = NULL;

	coprocess_pid = coprocess->coprocess_pid;

	mx_free( coprocess );

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: coprocess FILE pointers are now closed.", fname));
#endif

	/* If the timeout is negative, then we wait forever for the
	 * child to exit.
	 */

	if ( timeout_in_seconds < 0.0 ) {
#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: waiting forever for PID %lu",
			fname, (unsigned long) coprocess_pid));
#endif

		(void) waitpid( coprocess_pid, NULL, 0 );

		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we loop waiting for the child to exit until the
	 * timeout expires.
	 *
	 * Note: In principle, on some platforms we could use a Posix
	 * real time signal handler and wait in waitpid() for the timeout
	 * signal to arrive.  However, not all Unix-based MX build
	 * platforms support Posix real time signals.  In addition, we
	 * cannot depend on using SIGALRM, since third party software
	 * we are linked to may be using SIGALRM for its own purposes.
	 * Thus, the only safe thing to do is to loop.
	 */

	timeout_in_ticks =
		mx_convert_seconds_to_clock_ticks( timeout_in_seconds );

	current_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: finish_tick = (%lu,%lu)", fname,
		finish_tick.high_order, finish_tick.low_order));
#endif

	timed_out = TRUE;

	while (1) {

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: kill( %lu, 0 )",
			fname, (unsigned long) coprocess_pid));
#endif
		/* Test to see if the process is still alive. */

		kill_status = kill( coprocess_pid, 0 );

		saved_errno = errno;

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: kill_status = %d", fname, kill_status));
#endif
		if ( kill_status != 0 ) {

#if DEBUG_COPROCESS
			MX_DEBUG(-2,("%s: errno = %d, error message = '%s'",
				fname, saved_errno, strerror(saved_errno) ));
#endif
			if ( saved_errno == ESRCH ) {
				/* The child process has exited, so break
				 * out of the timeout loop.
				 */

				timed_out = FALSE;
				break;
			}
		}

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: waitpid( %lu, ..., WNOHANG )",
			fname, (unsigned long) coprocess_pid));
#endif
		/* Wait for any exited processes using waitpid().  If the
		 * process has exited, then this will cause the next call
		 * to kill(...,0) to return -1 with errno set to ESRCH.
		 */

		(void) waitpid( coprocess_pid, &wait_status, WNOHANG );

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: wait_status = %#x", fname, wait_status));
#endif
		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks(current_tick, finish_tick);

#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: comparison = %d, current_tick = (%lu,%lu)",
			fname, comparison, current_tick.high_order,
			current_tick.low_order));
#endif
		if ( comparison >= 0 )
			break;

		mx_msleep( 100 );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: timed_out = %d", fname, timed_out));
#endif

	/* If the process has exited by now, then we are done. */

	if ( timed_out == FALSE ) {
#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: process %lu has exited.",
			fname, (unsigned long) coprocess_pid));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the coprocess has _not_ exited by now, then we kill it
	 * and all of the members of its process group.
	 */

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: we must kill process group %lu.",
		fname, (unsigned long) coprocess_pid));
#endif

	os_status = kill ( - coprocess_pid, SIGKILL );

	if ( os_status == (-1) ) {
		saved_errno = errno;

		if ( saved_errno == ESRCH ) {
			return MX_SUCCESSFUL_RESULT;
		}

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Sending SIGKILL to process group %ld caused an error.  "
		"Errno = %d, error message = '%s'",
			(long) coprocess_pid, errno, strerror( errno ) );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: waiting for killed PID %lu.",
			fname, (unsigned long) coprocess_pid));
#endif

	/* If the coprocess dies before the coprocess's children die,
	 * then those children become children of the 'init' process
	 * rather than us, so the only process we can wait for here
	 * is the coprocess itself.
	 */

	(void) waitpid( coprocess_pid, NULL, 0 );

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: killed PID %lu has exited.",
			fname, (unsigned long) coprocess_pid));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_UNIX */
