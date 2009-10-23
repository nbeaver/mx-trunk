/*
 * Name:    mx_spawn.c
 *
 * Purpose: MX functions for executing external commands.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006, 2009 Illinois Institute of Technology
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

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

MX_EXPORT mx_status_type
mx_spawn( char *command_line,
		unsigned long flags,
		unsigned long *process_id )
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

	parent_pid = (pid_t) mx_process_id();

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

		child_pid = (pid_t) mx_process_id();

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
mx_spawn( char *command_line,
	unsigned long flags,
	unsigned long *process_id )
{
	static const char fname[] = "mx_spawn()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"%s has not yet been implemented for this platform.", fname );
}

#endif

/*-------------------------------------------------------------------------*/

#if defined( OS_UNIX )

MX_EXPORT int
mx_process_id_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_id_exists()";

	int kill_status, saved_errno;

	/* Clean up any zombie child processes. */

	(void) waitpid( (pid_t) -1, NULL, WNOHANG | WUNTRACED );

	/* See if the process exists. */

	kill_status = kill( (pid_t) process_id, 0 );

	saved_errno = errno;

	MX_DEBUG( 2,("mx_process_exists(): kill_status = %d, saved_errno = %d",
				kill_status, saved_errno));

	if ( kill_status == 0 ) {
		return TRUE;
	} else {
		switch( saved_errno ) {
		case ESRCH:
			return FALSE;
		case EPERM:
			return TRUE;
		default:
			(void) mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected errno value %d from kill(%lu,0).  Error = '%s'",
				saved_errno, process_id,
				mx_strerror( saved_errno, NULL, 0 ) );
			return FALSE;
		}
	}

#if defined( __BORLANDC__ )
	/* Suppress 'Function should return a value ...' error. */

	return FALSE;
#endif
}

MX_EXPORT mx_status_type
mx_kill_process_id( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process_id()";

	int kill_status, saved_errno;

	kill_status = kill( (pid_t) process_id, SIGTERM );

	saved_errno = errno;

	if ( kill_status == 0 ) {

		/* Attempt to wait for zombie processes, but do not block. */

		mx_msleep(100);

		(void) waitpid( (pid_t) -1, NULL, WNOHANG | WUNTRACED );

		return MX_SUCCESSFUL_RESULT;
	} else {
		switch( saved_errno ) {
		case ESRCH:
			return mx_error( (MXE_NOT_FOUND | MXE_QUIET), fname,
				"Process %lu does not exist.", process_id);

		case EPERM:
			return mx_error(
				(MXE_PERMISSION_DENIED | MXE_QUIET), fname,
				"Cannot send the signal to process %lu.",
					process_id );

		default:
			return mx_error( MXE_FUNCTION_FAILED, fname,
		"Unexpected errno value %d from kill(%lu,0).  Error = '%s'",
				saved_errno, process_id,
				mx_strerror( saved_errno, NULL, 0 ) );
		}
	}

#if defined( __BORLANDC__ )
	/* Suppress 'Function should return a value ...' error. */

	return MX_SUCCESSFUL_RESULT;
#endif
}

/*.........................................................................*/

#else  /* Not OS_UNIX */

MX_EXPORT int
mx_process_id_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_id_exists()";

	(void) mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented for this operating system.",
			fname );

	return FALSE;
}

MX_EXPORT mx_status_type
mx_kill_process_id( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process_id()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented for this operating system.",
			fname );
}

#endif /* OS_UNIX */

/*-------------------------------------------------------------------------*/

MX_EXPORT unsigned long
mx_process_id( void )
{
	unsigned long process_id;

#if defined(OS_WIN32)
	process_id = (unsigned long) GetCurrentProcessId();

#elif defined(OS_VXWORKS)
	process_id = 0;
#else
	process_id = (unsigned long) getpid();
#endif

	return process_id;
}

/*-------------------------------------------------------------------------*/

