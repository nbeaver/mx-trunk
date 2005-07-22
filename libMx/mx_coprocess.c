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
 * Copyright 1999-2001, 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

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
#include "mx_coprocess.h"

MX_EXPORT mx_status_type
mx_create_coprocess( MX_COPROCESS *coprocess, char *command_line )
{
	const char fname[] = "mx_create_coprocess()";

	pid_t fork_pid;
	int fd1[2], fd2[2];
	int result, saved_errno;
	int i, argc, envc;
	char **argv, **envp;

	if ( coprocess == (MX_COPROCESS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null coprocess pointer passed." );
	}
	if ( command_line == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null command_line pointer passed." );
	}

	coprocess->from_coprocess = NULL;
	coprocess->to_coprocess = NULL;
	coprocess->coprocess_pid = 0;

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

		MX_DEBUG(-2,("%s: This is child process %lu.",
			fname, mx_process_id()));

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

		MX_DEBUG(-2,("%s: command_line = '%s'",
			fname, command_line));

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

#if 1
		for ( i = 0; i < argc; i++ ) {
			MX_DEBUG(-2,("%s: argv[%d] = '%s'",
				fname, i, argv[i]));
		}
		MX_DEBUG(-2,("%s: argv[%d] = '%s'",
				fname, i, argv[i]));
#endif

		/* Add the environment variables found to the environment. */

		for ( i = 0; i < envc; i++ ) {
			MX_DEBUG(-2,("%s: envp[%d] = '%s'",
				fname, i, envp[i]));

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
		MX_DEBUG(-2,("%s: envp[%d] = '%s'", fname, i, envp[i]));

		/* Now execute the external command. */

		MX_DEBUG(-2,
		("%s: About to execute the external command.", fname));

		(void) execvp( argv[0], argv );

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

		coprocess->coprocess_pid = fork_pid;

		/* Connect the file handles to FILE pointers and
		 * change stdio buffering to line buffered.
		 */

		coprocess->from_coprocess = fdopen( fd2[0], "r" );

		if ( coprocess->from_coprocess == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
"Attempt to invoke fdopen() on 'from_coprocess' file handle failed.  "
"Errno = %d, error message = '%s'",  saved_errno, strerror( saved_errno ) );
		}

		coprocess->to_coprocess = fdopen( fd1[1], "w" );

		if ( coprocess->to_coprocess == NULL ) {
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
mx_kill_coprocess( MX_COPROCESS *coprocess )
{
	const char fname[] = "mx_kill_coprocess()";

	int status, saved_errno;

	if ( coprocess == (MX_COPROCESS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null coprocess pointer passed." );
	}

	if ( coprocess->from_coprocess != NULL ) {
		fclose( coprocess->from_coprocess );
	}
	coprocess->from_coprocess = NULL;

	if ( coprocess->to_coprocess != NULL ) {
		fclose( coprocess->to_coprocess );
	}
	coprocess->to_coprocess = NULL;

	/* Send the coprocess a SIGKILL. */

	if ( coprocess->coprocess_pid > 0 ) {
		status = kill( coprocess->coprocess_pid, SIGKILL );

		if ( status == (-1) ) {
			saved_errno = errno;

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"Sending SIGKILL to process %ld caused an error.  "
			"Errno = %d, error message = '%s'",
				(long) coprocess->coprocess_pid,
				errno, strerror( errno ) );
		}
		MX_DEBUG( 2,("%s: about to wait for process %ld.",
				fname, (long) coprocess->coprocess_pid));

		/* Wait for the child to exit. */

		(void) waitpid( coprocess->coprocess_pid, NULL, 0 );
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_UNIX */
