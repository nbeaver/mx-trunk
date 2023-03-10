/*
 * Name:    mx_spawn.c
 *
 * Purpose: MX functions for executing external commands.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006, 2009-2010, 2013-2021, 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_SPAWN_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_unistd.h"
#include "mx_signal.h"

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

#include <sys/types.h>
#include <sys/wait.h>

MX_EXPORT mx_status_type
mx_spawn( char *command_line,
		unsigned long flags,
		unsigned long *child_process_id )
{
	static const char fname[] = "mx_spawn()";

	pid_t fork_pid, parent_pid, child_pid, session_id;
	int result, saved_errno;
	int i, argc, envc;
	char **argv, **envp;

	if ( command_line == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command_line pointer passed was NULL." );
	}

#if MX_SPAWN_DEBUG
	MX_DEBUG(-2,("%s: command_line = '%s'", fname, command_line));
#endif

	/* Parse the command line.
	 *
	 * We have to do this here before we call fork(), because
	 * mx_parse_command_line() does a bunch of things that are
	 * not safe to do in a threaded program between fork() and
	 * execvp().  In the parent, we will need to discard the
	 * argv and envp pointers to prevent a memory leak.
	 *    
	 * Note that third-party libraries (such as glibc!) may use
	 * threads behind the scenes.
	 *    
	 * Some reasons for why this is necessary can be found here
	 *
	 *     http://www.linuxprogrammingblog.com/threads-and-fork-think-twice-before-using-them
	 *
	 */

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
			MX_DEBUG(-2,("%s: argv[%d] = '%s'", fname, i, argv[i]));
		}
		MX_DEBUG(-2,("%s: argv[%d] = '%s'", fname, i, argv[i]));
	}
#endif

	/* Create the child process. */

	parent_pid = getpid();

	fork_pid = fork();

	if ( fork_pid == (-1) ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Attempt to fork() failed.  errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	if ( fork_pid == 0 ) {

		/********** Child process. **********/

		child_pid = getpid();

#if MX_SPAWN_DEBUG
		MX_DEBUG(-2,("%s: This is child process %lu.",
			fname, (unsigned long) child_pid ));
#endif

		if ( flags & MXF_SPAWN_NEW_SESSION ) {
			session_id = setsid();

			if ( session_id < 0 ) {
				saved_errno = errno;

				(void) mx_error( MXE_FUNCTION_FAILED, fname,
				"The attempt by child process %lu (parent %lu) "
				"to create a new session using setsid() "
				"failed.  Errno = %d, error message = '%s'",
					(unsigned long) child_pid,
					(unsigned long) parent_pid,
					saved_errno,
					strerror( saved_errno ) );
				exit(1);
			}
		}

#if defined(OS_LINUX)
		/*--------------------------------------------------------*/

		/* If requested, remove the environment variable LD_PRELOAD
		 * from the environment passed to the child.
		 */

		if ( flags & MXF_SPAWN_NO_PRELOAD ) {

			char *ptr;
			int max_attempts = 20;

			/* It is possible, but unlikely that LD_PRELOAD
			 * may be found in the environment more than once.
			 * Just in case it is, we loop until all copies
			 * of LD_PRELOAD are gone.
			 */

			for ( i = 0; i < max_attempts; i++ ) {
				ptr = getenv( "LD_PRELOAD" );

#if MX_SPAWN_DEBUG
				MX_DEBUG(-2,("%s: i = %d, LD_PRELOAD = '%s'",
					fname, i, ptr));
#endif

				if ( ptr == NULL )
					break;

				unsetenv( "LD_PRELOAD" );
			}

			if ( i >= max_attempts ) {
				mx_warning(
    "%s: %d attempts to remove LD_PRELOAD from the environment have failed.",
					fname, max_attempts );
			}
		}
#endif /* OS_LINUX */

		/*--------------------------------------------------------*/

		/* Add the environment variables found by the earlier call
		 * to mx_parse_command_line() to the environment.
		 */

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

		/*--------------------------------------------------------*/

		/* Now execute the external command. */

#if MX_SPAWN_DEBUG
		MX_DEBUG(-2,
		("%s: About to execute the external command.", fname));
#endif

		(void) execvp( argv[0], argv );

		/* If execvp() succeeds, then we should not be able
		 * to get here, since this process image will have
		 * been replaced by the new process image.
		 */

		saved_errno = errno;

		fprintf( stderr, "Child process %lu could not execute "
		"command '%s'.  Errno = %d, error message = '%s'\n",
			(unsigned long) child_pid, argv[0],
			saved_errno, strerror( saved_errno ) );

		/* The only safe thing we can do now is exit. */

		exit(1);
	} else {
		/********** Parent process. **********/

#if MX_SPAWN_DEBUG
		MX_DEBUG(-2,("%s: This is parent process %lu.",
			fname, (unsigned long) parent_pid));
#endif

		child_pid = fork_pid;

		/* Send the child PID to the caller if requested. */

		if ( child_process_id != NULL ) {
			*child_process_id = (unsigned long) child_pid;
		}

		/* Free the argv and envp pointers that were malloced
		 * by mx_parse_command_line().
		 */

		mx_free( argv );
		mx_free( envp );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_spawn( char *command_line,
	unsigned long flags,
	unsigned long *child_process_id )
{
	static const char fname[] = "mx_spawn()";

	BOOL status;
	STARTUPINFO startup_info;
	PROCESS_INFORMATION process_info;
	DWORD cpflags;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( flags & MXF_SPAWN_NEW_SESSION ) {
		cpflags = CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP;
	} else {
		cpflags = CREATE_DEFAULT_ERROR_MODE;
	}

	memset( &startup_info, 0, sizeof(startup_info) );

	startup_info.cb = sizeof(startup_info);

	memset( &process_info, 0, sizeof(process_info) );

	status = CreateProcess( NULL,
				command_line,
				NULL,
				NULL,
				FALSE,
				cpflags,
				NULL,
				NULL,
				&startup_info,
				&process_info );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to create a process using the command line '%s'.  "
		"Win32 error code = %ld, error message = '%s'.",
			command_line, last_error_code, message_buffer );
	}

	if ( child_process_id != NULL ) {
		*child_process_id = process_info.dwProcessId;
	}

	CloseHandle( process_info.hProcess );
	CloseHandle( process_info.hThread );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

#include <ssdef.h>
#include <descrip.h>
#include <starlet.h>
#include <lib$routines.h>

MX_EXPORT mx_status_type
mx_spawn( char *command_line,
	unsigned long flags,
	unsigned long *child_process_id )
{
	static const char fname[] = "mx_spawn()";

	int vms_status;
	int32_t vms_flags;
	unsigned int vms_pid;
	$DESCRIPTOR( command_descriptor, command_line );

	vms_flags = 1;	/* NOWAIT */

	vms_status = lib$spawn( &command_descriptor,
				0, 0, &vms_flags, 0, &vms_pid );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to create a process using the command line '%s'.  "
		"VMS error number = %d, error message = '%s'",
		command_line, vms_status, strerror( EVMSERR, vms_status ) );
	}

	if ( child_process_id != NULL ) {
		*child_process_id = vms_pid;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_spawn( char *command_line,
	unsigned long flags,
	unsigned long *child_process_id )
{
	static const char fname[] = "mx_spawn()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Spawning processes is not supported on this platform." );
}

/*-------------------------------------------------------------------------*/

#else
#error mx_spawn() has not yet been implemented for this platform.
#endif

/*=========================================================================*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

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

	MX_DEBUG( 2,("%s: kill_status = %d, saved_errno = %d",
				fname, kill_status, saved_errno));

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

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

MX_EXPORT int
mx_process_id_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_id_exists()";

	HANDLE process_handle;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	process_handle = OpenProcess( PROCESS_QUERY_INFORMATION,
					FALSE, process_id );

	if ( process_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get a process handle for process ID %lu.  "
		"Win32 error code = %ld, error message = '%s'.",
			process_id, last_error_code, message_buffer );

		return FALSE;
	}

	CloseHandle( process_handle );

	return TRUE;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

MX_EXPORT int
mx_process_id_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_id_exists()";

	int vms_status;
	unsigned int vms_pid;

	vms_status = sys$getjpiw( 0, &vms_pid );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Cannot get job/process information for process id %d.  "
		"VMS error number %d, error message = '%s'",
			vms_pid, vms_status, strerror( EVMSERR, vms_status ) );

		return FALSE;
	}

	return TRUE;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_DJGPP)

MX_EXPORT int
mx_process_id_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_id_exists()";

	(void) mx_error( MXE_UNSUPPORTED, fname,
	"Multiple processes are not supported on this platform." );

	return FALSE;
}

/*-------------------------------------------------------------------------*/

#else
#error mx_process_id_exists() has not yet been implemented for this platform.
#endif

/*=========================================================================*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

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

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

MX_EXPORT mx_status_type
mx_kill_process_id( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process_id()";

	HANDLE process_handle;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	BOOL status;
	mx_status_type mx_status;

	process_handle = OpenProcess( PROCESS_ALL_ACCESS,
					FALSE, process_id );

	if ( process_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get a process handle for process ID %lu.  "
		"Win32 error code = %ld, error message = '%s'.",
			process_id, last_error_code, message_buffer );
	}

	status = TerminateProcess( process_handle, 127 );

	if ( status != 0 ) {
		mx_status = MX_SUCCESSFUL_RESULT;
	} else {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to terminate process handle %p, process ID %lu.  "
		"Win32 error code = %ld, error message = '%s'.",
			process_handle, process_id,
			last_error_code, message_buffer );
	}

	CloseHandle( process_handle );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

MX_EXPORT mx_status_type
mx_kill_process_id( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process_id()";

	int vms_status;
	unsigned int vms_pid;

	vms_pid = process_id;

	vms_status = sys$forcex( &vms_pid, 0, 0 );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to terminate process ID %u.  "
		"VMS error code = %d, error message = '%s'",
			vms_pid, vms_status, strerror( EVMSERR, vms_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_kill_process_id( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process_id()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Multiple processes are not supported on this platform." );
}

/*-------------------------------------------------------------------------*/

#else
#error mx_kill_process_id() has not yet been implemented for this platform.
#endif

/*=========================================================================*/

MX_EXPORT unsigned long
mx_get_process_id( void )
{
	unsigned long process_id;

#if defined(OS_WIN32)
	process_id = (unsigned long) GetCurrentProcessId();

#elif 0 && defined(OS_VXWORKS)
	process_id = 0;
#else
	process_id = (unsigned long) getpid();
#endif

	return process_id;
}

/*=========================================================================*/

#if defined(OS_WIN32)

/* This version of the code is based on this web page:
 *
 *     https://www.codeproject.com/Articles/9893/Get-Parent-Process-PID
 */

#include <tlhelp32.h>

MX_EXPORT unsigned long
mx_get_parent_process_id( unsigned long process_id )
{
	static const char fname[] = "mx_get_parent_process_id()";

	PROCESSENTRY32 process_entry;
	BOOL os_status;
	DWORD last_error_code;
	unsigned long parent_process_id = 0L;

	HANDLE toolhelp_handle =
		CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );

	if ( toolhelp_handle == INVALID_HANDLE_VALUE ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to acquire the CreateToolhelp32_Snapshot "
		"handle failed." );

		return 0;
	}

	process_entry.dwSize = sizeof(process_entry);

	os_status = Process32First( toolhelp_handle, &process_entry );

	if ( os_status != 0 ) {
		CloseHandle( toolhelp_handle );
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to call Process32First() failed." );

		return 0;
	}

	while( TRUE ) {
		if ( process_id == process_entry.th32ProcessID ) {
			/* Success!  We have found the parent process ID.
			 * Now we can returne that to our caller.
			 */

			parent_process_id = process_entry.th32ParentProcessID;

			return parent_process_id;
		}

		process_entry.dwSize = sizeof(process_entry);

		os_status = Process32Next( toolhelp_handle, &process_entry );

		last_error_code = GetLastError();
#if 1
		MX_DEBUG(-2,
		("Process32Next(): os_status = %d, last_error_code = %lu",
		 	(int) os_status, last_error_code));
#endif

		if ( os_status != 0 ) {
			CloseHandle( toolhelp_handle );

			if ( last_error_code == ERROR_NO_MORE_FILES ) {
				mx_warning(
			    "%s: The requested process id %lu was not found.",
				fname, process_id );
			} else {
				(void) mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
				"The attempt to call Process32Next() failed "
				"with Win32 error code %lu", last_error_code );
			}

			return 0;
		}

	}

	return 0;
}

#elif ( defined(OS_UNIX) || defined(OS_ANDROID) ) || defined(OS_CYGWIN)

MX_EXPORT unsigned long
mx_get_parent_process_id( unsigned long process_id )
{
	unsigned long parent_process_id = getppid();

	return parent_process_id;
}

#else

#error mx_get_parent_process_id() is not yet implemented for this platform.

MX_EXPORT unsigned long
mx_get_parent_process_id( unsigned long process_id )
{
	return (unsigned long) ( -1L );
}

#endif

/*=========================================================================*/

#if defined(OS_LINUX) || defined(OS_ANDROID)

MX_EXPORT mx_status_type
mx_get_process_name_from_process_id( unsigned long process_id,
					char *name_buffer,
					size_t name_buffer_length )
{
	static const char fname[] = "mx_get_process_name_from_process_id()";

	FILE *proc_file = NULL;
	char proc_file_name[ MXU_FILENAME_LENGTH+1 ];

	if ( ( name_buffer == (char *) NULL )
	  || ( name_buffer_length == 0 ) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No process name buffer was passed to this function." );
	}

	snprintf( proc_file_name, sizeof(proc_file_name),
		"/proc/%lu/cmdline", process_id );

	proc_file = fopen( proc_file_name, "r" );

	if ( proc_file == NULL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Could not open the proc_file_name '%s'.",
			proc_file_name );
	}

	mx_fgets( name_buffer, name_buffer_length, proc_file );

	fclose( proc_file );

	return MX_SUCCESSFUL_RESULT;
}

#elif defined(OS_MACOSX)

#include <libproc.h>

MX_EXPORT mx_status_type
mx_get_process_name_from_process_id( unsigned long process_id,
					char *name_buffer,
					size_t name_buffer_length )
{
	static const char fname[] = "mx_get_process_name_from_process_id()";

	int os_status, saved_errno;

	if ( ( name_buffer == (char *) NULL )
	  || ( name_buffer_length == 0 ) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No process name buffer was passed to this function." );
	}

	os_status = proc_pidpath( process_id, name_buffer, name_buffer_length );

	if ( os_status <= 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The call to proc_pidpath() failed with errno = %d, "
		"error message = '%s'.", saved_errno, strerror(saved_errno));
	}

	return MX_SUCCESSFUL_RESULT;
}

#elif 0

/* Parse the output of 'ps -p <pid>' if nothing else is available. */

MX_EXPORT mx_status_type
mx_get_process_name_from_process_id( unsigned long process_id,
					char *name_buffer,
					size_t name_buffer_length )
{
	static const char fname[] = "mx_get_process_name_from_process_id()";

	FILE *file = NULL;
	char command[40], response[80];
	int saved_errno, argc;
	char **argv;
	unsigned long process_id_for_line;

	if ( ( name_buffer == (char *) NULL )
	  || ( name_buffer_length == 0 ) )
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No process name buffer was passed to this function." );
	}

	snprintf( command, sizeof(command), "ps -p %lu", process_id );

	file = popen( command, "r" );

	if ( file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to run the command '%s'.  "
		"Errno = %d, error message = '%s'.",
			command, saved_errno, strerror(saved_errno) );
	}

	while( TRUE ) {
		mx_fgets( response, sizeof(response), file );

		if ( feof( file ) ) {
			fclose( file );

			return mx_error( MXE_NOT_FOUND, fname,
			"Pid %lu was not found on this computer.",
				process_id );
		}
		if ( ferror( file ) ) {
			saved_errno = errno;

			fclose( file );

			return mx_error( MXE_IPC_IO_ERROR, fname,
			"An error occurred while reading the output "
			"from the command '%s' for process id %lu.  "
			"Errno = %d, error message = '%s'.",
				command, process_id,
				saved_errno, strerror( saved_errno ) );
		}

		/* FIXME: The output of 'ps' for different
		 *        operating systems may vary.
		 */

		mx_string_split( response, " ", &argc, &argv );

		if ( argc < 4 ) {
			mx_free( argv );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Too few values were received in the response '%s' "
			"to the command '%s' for process id %lu.",
				response, command, process_id );
		}

		process_id_for_line = atol( argv[0] );

		if ( process_id_for_line == process_id ) {
			strlcpy( name_buffer, argv[3], name_buffer_length );

			mx_free( argv );

			return MX_SUCCESSFUL_RESULT;
		}

		mx_free( argv );
	}
}

#else

#error mx_get_process_name_from_process_id() is not yet implemented for this platform.

#endif

/*=========================================================================*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

MX_EXPORT mx_status_type
mx_wait_for_process_id( unsigned long process_id,
			long *process_status )
{
	static const char fname[] = "mx_wait_for_process_id()";

	pid_t result;
	int pid_status, saved_errno;

	result = waitpid( process_id, &pid_status, 0 );

	if ( result < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to wait for process ID %lu failed.  "
		"Errno = %d, error message = '%s'",
			process_id, saved_errno, strerror( saved_errno ) );
	}

	if ( process_status != NULL ) {
		*process_status = pid_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

MX_EXPORT mx_status_type
mx_wait_for_process_id( unsigned long process_id,
			long *process_status )
{
	static const char fname[] = "mx_wait_for_process_id()";

	HANDLE process_handle;
	DWORD return_code;
	DWORD last_error_code;
	TCHAR message_buffer[100];
	mx_status_type mx_status;

	/* FIXME: We shouldn't need PROCESS_ALL_ACCESS. */

#if 0
	process_handle = OpenProcess( PROCESS_QUERY_INFORMATION,
					FALSE, process_id );
#else
	process_handle = OpenProcess( PROCESS_ALL_ACCESS,
					FALSE, process_id );
#endif

	if ( process_handle == NULL ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get a process handle for process ID %lu.  "
		"Win32 error code = %ld, error message = '%s'.",
			process_id, last_error_code, message_buffer );
	}

	return_code = WaitForSingleObject( process_handle, INFINITE );

	switch( return_code ) {
	case WAIT_OBJECT_0:	/* The process has ended. */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;

	case WAIT_FAILED:
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		mx_status = mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"WaitForSingleObject() failed for process handle %p, "
			"process ID %lu.  "
			"Win32 error code = %ld, error message = '%s'.",
				process_handle, process_id,
				last_error_code, message_buffer );
		break;

	case WAIT_ABANDONED:
		mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
			"Received mutex WAIT_ABANDONED code from "
			"WaitForSingleObject() for handle %p, which "
			"is supposed to be the process handle for "
			"process ID %lu.  This should never happen.",
				process_handle, process_id );
		break;

	case WAIT_TIMEOUT:
		mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
			"Received WAIT_TIMEOUT code from "
			"WaitForSingleObject() for process handle %p, "
			"process ID %lu, even though the timeout interval "
			"was set to INFINITE.  This should never happen.",
				process_handle, process_id );
		break;

	default:
		mx_status = mx_error( MXE_UNKNOWN_ERROR, fname,
			"Received unexpected return code %lu from "
			"WaitForSingleObject() for process handle %p, "
			"process ID %lu.  This should never happen.",
				return_code, process_handle, process_id );
		break;
	}

	CloseHandle( process_handle );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_VMS)

/* FIXME: The following is a poor solution. */

MX_EXPORT mx_status_type
mx_wait_for_process_id( unsigned long process_id,
			long *process_status )
{
	static const char fname[] = "mx_wait_for_process_id()";

	mx_bool_type process_exists;

	while (1) {
		process_exists = mx_process_id_exists( process_id );

		if ( process_exists == FALSE )
			break;

		mx_msleep(100);
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_DJGPP)

MX_EXPORT mx_status_type
mx_wait_for_process_id( unsigned long process_id,
			long *process_status )
{
	static const char fname[] = "mx_wait_for_process_id()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Multiple processes are not supported on this platform." );
}

/*-------------------------------------------------------------------------*/

#else
#error mx_wait_for_process_id() is not yet implemented for this platform.
#endif

/*=========================================================================*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_ANDROID) \
	|| defined(OS_MINIX) || defined(OS_RTEMS)

static void
sigalrm_abort_handler( int mx_signal )
{
	abort();
}

MX_EXPORT void
mx_abort_after_timeout( double timeout_in_seconds )
{
	signal( SIGALRM, sigalrm_abort_handler );

	alarm( timeout_in_seconds );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_WIN32)

static void
abort_timer_proc( HWND arg1, UINT arg2, void *arg3, DWORD arg4 )
{
	abort();
}

MX_EXPORT void
mx_abort_after_timeout( double timeout_in_seconds )
{
	UINT timeout_in_msec;

	timeout_in_msec = mx_round( 1000.0 * timeout_in_seconds );

	(void) SetTimer( NULL, 0, timeout_in_msec,
			(TIMERPROC) &abort_timer_proc );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_VXWORKS)

MX_EXPORT void
mx_abort_after_timeout( double timeout_in_seconds )
{
	mx_sleep( timeout_in_seconds );

	abort();
}

#else
#error mx_abort_after_timeout() is not yet implemented for this platform.
#endif

