/*
 * Name:    mx_util.c
 *
 * Purpose: Define some of the utility functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include "mx_osdef.h"

#if defined( OS_WIN32 )
#include <windows.h>
#include <winsock.h>
#include <winerror.h>
#include <direct.h>
#endif

#if defined( OS_UNIX ) || defined( OS_CYGWIN )
#include <sys/wait.h>
#include <pwd.h>
#endif

#include "mx_unistd.h"

#include "mx_util.h"
#include "mx_record.h"

/*-------------------------------------------------------------------------*/

#define COPY_FILE_CLEANUP \
		do {                                           \
			if ( existing_fd >= 0 ) {              \
				(void) close( existing_fd );   \
			}                                      \
			if ( new_fd >= 0 ) {                   \
				(void) close( new_fd );        \
			}                                      \
			if ( buffer != (char *) NULL ) {       \
				mx_free( buffer );             \
			}                                      \
		} while(0)

MX_EXPORT mx_status_type
mx_copy_file( char *existing_filename, char *new_filename, int new_file_mode )
{
	static const char fname[] = "mx_copy_file()";

	int existing_fd, new_fd, saved_errno;
	struct stat stat_struct;
	unsigned long existing_file_size, new_file_size, file_blocksize;
	unsigned long bytes_already_written, bytes_to_write;
	long bytes_read, bytes_written;
	char *buffer;

	existing_fd = -1;
	new_fd      = -1;
	buffer      = NULL;

	/* Open the existing file. */

	existing_fd = open( existing_filename, O_RDONLY, 0 );

	if ( existing_fd < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The file '%s' could not be opened.  Reason = '%s'",
			existing_filename, strerror( saved_errno ) );
	}

	/* Get information about the existing file. */

	if ( fstat( existing_fd, &stat_struct ) != 0 ) {
		saved_errno = errno;

		COPY_FILE_CLEANUP;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Could not fstat() the file '%s'.  Reason = '%s'",
			existing_filename, strerror( saved_errno ) );
	}

	existing_file_size = (unsigned long) stat_struct.st_size;

#if defined( OS_WIN32 )
	file_blocksize = 4096;	/* FIXME: This is just a guess. */
#elif defined( OS_VMS )
	file_blocksize = 512;	/* FIXME: Is this correct for all VMS disks? */
#else
	file_blocksize = stat_struct.st_blksize;
#endif

	/* Allocate a buffer to read the old file contents into. */

	buffer = (char *) malloc( file_blocksize * sizeof( char ) );

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an %ld byte file copy buffer.",
			file_blocksize * sizeof(char) );
	}

	/* Open the new file. */

	new_fd = open( new_filename, O_WRONLY | O_CREAT, new_file_mode );

	if ( new_fd < 0 ) {
		saved_errno = errno;

		COPY_FILE_CLEANUP;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"New file '%s' could not be opened.  Reason = '%s'",
			new_filename, strerror( saved_errno ) );
	}

	/* Copy the contents of the existing file to the new file. */

	new_file_size = 0;

	while( 1 ) {
		bytes_read = read( existing_fd, buffer, file_blocksize );

		if ( bytes_read < 0 ) {
			saved_errno = errno;

			COPY_FILE_CLEANUP;

			return mx_error( MXE_FILE_IO_ERROR, fname,
	"An error occurred while reading the existing file '%s', so the "
	"new file '%s' is probably incomplete.  Error = '%s'.",
				existing_filename, new_filename,
				strerror( saved_errno ) );
		} else
		if ( bytes_read == 0 ) {
			/* This means end of file. */

			COPY_FILE_CLEANUP;

			break;	/* Exit the while() loop. */
		}

		bytes_already_written = 0;

		while ( bytes_already_written < bytes_read ) {

			bytes_to_write = bytes_read - bytes_already_written;

			bytes_written = write( new_fd,
				    &buffer[ bytes_already_written ],
				    bytes_to_write );

			if ( bytes_written < 0 ) {
				saved_errno = errno;

				COPY_FILE_CLEANUP;

				return mx_error( MXE_FILE_IO_ERROR, fname,
	"An error occurred while writing the new file '%s', so the "
	"new file is probably incomplete.  Error = '%s'",
					new_filename,
					strerror( saved_errno ) );
			} else
			if ( bytes_written == 0 ) {
				return mx_error( MXE_FILE_IO_ERROR, fname,
	"An attempt to write %ld bytes to the new file '%s' "
	"resulted in 0 bytes being written.  This should not happen, "
	"but it is not obvious why it happened.  Please report this as a bug.",
					bytes_to_write,
					new_filename );
			} else
			if ( bytes_written > bytes_to_write ) {
				return mx_error(
					MXE_OPERATING_SYSTEM_ERROR, fname,
	"Somehow the number of bytes written %ld to file '%s' "
	"is greater than the number "
	"of bytes (%ld) that we asked to write.  This should not happen, "
	"but it is not obvious why it happened.  Please report this as a bug.",
					bytes_written,
					new_filename,
					bytes_to_write );
			}

			bytes_already_written += bytes_written;
		}

		new_file_size += bytes_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 ) && !defined( __BORLANDC__ )
#define getcwd _getcwd
#endif

MX_EXPORT mx_status_type
mx_get_current_directory_name( char *filename_buffer,
				size_t max_filename_length )
{
	static const char fname[] = "mx_get_current_directory_name()";

	char *ptr;
	int saved_errno;

	/* Some versions of getcwd() will malloc() a buffer if filename_buffer
	 * is NULL.  However, since mx_get_current_directory_name() is 
	 * prototyped to return an mx_status_type structure, we cannot permit
	 * mallocing() of a buffer since there is no way to return the buffer
	 * pointer to the caller.
	 */

	if ( filename_buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename_buffer pointer passed was NULL." );
	}

	ptr = getcwd( filename_buffer, max_filename_length );

	if ( ptr == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while trying to get the name of the current "
		"directory.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#if defined( OS_UNIX )

MX_EXPORT int
mx_process_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_exists()";

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
			break;
		case EPERM:
			return TRUE;
			break;
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
mx_kill_process( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process()";

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
			return mx_error_quiet( MXE_NOT_FOUND, fname,
				"Process %lu does not exist.", process_id);
			break;
		case EPERM:
			return mx_error_quiet( MXE_PERMISSION_DENIED, fname,
				"Cannot send the signal to process %lu.",
					process_id );
			break;
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

#else  /* Not OS_UNIX */

MX_EXPORT int
mx_process_exists( unsigned long process_id )
{
	static const char fname[] = "mx_process_exists()";

	(void) mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented for this operating system.",
			fname );

	return FALSE;
}

MX_EXPORT mx_status_type
mx_kill_process( unsigned long process_id )
{
	static const char fname[] = "mx_kill_process()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented for this operating system.",
			fname );
}

#endif /* OS_UNIX */

/*-------------------------------------------------------------------------*/

MX_EXPORT char *
mx_username( char *buffer, size_t max_buffer_length )
{
#if defined( OS_WIN32 ) || defined( OS_LINUX ) || defined( OS_HPUX )
	static const char fname[] = "mx_username()";
#endif

	static char local_buffer[ MXU_USERNAME_LENGTH + 1 ];
	char *ptr;

	if ( buffer == NULL ) {
		ptr = local_buffer;
		max_buffer_length = MXU_USERNAME_LENGTH;
	} else {
		ptr = buffer;
	}

	/* Getting the username varies from OS to OS, so I will fill this in
	 * as time goes on.  Note that the various versions of Unix do not
	 * seem to be able to agree with each other on how to reentrantly
	 * extend getpwuid().
	 */

#if defined( OS_LINUX ) || defined( OS_HPUX )
	{
		char scratch_buffer[ 512 ];
		struct passwd pw_buffer, *pw;
		uid_t uid;
		int status, saved_errno;

		pw = NULL;

		uid = getuid();

#if defined( OS_LINUX )
		status = getpwuid_r( uid, &pw_buffer, scratch_buffer,
					sizeof(scratch_buffer), &pw );
#elif defined( OS_SOLARIS )
		/* FIXME: This doesn't work for some reason. */

		MX_DEBUG(-2,("%s: before getpwuid_r()", fname));

		pw = getpwuid_r( uid, &pw_buffer, scratch_buffer,
					sizeof(scratch_buffer) );

		if ( pw == NULL ) {
			status = -1;
		} else {
			status = 0;
		}

		MX_DEBUG(-2,("%s: pw = , status = %d",
			fname, status));
#elif defined( OS_HPUX )
		status = getpwuid_r( uid, &pw_buffer, scratch_buffer,
					sizeof(scratch_buffer) );

		pw = &pw_buffer;
#endif

		if ( status != 0 ) {
			saved_errno = errno;

			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Cannot determine user name for UID %lu, setting it to 'unknown'.  "
	"Error code = %d, error text = '%s'", (unsigned long) uid,
				saved_errno, strerror( saved_errno ) );

			mx_strncpy( ptr, "unknown", max_buffer_length );

			return ptr;
		}
		mx_strncpy( ptr, pw->pw_name, max_buffer_length );
	}

/* End of OS_LINUX, OS_SOLARIS, and OS_HPUX section. */

#elif defined( OS_IRIX ) || defined( OS_SUNOS4 ) || defined( OS_SOLARIS ) \
   || defined( OS_MACOSX ) || defined( OS_BSD ) || defined( OS_QNX )
	{
		/* This method is not reentrant. */

		struct passwd *pw;

		pw = getpwuid( getuid() );

		if ( pw == NULL ) {
			mx_strncpy( ptr, "unknown", max_buffer_length );

			return ptr;
		}
		mx_strncpy( ptr, pw->pw_name, max_buffer_length );
	}
#elif defined( OS_WIN32 )
	{
		TCHAR message_buffer[MXU_ERROR_MESSAGE_LENGTH - 120];
		DWORD dword_max_buffer_length;
		DWORD last_error_code;
		BOOL status;

		dword_max_buffer_length = max_buffer_length;

		/* I don't yet know which of the following two methods
		 * is better.
		 */
#if 0
		status = WNetGetUser( NULL, ptr, &dword_max_buffer_length );

		if ( status != NO_ERROR ) {
#else
		status = GetUserName( ptr, &dword_max_buffer_length );

		if ( status == 0 ) {
#endif
			last_error_code = GetLastError();

			switch ( last_error_code ) {
			case ERROR_NOT_LOGGED_ON:
				(void) mx_error( MXE_FUNCTION_FAILED, fname,
				"The current user is not logged into the "
				"network.  Username = 'unknown'." );

				break;
			default:
				mx_win32_error_message( last_error_code,
				    message_buffer, sizeof(message_buffer) );

				(void) mx_error( MXE_FUNCTION_FAILED, fname,
				"Cannot get username for the current thread.  "
				"Win32 error code = %ld, error message = '%s'",
				    last_error_code, message_buffer );

				break;
			}
			mx_strncpy( ptr, "unknown", max_buffer_length );
		}
	}

#elif defined( OS_CYGWIN ) || defined( OS_DJGPP ) || defined( OS_VMS )
	{
		char *login_ptr;

		login_ptr = getlogin();
 
		if ( login_ptr == NULL ) {
			mx_strncpy( ptr, "unknown", max_buffer_length );
		} else {
			mx_strncpy( ptr, login_ptr, max_buffer_length );
#if defined( OS_VMS )
			/* Convert the VMS username to lower case. */

			{
				int i, c;

				for ( i = 0; i < strlen( ptr ); i++ ) {
					c = ptr[i];

					if ( isupper(c) ) {
						ptr[i] = tolower(c);
					}
				}
			}
#endif
		}
	}

#elif defined( OS_RTEMS )

	mx_strncpy( ptr, "rtems", max_buffer_length );

#elif defined( OS_VXWORKS )

	mx_strncpy( ptr, "vxworks", max_buffer_length );

#else

#error mx_username() has not been implemented for this platform.

#endif
	return ptr;
}

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
		_exit(1);
	}

	recursion++;

	switch ( signal_number ) {
#ifdef SIGILL
	case SIGILL:
		strcpy( signal_name, "SIGILL (illegal instruction)" );
		break;
#endif
#ifdef SIGTRAP
	case SIGTRAP:
		strcpy( signal_name, "SIGTRAP" );
		break;
#endif
#ifdef SIGIOT
	case SIGIOT:
		strcpy( signal_name, "SIGIOT (I/O trap)" );
		break;
#endif
#ifdef SIGBUS
	case SIGBUS:
		strcpy( signal_name, "SIGBUS (bus error)" );
		break;
#endif
#ifdef SIGFPE
	case SIGFPE:
		strcpy( signal_name, "SIGFPE (floating point exception)" );
		break;
#endif
#ifdef SIGSEGV
	case SIGSEGV:
		strcpy( signal_name, "SIGSEGV (segmentation violation)" );
		break;
#endif
	default:
		sprintf( signal_name, "%d", signal_number );
		break;
	}

	mx_info( "CRASH: This program has died due to signal %s.\n",
			signal_name );

	mx_info( "Process id = %lu", mx_process_id() );

	/* Print out the stack traceback. */

	mx_stack_traceback();

	/* Try to force a core dump. */

	mx_info("Attempting to force a core dump in '%s'.", directory_name );

	mx_force_core_dump();
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

MX_EXPORT void
mx_start_debugger( char *command )
{
#if defined(OS_WIN32)
	fprintf(stderr,
	    "\nWarning: The Visual C++ debugger is being started.\n\n");
	fflush(stderr);

	DebugBreak();

#elif defined(OS_LINUX)
	char command_line[80];
	unsigned long pid;

	pid = mx_process_id();

	/* If no command line is supplied, we supply a default one with gdb. */

	if ( command == NULL ) {
		sprintf( command_line, "gdb -p %lu", pid );
	} else {
		strncpy( command_line, command, sizeof(command_line)-1 );
	}

	fprintf(stderr,
"\nWarning: The program is now waiting in an infinite loop for you to\n" );
	fprintf(stderr,
"         start debugging it with GDB via the command '%s'.\n\n",
						command_line );
	fprintf(stderr,
"         After attaching, type the command 'set loop=0' to break out\n" );
	fprintf(stderr,
"         of the loop.\n\n" );
	fprintf(stderr,
"         Waiting...\n\n" );


	/* Synchronize with the debugger by waiting for the debugger
	 * to reset the value of 'loop' to 0.
	 */

	{
		volatile int loop;

		loop = 1;

		while ( loop ) {
			mx_msleep(1000);
		}
	}

#else
	fprintf(stderr, "\n"
"Warning: Starting a debugger is not currently supported on this platform.\n"
"         The request to start a debugger will be ignored.  Sorry...\n\n" );
	fflush(stderr);
#endif
	return;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT int
mx_get_max_file_descriptors( void )
{
	int result;

	/* This operation varies from OS to OS, so I will fill this in as time
	 * goes on.
	 */

#if defined( OS_LINUX ) || defined( OS_SOLARIS ) || defined( OS_IRIX ) \
	|| defined( OS_HPUX ) || defined( OS_SUNOS4 ) || defined( OS_MACOSX ) \
	|| defined( OS_BSD ) || defined( OS_QNX ) || defined( OS_CYGWIN ) \
	|| defined( OS_DJGPP ) || defined( OS_VMS )

	result = getdtablesize();

#elif defined( OS_WIN32 )

	result = FD_SETSIZE;

#elif defined( OS_RTEMS ) || defined( OS_VXWORKS )

	result = 16;		/* A wild guess and probably wrong. */

#else

#error mx_get_max_file_descriptors() has not been implemented for this platform.

#endif

	return result;
}

MX_EXPORT unsigned long
mx_hex_char_to_unsigned_long( char c )
{
	static const char fname[] = "mx_hex_char_to_unsigned_long()";

	char buffer[2];
	char *endptr;
	unsigned long result;

	buffer[0] = c;
	buffer[1] = '\0';

	result = strtoul( buffer, &endptr, 16 );

	if ( *endptr != '\0' ) {
		result = 0;

		mx_warning(
		"%s: Supplied character %#x '%c' is not a hexadecimal digit.  "
		"Result set to zero.", fname, c, c );
	}
	return result;
}

MX_EXPORT unsigned long
mx_hex_string_to_unsigned_long( char *string )
{
	static const char fname[] = "mx_hex_string_to_unsigned_long()";

	char *endptr;
	unsigned long result;

	/* Skip over any leading whitespace. */

	string += strspn( string, MX_WHITESPACE );

	/* Parse the string. */

	result = strtoul( string, &endptr, 16 );

	if ( *endptr != '\0' ) {
		result = 0;

		mx_warning(
		"%s: Supplied string '%s' is not a hexadecimal number.  "
		"Result set to zero.", fname, string );
	}
	return result;
}

MX_EXPORT long
mx_string_to_long( char *string )
{
	static const char fname[] = "mx_hex_string_to_unsigned_long()";

	char *endptr;
	long result;

	result = strtol( string, &endptr, 0 );

	if ( *endptr != '\0' ) {
		result = 0;

		mx_warning(
		"%s: Supplied string '%s' is not a number.  "
		"Result set to zero.", fname, string );
	}
	return result;
}

MX_EXPORT unsigned long
mx_string_to_unsigned_long( char *string )
{
	static const char fname[] = "mx_hex_string_to_unsigned_long()";

	char *endptr;
	unsigned long result;

	result = strtoul( string, &endptr, 0 );

	if ( *endptr != '\0' ) {
		result = 0;

		mx_warning(
		"%s: Supplied string '%s' is not a number.  "
		"Result set to zero.", fname, string );
	}
	return result;
}

MX_EXPORT long
mx_round( double value )
{
	long result;

	if ( value >= 0.0 ) {
		result = (long) ( 0.5 + value );
	} else {
		result = (long) ( -0.5 + value );
	}

	return result;
}

MX_EXPORT long
mx_round_away_from_zero( double value, double threshold )
{
	long result;

	if ( value >= 0.0 ) {
		result = (long) ( value + 1.0 - threshold );
	} else {
		result = (long) ( value - 1.0 + threshold );
	}

	return result;
}

MX_EXPORT long
mx_round_toward_zero( double value, double threshold )
{
	long result;

	if ( value >= 0.0 ) {
		result = (long) ( value + threshold );
	} else {
		result = (long) ( value - threshold );
	}

	return result;
}

/*
 * mx_multiply_safely multiplies two floating point numbers by each other
 * while testing for and avoiding infinities.
 */

#define MX_WARN_ABOUT_INFINITY		FALSE

MX_EXPORT double
mx_multiply_safely( double multiplier1, double multiplier2 )
{
	double result;

	if ( ( fabs(multiplier1) <= 1.0 ) || ( fabs(multiplier2) <= 1.0 ) ) {
		result = multiplier1 * multiplier2;
	} else {
		if ( fabs(multiplier1 / DBL_MAX) < fabs(1.0 / multiplier2) ) {
			result = multiplier1 * multiplier2;
		} else {
			if ( (multiplier1 >= 0.0 && multiplier2 >= 0.0)
			  || (multiplier1 < 0.0 && multiplier2 < 0.0 ) )
			{
				result = DBL_MAX;
			} else {
				result = -DBL_MAX;
			}
#if MX_WARN_ABOUT_INFINITY
			mx_warning(
				"*** mx_multiply_safely(): Floating point "
				"overflow avoided for multiplier %g and "
				"multiplier %g.  Result set to %g. ***",
				multiplier1, multiplier2, result );
#endif
		}
	}
	return result;
}

/*
 * mx_divide_safely divides two floating point numbers by each other
 * while testing for and avoiding division by zero.
 */

#define MX_WARN_ABOUT_DIVISION_BY_ZERO	FALSE

MX_EXPORT double
mx_divide_safely( double numerator, double denominator )
{
	double result;

	if ( fabs(denominator) >= 1.0 ) {
		result = numerator / denominator;
	} else {
		if ( fabs(numerator) < fabs(denominator * DBL_MAX) ) {
			result = numerator / denominator;
		} else {
			if ( (numerator >= 0.0 && denominator >= 0.0)
			  || (numerator < 0.0 && denominator < 0.0) )
			{
				result = DBL_MAX;
			} else {
				result = -DBL_MAX;
			}
#if MX_WARN_ABOUT_DIVISION_BY_ZERO
			mx_warning(
				"*** mx_divide_safely(): Division by zero "
				"avoided for numerator %g and denominator %g."
				"  Result set to %g. ***",
				numerator, denominator, result );
#endif
		}
	}
	return result;
}

/* --- */

#define ARRAY_BLOCK_SIZE	100

static int
mxp_add_token_to_array( char *token_ptr,
			int *argc, char ***argv,
			int *envc, char ***envp,
			int in_quoted_string )
{
	int is_env;

	if ( in_quoted_string ) {
		is_env = FALSE;
	} else {
		if ( strchr( token_ptr, '=' ) != NULL ) {
			is_env = TRUE;
		} else {
			is_env = FALSE;
		}
	}

	if ( is_env ) {
		if ( *envp == NULL ) {
			/* The first time through we must allocate the
			 * initial block.
			 */

			*envp = ( char ** ) malloc( sizeof( char * )
						* ( 1 + ARRAY_BLOCK_SIZE ) );
		} else {
			if ( ( (*envc + 1) % ARRAY_BLOCK_SIZE ) == 0 ) {

				/* We must increase the size of the array. */

				*envp = ( char ** ) realloc( *envp,
					sizeof( char * )
					    * ( *envc + 1 + ARRAY_BLOCK_SIZE ));
			}
		}

		if ( *envp == NULL ) {
			errno = ENOMEM;

			return (-1);
		}

		(*envp)[*envc] = token_ptr;

		(*envc)++;
	} else {
		if ( *argv == NULL ) {
			/* The first time through we must allocate the
			 * initial block.
			 */

			*argv = ( char ** ) malloc( sizeof( char * )
						* ( 1 + ARRAY_BLOCK_SIZE ) );
		} else {
			if ( ( (*argc + 1) % ARRAY_BLOCK_SIZE ) == 0 ) {

				/* We must increase the size of the array. */

				*argv = ( char ** ) realloc( *argv,
					sizeof( char * )
					    * ( *argc + 1 + ARRAY_BLOCK_SIZE ));
			}
		}

		if ( *argv == NULL ) {
			errno = ENOMEM;

			return (-1);
		}

		(*argv)[*argc] = token_ptr;

		(*argc)++;
	}

	return 0;
}

MX_EXPORT int
mx_parse_command_line( char *command_line,
			int *argc, char ***argv,
			int *envc, char ***envp )
{
	char *ptr, *token_ptr;
	int in_quoted_string;
	int status;

	if ( ( command_line == NULL )
	  || ( argc == NULL )
	  || ( argv == NULL )
	  || ( envc == NULL )
	  || ( envp == NULL ) )
	{
		errno = EINVAL;

		return (-1);
	}

	*argc = 0;
	*envc = 0;
	*argv = NULL;
	*envp = NULL;

	/* If token_ptr is not NULL, then we are in the middle of a token. */

	token_ptr = NULL;
	in_quoted_string = FALSE;

	for ( ptr = command_line; ; ptr++ ) {

		switch( *ptr ) {
		case '\0':
			/* We have reached the end of the command line
			 * so prepare to return now.
			 */

			if ( token_ptr != NULL ) {
				status = mxp_add_token_to_array( token_ptr,
						argc, argv, envc, envp,
						in_quoted_string );

				if ( status != 0 )
					return status;
			}

			/* The last elements of the argv and envp arrays
			 * must be NULL.
			 */

			if ( *argc == 0 ) {
				mx_warning(
				"An empty command line was specified.");

				*argv = (char **) malloc( sizeof(char *) );
			}

			if ( *envc == 0 ) {
				mx_warning(
				"No environment variables were specified.");

				*envp = (char **) malloc( sizeof(char *) );
			}

			(*argv)[*argc] = NULL;
			(*envp)[*envc] = NULL;

			return 0;
			break;

		case ' ':
		case '\t':
			if ( token_ptr != NULL ) {
				if ( in_quoted_string == FALSE ) {
					/* We are at the end of a token. */

					*ptr = '\0';

					status = mxp_add_token_to_array(
						token_ptr,
						argc, argv, envc, envp,
						in_quoted_string );

					if ( status != 0 )
						return status;

					token_ptr = NULL;
				}
			}
			break;

		case '"':
			if ( token_ptr != NULL ) {
				/* We are at the end of a token. */

				*ptr = '\0';

				status = mxp_add_token_to_array( token_ptr,
						argc, argv, envc, envp,
						in_quoted_string );

				if ( status != 0 )
					return status;

				in_quoted_string = FALSE;
				token_ptr = NULL;
			} else {
				/* We are at the start of a quoted string
				 * token.  Skip over the quotation mark
				 * to point to the start of the token.
				 */

				in_quoted_string = TRUE;

				ptr++;
				token_ptr = ptr;
			}
			break;

		default:
			if ( token_ptr == NULL ) {
				/* We are at the start of a normal token. */

				token_ptr = ptr;
			}
			break;
		}
	}

#if !defined(OS_SOLARIS)

	/* It should not be possible for us to get here. */

	errno = ERANGE;

	return (-1);
#endif
}

/* --- */

/* FIXME: The following function probably should not be using tmpnam(),
 *        but it is currently only used by the XIA multichannel analyzer
 *        support and only if that is configured in a non-standard way,
 *        so the damage is limited for now.  (WML)
 */

MX_EXPORT FILE *
mx_open_temporary_file( char *returned_temporary_name,
			size_t maximum_name_length )
{
	FILE *temp_file;
	int temp_fd;

	if ( maximum_name_length < L_tmpnam ) {
		errno = ENAMETOOLONG;

		return NULL;
	}

	/* FIXME: There is a potential for a race condition in the following.
	 *        The filename returned by tmpnam() may be in use by another
	 *        file by the time we invoke open().  However, this method
	 *        is somewhat mitigated by the fact that the open() will
	 *        fail if the file already exists due to the inclusion of
	 *        the O_EXCL flag in the call.  This method also has the
	 *        virtue of being fairly portable.
	 */

	if ( tmpnam( returned_temporary_name ) == NULL ) {
		errno = EINVAL;

		return NULL;
	}

#if defined( OS_WIN32 )
#   if defined( __BORLANDC__ )
	temp_fd = _open( returned_temporary_name,
			_O_CREAT | _O_TRUNC | _O_EXCL | _O_RDWR );
#   else
	temp_fd = _open( returned_temporary_name,
			_O_CREAT | _O_TRUNC | _O_EXCL | _O_RDWR,
			_S_IREAD | _S_IWRITE );
#   endif
#elif defined( OS_VXWORKS )
	temp_fd = -1;	/* FIXME: VxWorks doesn't have S_IREAD or S_IWRITE. */
#else
	temp_fd = open( returned_temporary_name,
			O_CREAT | O_TRUNC | O_EXCL | O_RDWR,
			S_IREAD | S_IWRITE );
#endif

	/* If temp_fd is set to -1, an error has occurred during the open.
	 * The open function will already have set errno to an appropriate
	 * value.
	 */

	if ( temp_fd == -1 ) {
		return NULL;
	}

	temp_file = fdopen( temp_fd, "w+" );

	return temp_file;
}

/* --- */

MX_EXPORT int
mx_free_pointer( void *pointer )
{
	static const char fname[] = "mx_free_pointer()";

	if ( pointer == NULL ) {
		return TRUE;
	}

	if ( mx_is_valid_heap_pointer( pointer ) ) {
		free( pointer );
		return TRUE;
	} else {
		mx_warning(
			"Pointer %p passed to %s is not a valid heap pointer.",
			pointer, fname );
#if 0
		mx_start_debugger(NULL);
#endif

		return FALSE;
	}
}

/* --- */

#if defined(OS_WIN32)

/* The Win32 functions _snprintf() and _vsnprintf() do not null terminate
 * the strings they return if the output was truncated.  This is inconsitent
 * with the Unix definition of these functions, so we cannot simply define
 * snprintf() as _snprintf().  Instead, we provide wrapper functions that
 * make sure that the output string is always terminated.  Thanks to the
 * authors of http://www.di-mgt.com.au/cprog.html for pointing this out.
 */

MX_EXPORT int
snprintf( char *dest, size_t maxlen, const char *format, ... )
{
	va_list args;
	int result;

	va_start( args, format );

	result = _vsnprintf( dest, maxlen-1, format, args );

	if ( result < 0 ) {
		dest[ maxlen-1 ] = '\0';
	}

	va_end( args );

	return result;
}

MX_EXPORT int
vsnprintf( char *dest, size_t maxlen, const char *format, va_list args  )
{
	int result;

	result = _vsnprintf( dest, maxlen-1, format, args );

	if ( result < 0 ) {
		dest[ maxlen-1 ] = '\0';
	}

	return result;
}

#endif

/* --- */

MX_EXPORT char *
mx_strappend( char *dest, char *src, size_t buffer_length )
{
	size_t string_length, buffer_left;

	string_length = strlen( dest );

	buffer_left = buffer_length - string_length - 1;

	/* Only append characters if there is room left in the buffer
	 * to add them.
	 */

	if ( buffer_left > 0 ) {
		strncat( dest, src, buffer_left );
	}

	return dest;
}

MX_EXPORT char *
mx_ctime_string( void )
{
	char *ptr;
	size_t length;
	time_t seconds_since_1970_started;

	seconds_since_1970_started = time(NULL);

	ptr = ctime( &seconds_since_1970_started );

	/* Zap the newline at the end of the string returned by ctime(). */

	length = strlen(ptr);

	ptr[length - 1] = '\0';

	return ptr;
}

MX_EXPORT char *
mx_current_time_string( char *buffer, size_t buffer_length )
{
	time_t time_struct;
	struct tm *current_time;
	static char local_buffer[200];
	char *ptr;

	if ( buffer == NULL ) {
		ptr = &local_buffer[0];
		buffer_length = sizeof(local_buffer);
	} else {
		ptr = buffer;
	}

	time( &time_struct );

	current_time = localtime( &time_struct );

	strftime( ptr, buffer_length,
		"%a %b %d %Y %H:%M:%S", current_time );

	return ptr;
}

MX_EXPORT char *
mx_skip_string_fields( char *buffer, int num_fields )
{
	char *ptr;
	int num_skipped;
	size_t segment_length;

	ptr = buffer;

	/* Skip over any leading whitespace. */

	segment_length = strspn( ptr, MX_RECORD_FIELD_SEPARATORS );

	ptr += segment_length;

	if ( *ptr == '\0' )
		return ptr;

	/* Now loop over string fields. */

	num_skipped = 0;

	while ( num_skipped < num_fields ) {

		/* Skip the string. */

		segment_length = strcspn( ptr, MX_RECORD_FIELD_SEPARATORS );

		ptr += segment_length;

		num_skipped++;

		if ( *ptr == '\0' )
			return ptr;

		/* Skip the trailing whitespace. */

		segment_length = strspn( ptr, MX_RECORD_FIELD_SEPARATORS );

		ptr += segment_length;

		if ( *ptr == '\0' )
			return ptr;
	}

	return ptr;
}

MX_EXPORT char *
mx_string_split( char **string_ptr, const char *delim )
{
	char *ptr, *start_ptr, *strchr_ptr;

	if ( string_ptr == NULL )
		return NULL;

	ptr = *string_ptr;

	if ( ptr == NULL )
		return NULL;

	start_ptr = NULL;

	/* Find the start of the token. */

	while ( (*ptr) != '\0' ) {
		strchr_ptr = strchr( delim, *ptr );

		if ( strchr_ptr == NULL ) {
			start_ptr = ptr;
			break;			/* Exit the while() loop. */
		} else {
			*ptr = '\0';
		}

		ptr++;
	}

	/* If we did not find the start of a token, all we can do is
	 * quit and return NULL.
	 */

	if ( start_ptr == NULL ) {
		*string_ptr = NULL;

		return NULL;
	}

	/* Find the end of the token. */

	while ( (*ptr) != '\0' ) {

		strchr_ptr = strchr( delim, *ptr );

		if ( strchr_ptr != NULL ) {
			/* We have found the end of the token, so
			 * null out any trailing delimiters and 
			 * then return.
			 */

			while ( strchr_ptr != NULL ) {
				*ptr = '\0';

				ptr++;

				/* Note that if (*ptr) == '\0', then strchr() 
				 * will return NULL.
				 */

				strchr_ptr = strchr( delim, *ptr );
			}

			*string_ptr = ptr;

			return start_ptr;
		}

		ptr++;
	}

	/* If we get here, the token ended at the end of the original string,
	 * so we just return the start of token pointer.  Also, set *string_ptr
	 * to NULL to indicate that we have parsed all of the string.
	 */

	*string_ptr = NULL;

	return start_ptr;
}

#if defined(OS_VXWORKS)

/* If the current platform does not have an access() function, then we
 * implement one using stat().
 */

int
access( char *pathname, int mode )
{
	struct stat status_struct;
	mode_t file_mode;
	int status;

	status = stat( pathname, &status_struct );

	if ( status != 0 )
		return status;

	/* If we are just checking to see if the file exists and
	 * stat() succeeded, then the file must exist and we can
	 * return now.
	 */

	if ( mode == F_OK )
		return 0;

	/* Select out the bits from the st_mode field that we need
	 * to compare with the 'mode' argument to this function.
	 */

#if defined(OS_VXWORKS)
	/* Only look at the world permissions. */

	file_mode = 0x7 & status_struct.st_mode;
#else
	if ( getuid() == status_struct.st_uid ) {

		/* We need to check the owner permissions. */

		file_mode = 0x7 & ( status_struct.st_mode >> 8 );

	} else if ( getgid() == status_struct.st_gid ) {

		/* We need to check the group permissions. */

		file_mode = 0x7 & ( status_struct.st_mode >> 4 );

	} else {

		/* We need to check the world permissions. */

		file_mode = 0x7 & status_struct.st_mode;
	}
#endif

	if ( file_mode == mode ) {

		/* Success: the modes match. */

		return 0;
	} else {
		/* The modes do _not_ match. */

		errno = EACCES;

		return -1;
	}
}

#endif
