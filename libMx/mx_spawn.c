/*
 * Name:    mx_spawn.c
 *
 * Purpose: MX functions for executing external commands.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SPAWN_DEBUG		FALSE

#include <stdio.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_unistd.h"

#if defined(OS_UNIX)

#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

MX_EXPORT mx_status_type
mx_spawn( char *command_line, unsigned long flags )
{
	static const char fname[] = "mx_spawn()";

	pid_t fork_pid, parent_pid, child_pid;
	int result, saved_errno, os_status;
	int i, argc, envc;
	char **argv, **envp;

	if ( command_line == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command_line pointer passed was NULL." );
	}

#if MX_SPAWN_DEBUG
	MX_DEBUG(-2,("%s: command_line = '%s'", fname, command_line));
#endif

	parent_pid = mx_process_id();

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

		child_pid = mx_process_id();

#if MX_SPAWN_DEBUG
		MX_DEBUG(-2,("%s: This is child process %lu.",
			fname, (unsigned long) child_pid ));
#endif

		/* If requested, suspend the parent. */

		if ( flags & MXF_SPAWN_SUSPEND_PARENT ) {

#if MX_SPAWN_DEBUG
			MX_DEBUG(-2,("%s: Child %lu suspending parent %lu",
				fname, (unsigned long) child_pid,
				(unsigned long) parent_pid ));
#endif

			os_status = kill( parent_pid, SIGSTOP );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt by child process %lu to send a SIGSTOP signal to "
		"parent process %lu failed.  Errno = %d, error message = '%s'.",
					(unsigned long) child_pid,
					(unsigned long) parent_pid,
					saved_errno, strerror( saved_errno ) );
			}
		}

		/* If requested, suspend the child itself. */

		if ( flags & MXF_SPAWN_SUSPEND_CHILD ) {

#if MX_SPAWN_DEBUG
			MX_DEBUG(-2,("%s: Child %lu suspending itself.",
				fname, (unsigned long) child_pid));
#endif

			os_status = raise( SIGSTOP );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt by child process %lu to send a SIGSTOP signal to "
		"itself failed.  Errno = %d, error message = '%s'.",
					(unsigned long) child_pid,
					saved_errno, strerror( saved_errno ) );
			}
		}

		/* Parse the command line. */

#if MX_SPAWN_DEBUG
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

#if MX_SPAWN_DEBUG
		if ( argv != NULL ) {
			for ( i = 0; i < argc; i++ ) {
				MX_DEBUG(-2,("%s: argv[%d] = '%s'",
					fname, i, argv[i]));
			}
			MX_DEBUG(-2,("%s: argv[%d] = '%s'",
					fname, i, argv[i]));
		}
#endif

		/* Add the environment variables found to the environment. */

		if ( envp != NULL ) {
			for ( i = 0; i < envc; i++ ) {
#if MX_SPAWN_DEBUG
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
#if MX_SPAWN_DEBUG
			MX_DEBUG(-2,("%s: envp[%d] = '%s'", fname, i, envp[i]));
#endif
		}

		/* Now execute the external command. */

#if MX_SPAWN_DEBUG
		MX_DEBUG(-2,
		("%s: About to execute the external command.", fname));
#endif

		(void) execvp( argv[0], argv );

		saved_errno = errno;

		fprintf( stderr,
	"Can't execute command '%s'.  Errno = %d, error message = '%s'\n",
			argv[0], saved_errno, strerror( saved_errno ) );

		/* The only safe thing we can do now is exit. */

		exit(1);
	} else {
		/********** Parent process. **********/

#if MX_SPAWN_DEBUG
		MX_DEBUG(-2,("%s: This is parent process %lu.",
			fname, (unsigned long) parent_pid));
#endif

		child_pid = fork_pid;

		/* If requested, suspend the child. */

		if ( flags & MXF_SPAWN_SUSPEND_CHILD ) {

#if MX_SPAWN_DEBUG
			MX_DEBUG(-2,("%s: Parent %lu suspending child %lu",
				fname, (unsigned long) parent_pid,
				(unsigned long) child_pid ));
#endif

			os_status = kill( child_pid, SIGSTOP );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt by parent process %lu to send a SIGSTOP signal to "
		"child process %lu failed.  Errno = %d, error message = '%s'.",
					(unsigned long) parent_pid,
					(unsigned long) child_pid,
					saved_errno, strerror( saved_errno ) );
			}
		}

		/* If requested, suspend the parent itself. */

		if ( flags & MXF_SPAWN_SUSPEND_PARENT ) {

#if MX_SPAWN_DEBUG
			MX_DEBUG(-2,("%s: Parent %lu suspending itself.",
				fname, (unsigned long) parent_pid));
#endif

			os_status = raise( SIGSTOP );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt by parent process %lu to send a SIGSTOP signal to "
		"itself failed.  Errno = %d, error message = '%s'.",
					(unsigned long) parent_pid,
					saved_errno, strerror( saved_errno ) );
			}
		}

		/* Nothing else to do at this point. */
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

MX_EXPORT mx_status_type
mx_spawn( char *command_line, unsigned long flags )
{
	static const char fname[] = "mx_spawn()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"%s has not yet been implemented for this platform.", fname );
}

#endif

/*------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_WIN32)

MX_EXPORT int
mx_command_found( char *command_name )
{
	static const char fname[] = "mx_command_found()";

	char pathname[MXU_FILENAME_LENGTH+1];
	char *path, *start_ptr, *end_ptr;
	int os_status;
	size_t length;
	mx_bool_type try_pathname;

#if defined(OS_WIN32)
	char path_separator = ';';
#else
	char path_separator = ':';
#endif

	if ( command_name == NULL )
		return FALSE;

	/* See first if the file can be treated as an absolute
	 * or relative pathname.
	 */

	try_pathname = FALSE;

	if ( strchr( command_name, '/' ) != NULL ) {
		try_pathname = TRUE;
	}

#if defined(OS_WIN32)
	if ( strchr( command_name, '\\' ) != NULL ) {
		try_pathname = TRUE;
	}
#endif

	/* If the supplied command name appears to be a relative or absolute
	 * pathname, try using access() to see if the file exists and is
	 * executable.
	 */

	if ( try_pathname ) {
		os_status = access( command_name, X_OK );

		if ( os_status == 0 ) {
			return TRUE;
		} else {
			return FALSE;
		}
	}

	/* If we get here, look for the command in the directories listed
	 * by the PATH variable.
	 */

	path = getenv( "PATH" );

	MX_DEBUG( 2,("%s: path = '%s'", fname, path));

	/* Loop through the path components. */

	start_ptr = path;

	for(;;) {
		/* Look for the end of the next path component. */

		end_ptr = strchr( start_ptr, path_separator );

		if ( end_ptr == NULL ) {
			length = strlen( start_ptr );
		} else {
			length = end_ptr - start_ptr;
		}

		/* If the next path component is longer than the
		 * maximum filename length, skip it.
		 */

		if ( length > MXU_FILENAME_LENGTH ) {
			start_ptr = end_ptr + 1;

			continue;  /* Go back to the top of the for(;;) loop. */
		}

		/* Copy the path directory to the pathname buffer
		 * and then null terminate it.
		 */

		memset( pathname, '\0', sizeof(pathname) );

		memcpy( pathname, start_ptr, length );

		pathname[length] = '\0';

		/* Append a directory separator to the filename.  Forward
		 * slashes work here just as well on Windows as backslashes.
		 */

		strlcat( pathname, "/", sizeof(pathname) );

		/* Append the command name. */

		strlcat( pathname, command_name, sizeof(pathname) );

		/* See if this pathname exists and is executable. */

		os_status = access( pathname, X_OK );

		MX_DEBUG( 2,("%s: pathname = '%s', os_status = %d",
				fname, pathname, os_status));

		if ( os_status == 0 ) {

			/* If the returned status is 0, we have found the
			 * command and know that it is executable, so we
			 * can return now with a success status code.
			 */

			return TRUE;
		}

		if ( end_ptr == NULL ) {
			break;		/* Exit the for(;;) loop. */
		} else {
			start_ptr = end_ptr + 1;
		}
	}

	return FALSE;
}

#else

MX_EXPORT int
mx_command_found( char *command_name )
{
	return FALSE;
}

#endif
