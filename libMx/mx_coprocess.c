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
 * Copyright 1999-2001, 2003-2004, 2010-2011, 2015-2017, 2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_COPROCESS		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_clock.h"
#include "mx_io.h"
#include "mx_coprocess.h"

/*-------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mx_signal.h"

MX_EXPORT mx_status_type
mx_coprocess_open( MX_COPROCESS **coprocess,
			char *command_line,
			unsigned long flags )
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

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
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
	MX_DEBUG(-2,("%s: timed_out = %d", fname, (int) timed_out));
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

MX_EXPORT mx_status_type
mx_coprocess_num_bytes_available( MX_COPROCESS *coprocess,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_coprocess_num_bytes_available()";

	int read_fd;
	mx_status_type mx_status;

	if ( num_bytes_available == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available_pointer passed was NULL." );
	}
	if ( coprocess == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_COPROCESS pointer passed was NULL." );
	}
	if ( coprocess->from_coprocess == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"Coprocess %p is not open for reading.", coprocess );
	}

	read_fd = fileno( coprocess->from_coprocess );

	mx_status = mx_fd_num_input_bytes_available( read_fd,
						num_bytes_available );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

#define MXCP_CLOSE_HANDLES \
	do {							\
		CloseHandle(from_coprocess_read_handle);	\
		CloseHandle(from_coprocess_write_dup_handle);	\
		CloseHandle(to_coprocess_read_dup_handle);	\
		CloseHandle(to_coprocess_write_handle);		\
	} while (0)

#include <windows.h>
#include <io.h>
#include <stddef.h>
#include <fcntl.h>

static mx_bool_type job_objects_are_available = TRUE;

/*------------------------*/

static HINSTANCE hinst_kernel32 = NULL;

typedef BOOL (*IsProcessInJob_type)( HANDLE, HANDLE, PBOOL );

static IsProcessInJob_type pIsProcessInJob = NULL;

typedef HANDLE (*CreateJobObject_type)( LPSECURITY_ATTRIBUTES, LPCTSTR );

static CreateJobObject_type pCreateJobObject = NULL;

typedef BOOL (*AssignProcessToJobObject_type)( HANDLE, HANDLE );

static AssignProcessToJobObject_type pAssignProcessToJobObject = NULL;

typedef BOOL (*TerminateJobObject_type)( HANDLE, UINT );

static TerminateJobObject_type pTerminateJobObject = NULL;

/*------------------------*/

static HANDLE
mxp_coprocess_create_job_object( HANDLE child_process_handle,
				unsigned long child_process_pid )
{
	static const char fname[] = "mxp_coprocess_create_job_object()";

	HANDLE job_object;
	BOOL status, already_in_job;
	static mx_bool_type already_in_job_warning_seen = FALSE;
	char job_object_name[100];
	DWORD last_error_code;
	TCHAR error_message[100];

	if ( job_objects_are_available == FALSE )
		return NULL;

	/* Job objects are only supported on Windows 2000 and above, so we
	 * must try to dynamically find the functions that support them.
	 */

	if ( hinst_kernel32 == NULL ) {
		hinst_kernel32 = LoadLibrary(TEXT("kernel32.dll"));

		if ( hinst_kernel32 == NULL ) {
			(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		    "Cannot load KERNEL32.DLL.  This should _NEVER_ happen.");

			job_objects_are_available = FALSE;
			return NULL;
		}
	}

	/*---*/

	if ( pIsProcessInJob == NULL ) {
		pIsProcessInJob = (IsProcessInJob_type) GetProcAddress(
				hinst_kernel32, TEXT("IsProcessInJob") );

		if ( pIsProcessInJob == NULL ) {
			job_objects_are_available = FALSE;
			return NULL;
		}
	}

	/*---*/

	if ( pCreateJobObject == NULL ) {
		pCreateJobObject = (CreateJobObject_type) GetProcAddress(
				hinst_kernel32, TEXT("CreateJobObjectA") );

		if ( pCreateJobObject == NULL ) {
			job_objects_are_available = FALSE;
			return NULL;
		}
	}

	/*---*/

	if ( pAssignProcessToJobObject == NULL ) {
		pAssignProcessToJobObject =
			(AssignProcessToJobObject_type) GetProcAddress(
			hinst_kernel32, TEXT("AssignProcessToJobObject") );

		if ( pAssignProcessToJobObject == NULL ) {
			job_objects_are_available = FALSE;
			return NULL;
		}
	}

	/*---*/

	if ( pTerminateJobObject == NULL ) {
		pTerminateJobObject = (TerminateJobObject_type) GetProcAddress(
				hinst_kernel32, TEXT("TerminateJobObject") );

		if ( pTerminateJobObject == NULL ) {
			job_objects_are_available = FALSE;
			return NULL;
		}
	}

	/* We cannot assign children to a new job if we are already in a job. */

	status = pIsProcessInJob( GetCurrentProcess(), NULL, &already_in_job );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The call to IsProcessInJob() failed.  "
		"error code = %lu, error message = '%s'.",
			last_error_code, error_message );
	}

	if ( already_in_job ) {
		if ( already_in_job_warning_seen == FALSE ) {
			mx_warning( "The current process is already a member "
			"of a Win32 job, so we cannot put the coprocess into "
			"a separate job.  We will not be able to kill all "
			"children of the coprocess." );

			already_in_job_warning_seen = TRUE;
		}
		return NULL;
	}

	/* We must create a unique name for the job object, since 
	 * CreateJobObject() will fail if the name is not unique.
	 */

	snprintf( job_object_name, sizeof(job_object_name),
		"MX_Job_%lu", GetCurrentProcessId() );

	job_object = pCreateJobObject( NULL, job_object_name );

	if ( job_object == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to create a job object failed.  "
		"error code = %lu, error message = '%s'.",
			last_error_code, error_message );
	}

	/* We finish by assigning the new child process to a job. */

	status = pAssignProcessToJobObject( job_object, child_process_handle );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to assign child process %lu to job %p failed.  "
		"error code = %lu, error message = '%s'.",
			child_process_pid, job_object,
			last_error_code, error_message );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: Assigned child process %lu to job %p.",
		fname, child_process_pid, job_object ));
#endif

	return job_object;
}

MX_EXPORT mx_status_type
mx_coprocess_open( MX_COPROCESS **coprocess,
			char *command_line,
			unsigned long flags )
{
	static const char fname[] = "mx_coprocess_open()";

	BOOL os_status;
	DWORD resume_status;
	STARTUPINFO startup_info;
	PROCESS_INFORMATION process_info;
	DWORD creation_flags;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	HANDLE from_coprocess_read_handle;
	HANDLE from_coprocess_write_handle;
	HANDLE to_coprocess_read_handle;
	HANDLE to_coprocess_write_handle;

	HANDLE process_pseudo_handle;
	HANDLE from_coprocess_write_dup_handle;
	HANDLE to_coprocess_read_dup_handle;
	HANDLE stderr_dup_handle;

	SECURITY_ATTRIBUTES from_pipe_attributes;
	SECURITY_ATTRIBUTES to_pipe_attributes;
	int from_file_descriptor;
	int to_file_descriptor;
	FILE *from_pipe, *to_pipe;
	int saved_errno;

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

	/* Create security descriptors for the new pipes that we
	 * are about to create.
	 */

	from_pipe_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	from_pipe_attributes.lpSecurityDescriptor = NULL;
	from_pipe_attributes.bInheritHandle = TRUE;

	to_pipe_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	to_pipe_attributes.lpSecurityDescriptor = NULL;
	to_pipe_attributes.bInheritHandle = TRUE;

	/* Create an anonymous pipe used to read from the coprocess. */

	os_status = CreatePipe( &from_coprocess_read_handle,
				&from_coprocess_write_handle,
				&from_pipe_attributes, 0 );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to create a pipe to read from the coprocess "
		"failed.  Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	/* Create an anonymous pipe used to write to the coprocess. */

	os_status = CreatePipe( &to_coprocess_read_handle,
				&to_coprocess_write_handle,
				&to_pipe_attributes, 0 );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		CloseHandle( from_coprocess_read_handle );
		CloseHandle( from_coprocess_write_handle );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to create a pipe to write to the coprocess "
		"failed.  Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: pipes created.", fname));
#endif
	process_pseudo_handle = GetCurrentProcess();

	/* Create an inheritable handle for the coprocess's standard output. */

	os_status = DuplicateHandle( process_pseudo_handle,
					from_coprocess_write_handle,
					process_pseudo_handle,
					&from_coprocess_write_dup_handle,
					0,
					TRUE,
					DUPLICATE_SAME_ACCESS );

	last_error_code = GetLastError();

	CloseHandle( from_coprocess_write_handle );

	if ( os_status == 0 ) {
		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		CloseHandle( from_coprocess_read_handle );
		CloseHandle( to_coprocess_read_handle );
		CloseHandle( to_coprocess_write_handle );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to create an inheritable handle for the "
		"coprocess's standard output failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: from_coprocess_write_dup_handle = %p",
		fname, from_coprocess_write_dup_handle));
#endif
	/* Create an inheritable handle for the coprocess's standard input. */

	os_status = DuplicateHandle( process_pseudo_handle,
					to_coprocess_read_handle,
					process_pseudo_handle,
					&to_coprocess_read_dup_handle,
					0,
					TRUE,
					DUPLICATE_SAME_ACCESS );

	last_error_code = GetLastError();

	CloseHandle( to_coprocess_read_handle );

	if ( os_status == 0 ) {
		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		CloseHandle( from_coprocess_read_handle );
		CloseHandle( from_coprocess_write_dup_handle );
		CloseHandle( to_coprocess_write_handle );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to create an inheritable handle for the "
		"coprocess's standard input failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: to_coprocess_read_dup_handle = %p",
		fname, to_coprocess_read_dup_handle));
#endif
	/* Get an inheritable handle for the coprocess's standard error.
	 *
	 * The coprocess will, at least initially, have the same standard
	 * error as the parent process.
	 */

	os_status = DuplicateHandle( process_pseudo_handle,
					GetStdHandle( STD_ERROR_HANDLE ),
					process_pseudo_handle,
					&stderr_dup_handle,
					0,
					TRUE,
					DUPLICATE_SAME_ACCESS );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		MXCP_CLOSE_HANDLES;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"An attempt to create an inheritable handle for standard "
		"error failed.  Win32 error code = %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: stderr_dup_handle = %p", fname, stderr_dup_handle));
#endif

	/* Setup the STARTUPINFO for the coprocess to redirect standard input
	 * and standard output to the pipes that we have just created.
	 */

	memset( &startup_info, 0, sizeof(startup_info) );

	startup_info.cb = sizeof(startup_info);

	startup_info.dwFlags = STARTF_USESTDHANDLES;

	startup_info.hStdInput = to_coprocess_read_dup_handle;

	startup_info.hStdOutput = from_coprocess_write_dup_handle;

	startup_info.hStdError = stderr_dup_handle;

	/* Setup stdio-style FILE pointers for the new pipes
	 * for the parent process to use.
	 */

	from_file_descriptor = _open_osfhandle(
					(intptr_t) from_coprocess_read_handle,
					_O_RDONLY );

	if ( from_file_descriptor == (-1) ) {
		saved_errno = errno;

		MXCP_CLOSE_HANDLES;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An error occurred while trying to convert the Win32 "
		"'from coprocess' handle into a file descriptor.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	from_pipe = _fdopen( from_file_descriptor, "r" );

	if ( from_pipe == NULL ) {
		saved_errno = errno;

		MXCP_CLOSE_HANDLES;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An error occurred while trying to convert the Win32 "
		"'from coprocess' handle into a FILE pointer.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	/*--------*/

	to_file_descriptor = _open_osfhandle(
					(intptr_t) to_coprocess_write_handle,
					_O_WRONLY );

	if ( to_file_descriptor == (-1) ) {
		saved_errno = errno;

		MXCP_CLOSE_HANDLES;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An error occurred while trying to convert the Win32 "
		"'to coprocess' handle into a file descriptor.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	to_pipe = _fdopen( to_file_descriptor, "w" );

	if ( to_pipe == NULL ) {
		saved_errno = errno;

		MXCP_CLOSE_HANDLES;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An error occurred while trying to convert the Win32 "
		"'to coprocess' handle into a FILE pointer.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	/*--------*/

	(*coprocess)->from_coprocess = from_pipe;
	(*coprocess)->to_coprocess   = to_pipe;

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: FILE * from_coprocess = %p, to_coprocess = %p",
		fname, from_pipe, to_pipe));
#endif

	/*------------------------------------*/
	/* Prepare to create the new process. */
	/*------------------------------------*/

	memset( &process_info, 0, sizeof(process_info) );

	creation_flags = 0;

	if ( flags & MXF_CP_CREATE_PROCESS_GROUP ) {
		creation_flags |= CREATE_SUSPENDED;
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: creating the coprocess.", fname));
#endif

	os_status = CreateProcess( NULL,
					command_line,
					NULL,
					NULL,
					TRUE,
					creation_flags,
					NULL,
					NULL,
					&startup_info,
					&process_info );

	if ( os_status == 0 ) {
		last_error_code = GetLastError();

		MXCP_CLOSE_HANDLES;

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to create a process using the command line '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
			command_line, last_error_code, message_buffer );
	}

	(*coprocess)->coprocess_pid = process_info.dwProcessId;

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: coprocess PID = %lu",
		fname, (*coprocess)->coprocess_pid));
#endif

	/* If the MXF_CP_CREATE_PROCESS_GROUP flag was set, then we create
	 * a Windows job object, since that is a more useful concept on
	 * the Win32 platform.
	 */

	if ( (flags & MXF_CP_CREATE_PROCESS_GROUP) == 0 ) {
		(*coprocess)->private = NULL;
	} else {
		(*coprocess)->private = mxp_coprocess_create_job_object(
						process_info.hProcess,
						(*coprocess)->coprocess_pid );

		/* Regardless of whether or not a job object was created,
		 * we must now resume the process.
		 */

		resume_status = ResumeThread( process_info.hThread );

		if ( resume_status == -1 ) {
			last_error_code = GetLastError();

			MXCP_CLOSE_HANDLES;

			mx_win32_error_message( last_error_code,
				message_buffer, sizeof(message_buffer) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to resume suspended coprocess %lu.  "
			"Win32 error code = %ld, error message = '%s'.",
				(*coprocess)->coprocess_pid,
				last_error_code, message_buffer );
		}
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: job object = %p", fname, (*coprocess)->private ));
#endif

	/* Close some handles we do not need to prevent a memory leak. */

	CloseHandle( process_info.hProcess );
	CloseHandle( process_info.hThread );

	CloseHandle( to_coprocess_read_dup_handle );
	CloseHandle( from_coprocess_write_dup_handle );

	return MX_SUCCESSFUL_RESULT;
}

static void
mxp_coprocess_kill_job( HANDLE job_object )
{
	static const char fname[] = "mxp_coprocess_kill_job()";

	BOOL terminate_status;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( job_object == NULL )
		return;

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: deleting job object %p", fname, job_object));
#endif

	terminate_status = pTerminateJobObject( job_object, 0 );

	if ( terminate_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to terminate job %p.  "
		"Error code = %lu, error message = '%s'.",
			job_object, last_error_code, message_buffer );
	}

	CloseHandle( job_object );

	return;
}

MX_EXPORT mx_status_type
mx_coprocess_close( MX_COPROCESS *coprocess, double timeout_in_seconds )
{
	static const char fname[] = "mx_coprocess_close()";

	unsigned long coprocess_pid;
	HANDLE coprocess_handle;
	HANDLE job_object;
	DWORD timeout_ms;
	DWORD wait_status;
	BOOL terminate_status;

	double wait_time;
	DWORD wait_ms;

	DWORD last_error_code;
	TCHAR message_buffer[100];

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s invoked for PID %lu with a timeout of %g seconds.",
		fname, coprocess->coprocess_pid, timeout_in_seconds ));
#endif
	/* Close the coprocess FILE objects. */

	if ( coprocess->from_coprocess != NULL ) {
		fclose( coprocess->from_coprocess );
	}

	if ( coprocess->to_coprocess != NULL ) {
		fclose( coprocess->to_coprocess );
	}

	coprocess_pid = coprocess->coprocess_pid;

	if ( job_objects_are_available ) {
		job_object = coprocess->private;
	} else {
		job_object = NULL;
	}

	mx_free( coprocess );

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: coprocess FILE pointers are now closed.", fname));
#endif
	/* Get a Win32 handle for the process from the process ID. */

	coprocess_handle = OpenProcess( PROCESS_ALL_ACCESS,
					FALSE, coprocess_pid );

	if ( coprocess_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		if ( job_object != NULL ) {
			mxp_coprocess_kill_job( job_object );
		}

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get a process handle for process id %lu.  "
		"Error code = %lu, error message = '%s'.",
			coprocess_pid, last_error_code, message_buffer );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: coprocess PID %lu, handle = %p",
		fname, coprocess_pid, coprocess_handle));
#endif

	/* If the timeout is negative, then we wait forever for the
	 * child to exit.
	 */

	if ( timeout_in_seconds < 0.0 ) {
#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: waiting forever for PID %lu to exit.",
			fname, coprocess_pid));
#endif
		(void) WaitForSingleObject( coprocess_handle, INFINITE );

		CloseHandle( coprocess_handle );

		if ( job_object != NULL ) {
			mxp_coprocess_kill_job( job_object );
		}

		return MX_SUCCESSFUL_RESULT;
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: waiting %g seconds for PID %lu to exit.",
			fname, timeout_in_seconds, coprocess_pid));
#endif

	/* Wait for the requested timeout interval for the process to exit. */

	timeout_ms = mx_round( 1000.0 * timeout_in_seconds );

	wait_status = WaitForSingleObject( coprocess_handle, timeout_ms );

	/* If the process has exited by now, then we are done. */

	if ( wait_status == WAIT_OBJECT_0 ) {
#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: process %lu has exited.",
			fname, coprocess_pid));
#endif
		CloseHandle( coprocess_handle );

		if ( job_object != NULL ) {
			mxp_coprocess_kill_job( job_object );
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* If the coprocess has _not_ exited by now, then we kill it. */

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: we must kill PID %lu", fname, coprocess_pid));
#endif

	terminate_status = TerminateProcess( coprocess_handle, 127 );

	if ( terminate_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		CloseHandle( coprocess_handle );

		if ( job_object != NULL ) {
			mxp_coprocess_kill_job( job_object );
		}

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to terminate process id %lu.  "
		"Win32 error code = %d, error message = '%s'.",
			coprocess_pid, (int) last_error_code, message_buffer );
	}

	/* Wait a short time for the process to die. */

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s: waiting for killed PID %lu to exit",
			fname, coprocess_pid));
#endif
	wait_time = 1.0;	/* in seconds */

	wait_ms = mx_round( 1000.0 * wait_time );

	wait_status = WaitForSingleObject( coprocess_handle, 1000 );

	/* See if the process has finally exited. */

	if ( wait_status == WAIT_OBJECT_0 ) {
#if DEBUG_COPROCESS
		MX_DEBUG(-2,("%s: process %lu has finally exited.",
			fname, coprocess_pid));
#endif
		CloseHandle( coprocess_handle );

		if ( job_object != NULL ) {
			mxp_coprocess_kill_job( job_object );
		}

		return MX_SUCCESSFUL_RESULT;
	}

	CloseHandle( coprocess_handle );

	if ( wait_ms == 1000 ) {
		mx_warning("%s: Coprocess %lu was terminated "
			"1 second ago, but has not died.",
			fname, coprocess_pid );
	} else {
		mx_warning("%s: Coprocess %lu was terminated "
			"%g seconds ago, but has not died.",
			fname, coprocess_pid, wait_time );
	}

	if ( job_object != NULL ) {
		mxp_coprocess_kill_job( job_object );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_coprocess_num_bytes_available( MX_COPROCESS *coprocess,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_coprocess_num_bytes_available()";

	int read_fd;
	HANDLE read_handle;
	BOOL pipe_status;
	DWORD last_error_code, bytes_read, total_bytes_avail;
	TCHAR message_buffer[100];
	TCHAR peek_buffer[1000];

	if ( num_bytes_available == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available_pointer passed was NULL." );
	}
	if ( coprocess == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_COPROCESS pointer passed was NULL." );
	}
	if ( coprocess->from_coprocess == NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"Coprocess %p is not open for reading.", coprocess );
	}

	read_fd = _fileno( coprocess->from_coprocess );

	/* When calling _get_osfhandle(), we cast first to (intptr_t)
	 * so that on 64-bit windows the numerical handle value will
	 * be converted to an integer type compatible with a pointer
	 * before being cast to a HANDLE (really void *).
	 */

	read_handle = (HANDLE) (intptr_t) _get_osfhandle( read_fd );

	if ( read_handle == INVALID_HANDLE_VALUE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The read handle for MX coprocess %p is not valid.",
			coprocess );
	}

	pipe_status = PeekNamedPipe( read_handle,
			peek_buffer, sizeof(peek_buffer),
			&bytes_read, &total_bytes_avail, NULL );

#if DEBUG_COPROCESS
	if ( total_bytes_avail != 0 ) {
		MX_DEBUG(-2,("Peek: bytes_read = %ld, total_bytes_avail = %ld",
				(long) bytes_read, (long) total_bytes_avail ));
	}
#endif

	if ( pipe_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to determine the number of bytes available "
		"from MX coprocess %p failed.  "
		"Win32 error code = %ld, error message = '%s'.",
			coprocess, last_error_code, message_buffer );
	}

	*num_bytes_available = total_bytes_avail;

	return MX_SUCCESSFUL_RESULT;
}


/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

#include <errno.h>
#include <fcntl.h>

#include <starlet.h>
#include <descrip.h>
#include <ssdef.h>
#include <clidef.h>
#include <dvidef.h>
#include <lib$routines.h>

typedef struct {
	struct dsc$descriptor_s from_descriptor;
	struct dsc$descriptor_s to_descriptor;
	char from_name[40];
	char to_name[40];
	short int from_channel;
	short int to_channel;
} MX_VMS_COPROCESS_PRIVATE;

MX_EXPORT mx_status_type
mx_coprocess_open( MX_COPROCESS **coprocess,
			char *command_line,
			unsigned long flags )
{
	static const char fname[] = "mx_coprocess_open()";

	MX_VMS_COPROCESS_PRIVATE *vms_private;
	struct dsc$descriptor_s command_descriptor;
	short int from_channel, to_channel;
	int from_fd, to_fd;
	int vms_status, saved_errno;
	int32_t vms_flags;
	unsigned int vms_pid;

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

	vms_private = malloc( sizeof(MX_VMS_COPROCESS_PRIVATE) );

	if ( vms_private == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_VMS_COPROCESS_PRIVATE structure." );
	}

	(*coprocess)->private = vms_private;

	/* VMS mailboxes are actually bi-directional.  However, we want
	 * to have separate FILE pointers for 'from_coprocess' and
	 * 'to_coprocess', so we really need two mailboxes.
	 */

	/* Create the 'from' mailbox. */

	snprintf( vms_private->from_name,
		sizeof( vms_private->from_name ),
		"MX_FROM_COPROCESS_%lu", mx_process_id() );

	vms_private->from_descriptor.dsc$w_length =
					strlen( vms_private->from_name ) + 1;
	vms_private->from_descriptor.dsc$a_pointer = vms_private->from_name;
	vms_private->from_descriptor.dsc$b_class = DSC$K_CLASS_S;
	vms_private->from_descriptor.dsc$b_dtype = DSC$K_DTYPE_T;

	vms_status = sys$crembx( 0, &from_channel, 0, 0, 0, 0,
				&(vms_private->from_descriptor), 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to create mailbox '%s' failed with a "
		"VMS error code = %d, error message = '%s'.",
			vms_private->from_name,
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	vms_private->from_channel = from_channel;

	from_fd = open( vms_private->from_name, O_RDONLY );

	if ( from_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to open a Posix-style file descriptor for "
		"VMS mailbox '%s'.  Errno = d, error message = '%s'",
			vms_private->from_name,
			saved_errno, strerror(saved_errno) );
	}

	(*coprocess)->from_coprocess = fdopen( from_fd, "r" );

	if ( (*coprocess)->from_coprocess == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to convert Posix-style file descriptor %d for "
		"VMS mailbox '%s' failed.  Errno = %d, error message = '%s'",
			from_fd, vms_private->from_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Create the 'to' mailbox. */

	snprintf( vms_private->to_name, sizeof( vms_private->to_name ),
		"MX_TO_COPROCESS_%lu", mx_process_id() );

	vms_private->to_descriptor.dsc$w_length =
					strlen( vms_private->to_name ) + 1;
	vms_private->to_descriptor.dsc$a_pointer = vms_private->to_name;
	vms_private->to_descriptor.dsc$b_class = DSC$K_CLASS_S;
	vms_private->to_descriptor.dsc$b_dtype = DSC$K_DTYPE_T;

	vms_status = sys$crembx( 0, &to_channel, 0, 0, 0, 0,
				&(vms_private->to_descriptor), 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to create mailbox '%s' failed with a "
		"VMS error code = %d, error message = '%s'.",
			vms_private->to_name,
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	vms_private->to_channel = to_channel;

	to_fd = open( vms_private->to_name, O_WRONLY );

	if ( to_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to open a Posix-style file descriptor for "
		"VMS mailbox '%s'.  Errno = d, error message = '%s'",
			vms_private->to_name,
			saved_errno, strerror(saved_errno) );
	}

	(*coprocess)->to_coprocess = fdopen( to_fd, "w" );

	if ( (*coprocess)->to_coprocess == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"The attempt to convert Posix-style file descriptor %d for "
		"VMS mailbox '%s' failed.  Errno = %d, error message = '%s'",
			to_fd, vms_private->to_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Now we create the coprocess as a VMS subprocess. */

	command_descriptor.dsc$w_length = strlen( command_line ) + 1;
	command_descriptor.dsc$a_pointer = command_line;
	command_descriptor.dsc$b_class = DSC$K_CLASS_S;
	command_descriptor.dsc$b_dtype = DSC$K_DTYPE_T;

	vms_flags = CLI$M_NOWAIT;

	vms_status = lib$spawn( &command_descriptor,
				&(vms_private->to_descriptor),
				&(vms_private->from_descriptor),
				&vms_flags, 0, &vms_pid );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to create a process using the command line '%s'.  "
		"VMS error number = %d, error_message = '%s'",
		command_line, vms_status, strerror( EVMSERR, vms_status ) );
	}

	(*coprocess)->coprocess_pid = vms_pid;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_coprocess_close( MX_COPROCESS *coprocess, double timeout_in_seconds )
{
	static const char fname[] = "mx_coprocess_close()";

	MX_VMS_COPROCESS_PRIVATE *vms_private;
	int vms_status;
	mx_status_type mx_status;

	if ( coprocess == (MX_COPROCESS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null coprocess pointer passed." );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s invoked for coprocess %p", fname, coprocess));
#endif

	/* Kill the coprocess itself. */

	mx_status = mx_kill_process_id( coprocess->coprocess_pid );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Close the file descriptors. */

	fclose( coprocess->to_coprocess );

	fclose( coprocess->from_coprocess );

	/* Finish by releasing the mailbox channels. */

	vms_private = coprocess->private;

	if ( vms_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_VMS_COPROCESS_PRIVATE pointer for coprocess %p is NULL.",
	    		coprocess );
	}

	vms_status = sys$dassgn( vms_private->to_channel );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to close VMS channel %d to coprocess.  "
		"VMS error number = %d, error message = '%s'",
			vms_private->to_channel,
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	vms_status = sys$dassgn( vms_private->from_channel );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to close VMS channel %d from coprocess.  "
		"VMS error number = %d, error message = '%s'",
			vms_private->from_channel,
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	mx_free( vms_private );
	mx_free( coprocess );

	return MX_SUCCESSFUL_RESULT;
}

/* FIXME: The following routine might work on Alpha or Itanium, but does
 * not work on Vax.
 */

MX_EXPORT mx_status_type
mx_coprocess_num_bytes_available( MX_COPROCESS *coprocess,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_coprocess_num_bytes_available()";

	MX_VMS_COPROCESS_PRIVATE *vms_private;
	unsigned int mbx_size, mbx_avail;
	int vms_status;

	struct {
		unsigned short buffer_length;
		unsigned short item_code;
		void *buffer_address;
		void *return_length_address;
	} dvi_list[] = {
#if defined(__VAX)
	    { sizeof(mbx_size), 0, &mbx_size, NULL },
	    { sizeof(mbx_avail), 0, &mbx_avail, NULL },
#else
	    { sizeof(mbx_size), DVI$_MAILBOX_INITIAL_QUOTA, &mbx_size, NULL },
	    { sizeof(mbx_avail), DVI$_MAILBOX_BUFFER_QUOTA, &mbx_avail, NULL },
#endif
	    { 0, 0, NULL, NULL }
	};

	short iosb[4];

	if ( coprocess == (MX_COPROCESS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null coprocess pointer passed." );
	}
	if ( num_bytes_available == (size_t *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Null num_bytes_available pointer passed." );
	}

#if DEBUG_COPROCESS
	MX_DEBUG(-2,("%s invoked for coprocess %p", fname, coprocess));
#endif

	vms_private = coprocess->private;

	if ( vms_private == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_VMS_COPROCESS_PRIVATE pointer for coprocess %p is NULL.",
	    		coprocess );
	}

	vms_status = sys$getdviw( 0, 0,
				&(vms_private->from_descriptor),
				dvi_list, iosb, 0, 0, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to get VMS mailbox statistics for VMS mailbox '%s'.  "
		"VMS error number = %d, error message = '%s'",
			vms_private->from_name,
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	*num_bytes_available = mbx_size - mbx_avail;

#if DEBUG_COPROCESS
	MX_DEBUG(-2,
	("%s: mbx_size = %u, mbx_avail = %u, num_bytes_available = %lu",
	 	fname, mbx_size, mbx_avail, *num_bytes_available ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_RTEMS) || defined(OS_ECOS) || defined(OS_VXWORKS) \
	|| defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_coprocess_open( MX_COPROCESS **coprocess,
			char *command_line,
			unsigned long flags )
{
	static const char fname[] = "mx_coprocess_open()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"This platform does not support multiple processes." );
}

MX_EXPORT mx_status_type
mx_coprocess_close( MX_COPROCESS *coprocess, double timeout_in_seconds )
{
	static const char fname[] = "mx_coprocess_close()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"This platform does not support multiple processes." );
}

MX_EXPORT mx_status_type
mx_coprocess_num_bytes_available( MX_COPROCESS *coprocess,
				size_t *num_bytes_available )
{
	static const char fname[] = "mx_coprocess_num_bytes_available()";

	if ( num_bytes_available != NULL ) {
		*num_bytes_available = 0;
	}

	return mx_error( MXE_UNSUPPORTED, fname,
	"This platform does not support multiple processes." );
}

/*-------------------------------------------------------------------------*/

#else
#error Coprocess support has not yet been implemented for this platform.
#endif

