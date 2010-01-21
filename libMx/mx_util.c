/*
 * Name:    mx_util.c
 *
 * Purpose: Define some of the utility functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include <sys/time.h>
#include <pwd.h>
#endif

#include "mx_util.h"
#include "mx_hrt.h"
#include "mx_unistd.h"
#include "mx_record.h"
#include "mx_signal.h"
#include "mx_atomic.h"

/*-------------------------------------------------------------------------*/

/* mx_initialize_runtime() sets up the parts of the MX runtime environment
 * that do not depend on the presence of an MX database.
 */

static int mx_runtime_is_initialized = FALSE;

MX_EXPORT mx_status_type
mx_initialize_runtime( void )
{
	mx_status_type mx_status;

	/* Only run the initialization code once. */

	if ( mx_runtime_is_initialized ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_runtime_is_initialized = TRUE;

#if defined( OS_WIN32 )
	{
		/* HACK: If a Win32 compiled program is run from
		 * a Cygwin bash shell under rxvt or a remote ssh
		 * session, _isatty() returns 0 and Win32 thinks
		 * that it needs to fully buffer stdout and stderr.
		 * This interferes with timely updates of the 
		 * output from the MX server in such cases.  The
		 * following ugly hack works around this problem.
		 * 
		 * Read
		 *    http://www.khngai.com/emacs/tty.php
		 * or
		 *    http://homepages.tesco.net/~J.deBoynePollard/
		 *		FGA/capture-console-win32.html
		 * or the thread starting at
		 *    http://sources.redhat.com/ml/cygwin/2003-03/msg01325.html
		 *
		 * for more information about this problem.
		 */

		int is_a_tty;

#if defined( __BORLANDC__ )
		is_a_tty = isatty(fileno(stdin));
#else
		is_a_tty = _isatty(fileno(stdin));
#endif

		if ( is_a_tty == 0 ) {
			setvbuf( stdout, (char *) NULL, _IONBF, 0 );
			setvbuf( stderr, (char *) NULL, _IONBF, 0 );
		}
	}
#endif   /* OS_WIN32 */

#if defined( OS_WIN32 ) && defined( __BORLANDC__ )

	/* Borland C++ enables floating point exceptions that are not
	 * enabled by Microsoft's compiler.  Change the floating point
	 * exception mask to mask off those exceptions.
	 */

	_control87( MCW_EM, MCW_EM );
#endif

	/* If we have a child process, like gnuplot, and the process
	 * dies before we expected, then any attempt to write to the
	 * pipe connecting us to the child will result in us being
	 * sent a SIGPIPE signal.  The default response under Unix
	 * to a SIGPIPE is to terminate the process.  This is
	 * undesirable for most MX programs, since it does not give 
	 * MX a chance to recover from the error.  Thus, we arrange to
	 * ignore any SIGPIPE signals sent our way and depend on
	 * 'errno' to let us know what really happened during a
	 * failed write.
	 */

#if defined( SIGPIPE )
	signal( SIGPIPE, SIG_IGN );
#endif

#if defined( _POSIX_REALTIME_SIGNALS )
	mx_status = mx_signal_initialize();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Set the MX debugging level to zero. */

	mx_set_debug_level(0);

	/* Initialize the subsecond sleep functions (if they need it). */

	mx_msleep(1);

	/* Initialize the MX time keeping functions. */

	mx_initialize_clock_ticks();

	/* Initialize atomic operations (if they need it). */

	mx_atomic_initialize();

	/* We are done, so return to the caller. */

	mx_status = MX_SUCCESSFUL_RESULT;

	return mx_status;
}

/*-------------------------------------------------------------------------*/

#if defined( OS_VXWORKS )

MX_EXPORT int
mx_strcasecmp( const char *s1, const char *s2 )
{
	const char *ptr1, *ptr2;
	char c1, c2;

	if ( s1 == NULL ) {
		if ( s2 == NULL ) {
			return 0;
		} else {
			return (-1);
		}
	} else {
		if ( s2 == NULL ) {
			return 1;
		}
	}

	for( ptr1 = s1, ptr2 = s2; ; ptr1++, ptr2++ ) {
		c1 = *ptr1;
		c2 = *ptr2;

		if ( c1 == '\0' ) {
			if ( c2 == '\0' ) {
				return 0;
			} else {
				return (-1);
			}
		} else {
			if ( c2 == '\0' ) {
				return 1;
			}
		}

		if ( isupper(c1) ) {
			c1 = tolower(c1);
		}
		if ( isupper(c2) ) {
			c2 = tolower(c2);
		}

		if ( c1 < c2 ) {
			return (-1);
		} else
		if ( c1 > c2 ) {
			return 1;
		}
	}

	mx_warning("Broke out of for() loop in mx_strncasecmp()");
}

MX_EXPORT int
mx_strncasecmp( const char *s1, const char *s2, size_t n )
{
	const char *ptr1, *ptr2;
	char c1, c2;
	size_t i;

	if ( s1 == NULL ) {
		if ( s2 == NULL ) {
			return 0;
		} else {
			return (-1);
		}
	} else {
		if ( s2 == NULL ) {
			return 1;
		}
	}

	for( i = 0, ptr1 = s1, ptr2 = s2; i < n ; i++, ptr1++, ptr2++ ) {
		c1 = *ptr1;
		c2 = *ptr2;

		if ( c1 == '\0' ) {
			if ( c2 == '\0' ) {
				return 0;
			} else {
				return (-1);
			}
		} else {
			if ( c2 == '\0' ) {
				return 1;
			}
		}

		if ( isupper(c1) ) {
			c1 = tolower(c1);
		}
		if ( isupper(c2) ) {
			c2 = tolower(c2);
		}

		if ( c1 < c2 ) {
			return (-1);
		} else
		if ( c1 > c2 ) {
			return 1;
		}
	}

	return 0;
}

#endif /* OS_VXWORKS */

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 ) && defined( __BORLANDC__ )

	/* Borland C++ enables floating point exceptions that are not
	 * enabled by Microsoft's compiler.  Supply a math exception
	 * handler that ignores these errors.
	 */

int _matherr( struct _exception *e )
{
	return 1;	/* Claim that the error has been handled. */
}
#endif

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

#elif defined( OS_ECOS )
	file_blocksize = 512;	/* FIXME: This is just a guess. */
#else
	file_blocksize = stat_struct.st_blksize;
#endif

	/* Allocate a buffer to read the old file contents into. */

	buffer = (char *) malloc( file_blocksize * sizeof( char ) );

	if ( buffer == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Unable to allocate memory for an %lu byte file copy buffer.",
			(unsigned long) (file_blocksize * sizeof(char)) );
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

	for (;;) {
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
	"An attempt to write %lu bytes to the new file '%s' "
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
	"of bytes (%lu) that we asked to write.  This should not happen, "
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

/* mx_change_filename_prefix() is intended for use in cases where a given
 * system can access a file under two different names and it is necessary
 * to convert between the two names.  For example, suppose that a client
 * program running on host 'ad_client' is communicating with an area
 * detector server running on host 'ad_server'.  Also suppose, that area
 * detector images are saved on 'ad_server' in the directory /data/lavender
 * which is NFS or SMB exported to 'ad_client' as /mnt/ad_server/lavender.
 * In this case, mx_change_filename_prefix() can be used to strip off the
 * leading /data string at the start of the filename and replace it with
 * /mnt/ad_server or vice versa.
 */

MX_EXPORT mx_status_type
mx_change_filename_prefix( char *old_filename,
			char *old_filename_prefix,
			char *new_filename_prefix,
			char *new_filename,
			size_t max_new_filename_length )
{
	static const char fname[] = "mx_change_filename_prefix()";

	char *old_filename_ptr;
	size_t old_prefix_length, new_prefix_length;
	char end_char;

	if ( old_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The old_filename pointer passed was NULL." );
	}
	if ( new_filename == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The new_filename pointer passed was NULL." );
	}

	/* If present, strip off the old prefix. */

	if ( old_filename_prefix == NULL ) {
		old_filename_ptr = old_filename;
	} else {
		old_prefix_length = strlen(old_filename_prefix);

		if ( old_prefix_length == 0 ) {
			old_filename_ptr = old_filename;
		} else {
			/* Does the old prefix match the start of the
			 * old filename?
			 */

			if ( strncmp( old_filename_prefix, old_filename,
					old_prefix_length ) != 0 )
			{
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"The old filename prefix '%s' does not match "
				"the start of the old filename '%s'.",
					old_filename_prefix,
					old_filename );
			} else {
				old_filename_ptr =
					old_filename + old_prefix_length;
			}
		}
	}

	/* Skip over any leading '/' character. */

	if ( *old_filename_ptr == '/' ) {
		old_filename_ptr++;
	}

	/* Now create the new filename. */

	if ( new_filename_prefix == NULL ) {
		strlcpy( new_filename, old_filename_ptr,
				max_new_filename_length );
	} else {
		new_prefix_length = strlen(new_filename_prefix);

		if ( new_prefix_length == 0 ) {
			strlcpy( new_filename, old_filename_ptr,
					max_new_filename_length );
		} else {
			/* Does the new prefix have a path separator '/'
			 * at the end of the filename?
			 */

			end_char = new_filename_prefix[ new_prefix_length - 1 ];

			if (end_char == '/') {
				/* If the filename already contains a path
				 * separator, then we can just concatenate
				 * the strings as is.
				 */

				snprintf( new_filename, max_new_filename_length,
					"%s%s", new_filename_prefix,
					old_filename_ptr );
			} else {
				/* Otherwise, we must insert a path separator.*/

				snprintf( new_filename, max_new_filename_length,
					"%s/%s", new_filename_prefix,
					old_filename_ptr );
			}
		}
	}

#if 0
	MX_DEBUG(-2,("%s: old_filename = '%s'", fname, old_filename));
	MX_DEBUG(-2,("%s: old_filename_prefix = '%s'",
					fname, old_filename_prefix));
	MX_DEBUG(-2,("%s: new_filename_prefix = '%s'",
					fname, new_filename_prefix));
	MX_DEBUG(-2,("%s: new_filename = '%s'", fname, new_filename));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT char *
mx_username( char *buffer, size_t max_buffer_length )
{
#if defined( OS_WIN32 ) || defined( OS_LINUX ) || defined( OS_TRU64 )
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

#if defined( OS_LINUX ) || defined( OS_TRU64 )
	{
		char scratch_buffer[ 1024 ];
		struct passwd pw_buffer, *pw;
		uid_t uid;
		int status, saved_errno;

		pw = NULL;

		uid = getuid();

#if defined( OS_LINUX ) || defined( OS_TRU64 )

		/* FIXME: Valgrind says that this call to getpwuid_r()
		 * results in a memory leak.  However, the man page
		 * for getpwuid_r() on Linux does not mention anything
		 * about allocating memory that needs to be freed by
		 * the caller.
		 */

		status = getpwuid_r( uid, &pw_buffer, scratch_buffer,
					sizeof(scratch_buffer), &pw );

#elif defined( OS_SOLARIS )
		/* FIXME: This doesn't work for some reason. */

		MX_DEBUG( 2,("%s: before getpwuid_r()", fname));

		pw = getpwuid_r( uid, &pw_buffer, scratch_buffer,
					sizeof(scratch_buffer) );

		if ( pw == NULL ) {
			status = -1;
		} else {
			status = 0;
		}

		MX_DEBUG( 2,("%s: pw = , status = %d",
			fname, status));
#elif defined( OS_HPUX )
		/* FIXME: At the moment getpwuid_r() core dumps with
		 * a segmentation fault for some reason.
		 */

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

			strlcpy( ptr, "unknown", max_buffer_length );

			return ptr;
		}
		strlcpy( ptr, pw->pw_name, max_buffer_length );
	}

/* End of OS_LINUX, OS_SOLARIS, and OS_HPUX section. */

#elif defined( OS_IRIX ) || defined( OS_SUNOS4 ) || defined( OS_SOLARIS ) \
   || defined( OS_MACOSX ) || defined( OS_BSD ) || defined( OS_QNX ) \
   || defined( OS_HPUX )
	{
		/* This method is not reentrant. */

		struct passwd *pw;

		pw = getpwuid( getuid() );

		if ( pw == NULL ) {
			strlcpy( ptr, "unknown", max_buffer_length );

			return ptr;
		}
		strlcpy( ptr, pw->pw_name, max_buffer_length );
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
			strlcpy( ptr, "unknown", max_buffer_length );
		}
	}

#elif defined( OS_CYGWIN ) || defined( OS_DJGPP ) || defined( OS_VMS )
	{
		char *login_ptr;

		login_ptr = getlogin();
 
		if ( login_ptr == NULL ) {
			strlcpy( ptr, "unknown", max_buffer_length );
		} else {
			strlcpy( ptr, login_ptr, max_buffer_length );
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

	strlcpy( ptr, "rtems", max_buffer_length );

#elif defined( OS_VXWORKS )

	strlcpy( ptr, "vxworks", max_buffer_length );

#elif defined( OS_ECOS )

	strlcpy( ptr, "ecos", max_buffer_length );

#else

#error mx_username() has not been implemented for this platform.

#endif
	return ptr;
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

#elif ( defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)) )

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
mx_start_debugger( char *command )
{
	fprintf(stderr,
	    "\nWarning: The Visual C++ debugger is being started.\n\n");
	fflush(stderr);

	mx_debugger_started = TRUE;

	DebugBreak();

	return;
}

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS)

MX_EXPORT void
mx_start_debugger( char *command )
{
	static const char fname[] = "mx_start_debugger()";

	char terminal[80];
	char command_line[80];
	char *ptr;
	unsigned long pid, spawn_flags;
	mx_bool_type x_windows_available, use_suspend;
	mx_status_type mx_status;

	static char unsafe_fmt[] =
    "Unsafe command line '%s' requested for %s.  The command will be ignored.";

	mx_debugger_started = TRUE;

	pid = mx_process_id();

#if 1 || defined(OS_MACOSX) || defined(OS_SOLARIS)
	use_suspend = FALSE;
#else
	use_suspend = TRUE;
#endif

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
			strlcpy( command_line, command, sizeof(command_line) );
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

			snprintf( command_line, sizeof(command_line),
						command, pid );
		}
	} else {
		/* If no command line was specified, then we try several
		 * commands in turn.
		 */

		command_line[0] = '\0';

		if ( getenv("DISPLAY") != NULL ) {
			x_windows_available = TRUE;
		} else {
			x_windows_available = FALSE;
		}

		if ( x_windows_available ) {

			/* Look for an appropriate terminal emulator. */

			if ( mx_command_found( "konsole" ) ) {
				strlcpy( terminal, "konsole --vt_sz 80x52 -e",
					sizeof(terminal) );
			} else {
				strlcpy( terminal, "xterm -geometry 80x52 -e",
					sizeof(terminal) );
			}

			/* Look for an appropriate debugger. */

#if defined(OS_LINUX)
			if ( mx_command_found( "ddd" ) ) {

				snprintf( command_line, sizeof(command_line),
					"ddd --gdb -p %lu", pid );
			} else
#endif
#if defined(OS_SOLARIS)
			if ( mx_command_found( "sunstudio" ) ) {

				snprintf( command_line, sizeof(command_line),
					"sunstudio -A %lu", pid );
			} else
#endif
			if ( mx_command_found( "gdbtui" ) ) {

				snprintf( command_line, sizeof(command_line),
					"%s gdbtui -p %lu", terminal, pid );
			} else
			if ( mx_command_found( "gdb" ) ) {

				snprintf( command_line, sizeof(command_line),
					"%s gdb -p %lu", terminal, pid );
			} else
			if ( mx_command_found( "dbx" ) ) {

				snprintf( command_line, sizeof(command_line),
					"%s dbx - %lu", terminal, pid );
			}
		}
	}

	if ( command_line[0] != '\0' ) {

		/* If a debugger has been found, start it. */

		fprintf( stderr,
		"\nWarning: Starting a debugger using the command '%s'.\n",
			command_line );

		if ( use_suspend ) {
			spawn_flags = MXF_SPAWN_SUSPEND_PARENT;
		} else {
			spawn_flags = 0;
		}

		mx_status = mx_spawn( command_line, spawn_flags, NULL );

		/* See if starting the debugger succeeded. */

		if ( mx_status.code == MXE_SUCCESS ) {

			/* If we did not do a suspend in mx_spawn(),
			 * then we must wait for the debugger here.
			 */

			if ( use_suspend == FALSE ) {
				mx_wait_for_debugger();
			}
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

#if defined(USE_MX_DEBUGGER_IS_PRESENT)

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

#elif 0

/* This version attempts to detect attachment by the debugger by looking
 * for unexpected delays in the loop below.
 *
 * This method is not 1000% bulletproof.  Also, on Linux in GDB, the
 * 'finish' command does not correctly find the right stack frame to
 * return to.  Instead, it acts like a 'continue' command, which is
 * not what we want.  In order to get this to work, you typically end up
 * having to type a bunch of 'stepi' commands to step back into the 
 * mx_msleep() stack frame.
 */

MX_EXPORT void
mx_wait_for_debugger( void )
{
	unsigned long pid;
	double current_time, old_time;

	pid = mx_process_id();

	fprintf( stderr,
	"\nProcess %lu is now waiting to be attached to by a debugger.\n\n",
		pid );
	fprintf( stderr,
	"The appropriate attach command for GDB is 'gdb -p %lu'\n", pid );
	fprintf( stderr,
	"The appropriate attach command for DBX is 'dbx - %lu'\n", pid );
	fprintf( stderr, "\nWaiting...\n" );

	old_time = mx_high_resolution_time_as_double();

	while (1) {
		mx_msleep(10);

		current_time = mx_high_resolution_time_as_double();

		if ( (current_time - old_time) > 1.0 ) {
			break;
		}

		old_time = current_time;
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
		
	fprintf( stderr,
"If you are using DBX, follow this procedure:\n"
"  1.  If not already attached, attach to the process with the command\n"
"          'dbx - %lu'\n"
"  2.  Type 'assign loop=0' in the mx_wait_for_debugger() stack frame\n"
"      to break out of the loop.\n"
"  3.  Use the command 'step up' to run functions to completion.\n\n", pid );
		
	fprintf( stderr, "Waiting...\n\n" );

	/* Synchronize with the debugger by waiting for the debugger
	 * to reset the value of 'loop' to 0.
	 */

	loop = 1;

	while ( loop ) {

#ifdef OS_LINUX
		/* Do nothing. */
#else
		mx_msleep(1000);
#endif
	}

	return;
}

#endif

/*-------------------------------------------------------------------------*/

MX_EXPORT int
mx_get_max_file_descriptors( void )
{
	int result;

#if defined( OS_LINUX ) || defined( OS_SOLARIS ) || defined( OS_IRIX ) \
	|| defined( OS_HPUX ) || defined( OS_SUNOS4 ) || defined( OS_MACOSX ) \
	|| defined( OS_BSD ) || defined( OS_QNX ) || defined( OS_CYGWIN ) \
	|| defined( OS_DJGPP ) || defined( OS_VMS ) || defined( OS_TRU64 )

	result = getdtablesize();

#elif defined( OS_WIN32 )

	result = FD_SETSIZE;

#elif defined( OS_ECOS )

	result = CYGNUM_FILEIO_NFD;

#elif defined( OS_RTEMS ) || defined( OS_VXWORKS )

	result = 16;		/* A wild guess and probably wrong. */

#else

#error mx_get_max_file_descriptors() has not been implemented for this platform.

#endif

	return result;
}

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

/* mxp_value_is_near_integer() is used by the mx_round_...() set of 
 * functions to try and guard against imprecise floating point
 * representations of numbers.  It assumes that double precision
 * numbers have around 15 significant digits.  This is true for
 * IEEE floating point.
 *
 * For example, this guards against the possibility of the function
 * mx_round_down( 3.9999999999999998 ) returning 3 instead of 4.
 */

#define MXP_DIFFERENCE_THRESHOLD	(1.0e-15)

static mx_bool_type
mxp_value_is_near_integer( double value )
{
	double relative_difference;

	relative_difference = mx_difference( value, (double)mx_round(value) );

	if ( relative_difference < MXP_DIFFERENCE_THRESHOLD ) {
		return TRUE;
	} else {
		return FALSE;
	}
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
mx_round_down( double value )
{
	long result;

	if ( mxp_value_is_near_integer(value) ) {
		result = mx_round(value);
	} else {
		if ( value >= 0.0 ) {
			result = (long) value;
		} else {
			result = (long) ( value - 1.0 );
		}
	}

	return result;
}

MX_EXPORT long
mx_round_up( double value )
{
	long result;

	if ( mxp_value_is_near_integer(value) ) {
		result = mx_round(value);
	} else {
		if ( value >= 0.0 ) {
			result = (long) ( value + 1.0 );
		} else {
			result = (long) value;
		}
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

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

MX_EXPORT int
mx_parse_command_line( const char *command_line,
			int *argc, char ***argv,
			int *envc, char ***envp )
{
	char *command_buffer;
	char *ptr, *token_ptr;
	int in_quoted_string;
	int status;

	errno = 0;

	if ( ( command_line == NULL )
	  || ( argc == NULL )
	  || ( argv == NULL )
	  || ( envc == NULL )
	  || ( envp == NULL ) )
	{
		errno = EINVAL;

		return (-1);
	}

	command_buffer = strdup( command_line );

	if ( command_buffer == NULL ) {
		errno = ENOMEM;

		return (-1);
	}

#if 0
	MX_DEBUG(-2,("mx_parse_command_line('%s'), command_buffer = %p",
		command_line, command_buffer));
#endif

	*argc = 0;
	*envc = 0;
	*argv = NULL;
	*envp = NULL;

	/* If token_ptr is not NULL, then we are in the middle of a token. */

	token_ptr = NULL;
	in_quoted_string = FALSE;

	for ( ptr = command_buffer; ; ptr++ ) {

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
				*argv = (char **) malloc( sizeof(char *) );
			}

			if ( *envc == 0 ) {
				*envp = (char **) malloc( sizeof(char *) );
			}

			if ( *argv != NULL ) {
				(*argv)[*argc] = NULL;
			}

			if ( *envp != NULL ) {
				(*envp)[*envc] = NULL;
			}

			/* If both argc and envc are zero, then we
			 * will not need 'command_buffer' later, so
			 * we free it now.
			 */

			if ( ( *argc == 0 ) && ( *envc == 0 ) ) {
				mx_free( command_buffer );
			}

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

	/* It should not be possible for us to get here. */

#if defined(OS_SOLARIS)
	/* Do nothing */

#elif ( defined(OS_HPUX) && defined(__ia64) )
	/* Do nothing */

#else
	errno = ERANGE;
	return (-1);
#endif
}

MX_EXPORT void
mx_free_command_line( char **argv, char **envp )
{
	char *command_buffer, *argv0, *envp0;

	MX_DEBUG(-2,("mx_free_command_line( %p, %p ) invoked.", argv, envp));

	/* First find a pointer to the internal 'command_buffer'.  The
	 * correct pointer will be either argv[0] or envp[0].  We must
	 * compare these two pointers to see which one has the lower
	 * memory address.  The one with the lower memory address is
	 * the address of 'command_buffer'.
	 */

	command_buffer = NULL;   argv0 = NULL;  envp0 = NULL;

	if ( argv == NULL ) {
		if ( envp == NULL ) {
			/* Nothing to do, so return. */
			return;
		} else {
			command_buffer = envp[0];
		}
	} else {
		argv0 = argv[0];

		if ( envp == NULL ) {
			command_buffer = argv0;
		} else {
			envp0 = envp[0];

			if ( envp0 == NULL ) {
				command_buffer = argv0;
			} else {
				if ( argv0 == NULL ) {
					command_buffer = envp0;
				} else {
					if ( envp0 < argv0 ) {
						command_buffer = envp0;
					} else {
						command_buffer = argv0;
					}
				}
			}
		}
	}

	MX_DEBUG(-2,("  command_buffer = %p, argv0 = %p, envp0 = %p",
		command_buffer, argv0, envp0 ));

	if ( argv != NULL ) {
		free( argv );
	}
	if ( envp != NULL ) {
		free( envp );
	}
	if ( command_buffer != NULL ) {
		free( command_buffer );
	}

	return;
}

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32) && defined(_MSC_VER)

/* The Win32 functions _snprintf() and _vsnprintf() do not null terminate
 * the strings they return if the output was truncated.  This is inconsitent
 * with the Unix definition of these functions, so we cannot simply define
 * snprintf() as _snprintf().  Instead, we provide wrapper functions that
 * make sure that the output string is always terminated.  Thanks to the
 * authors of http://www.di-mgt.com.au/cprog.html for pointing this out.
 */

MX_EXPORT int
mx_snprintf( char *dest, size_t maxlen, const char *format, ... )
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
mx_vsnprintf( char *dest, size_t maxlen, const char *format, va_list args  )
{
	int result;

	result = _vsnprintf( dest, maxlen-1, format, args );

	if ( result < 0 ) {
		dest[ maxlen-1 ] = '\0';
	}

	return result;
}

#elif defined(OS_VXWORKS) || defined(OS_DJGPP) \
	|| (defined(OS_VMS) && (__VMS_VER < 70320000))

/* Some platforms do not provide snprintf() and vsnprintf().  For those
 * platforms we fall back to sprintf() and vsprintf().  The hope in doing
 * this is that any buffer overruns will be found on the plaforms that
 * _do_ support snprintf() and vsnprint().
 */

MX_EXPORT int
mx_snprintf( char *dest, size_t maxlen, const char *format, ... )
{
	va_list args;
	int result;

	va_start( args, format );

	result = vsprintf( dest, format, args );

	va_end( args );

	return result;
}

MX_EXPORT int
mx_vsnprintf( char *dest, size_t maxlen, const char *format, va_list args  )
{
	return vsprintf( dest, format, args );
}

#endif

/*-------------------------------------------------------------------------*/

/* For some reason, the 64-bit version of Windows 7 crashes when I try
 * to free() memory allocated with the strdup() provided by Microsoft.
 * However, the following replacement works just fine.
 */

MX_EXPORT char *
mx_strdup( const char *original )
{
	char *duplicate;
	size_t original_length;

	if ( original == NULL ) {
		errno = EINVAL;

		return NULL;
	}

	original_length = strlen(original) + 1;

	duplicate = malloc( original_length );

	if ( duplicate == NULL ) {
		errno = ENOMEM;

		return NULL;
	}

	strlcpy( duplicate, original, original_length );

	return duplicate;
}

/*-------------------------------------------------------------------------*/

#if ( defined(_MSC_VER) && (_MSC_VER < 1300) )

/* Visual C++ 6.0 SP 6 does not have a direct way of converting an
 * unsigned 64 bit integer to a double.
 */

MX_EXPORT double
mx_uint64_to_double( unsigned __int64 uint64_value )
{
	int64_t int64_temp;
	double result;

	int64_temp = uint64_value & 0x7FFFFFFFFFFFFFFF;
	
	result = (double) int64_temp;

	if ( uint64_value >= 0x8000000000000000 ) {
		result += 9223372036854775808.0;
	}

	return result;
}
#endif

/*-------------------------------------------------------------------------*/

#if 0

MX_EXPORT struct timespec
mx_current_os_time( void )
{
	struct timespec result;

	result.tv_sec = 0;
	result.tv_nsec = 0;

	return result;
}

#elif defined(OS_WIN32)

MX_EXPORT struct timespec
mx_current_os_time( void )
{
	DWORD os_time;
	struct timespec result;

	os_time = timeGetTime();

	result.tv_sec = os_time / 1000L;
	result.tv_nsec = ( os_time % 1000L ) * 1000000L;

	return result;
}

#elif defined(OS_ECOS) || defined(OS_VXWORKS)

MX_EXPORT struct timespec
mx_current_os_time( void )
{
	static const char fname[] = "mx_current_os_time()";

	struct timespec result;
	int os_status, saved_errno;

	os_status = clock_gettime(CLOCK_REALTIME, &result);

	if ( os_status == -1 ) {
		saved_errno = errno;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"The call to clock_gettime() failed.  "
			"Errno = %d, error message = '%s'",
				saved_errno, strerror(saved_errno) );
	}

	return result;
}

#else

MX_EXPORT struct timespec
mx_current_os_time( void )
{
	static const char fname[] = "mx_current_os_time()";

	struct timespec result;
	struct timeval os_timeofday;
	int os_status, saved_errno;

	os_status = gettimeofday( &os_timeofday, NULL );

	if ( os_status != 0 ) {
		saved_errno = errno;

		result.tv_sec = 0;
		result.tv_nsec = 0;

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"A call to gettimeofday() failed.  "
		"Errno = %d, error message = '%s'",
			saved_errno, strerror(saved_errno) );
	} else {
		result.tv_sec = os_timeofday.tv_sec;
		result.tv_nsec = 1000L * os_timeofday.tv_usec;
	}

	return result;
}

#endif

MX_EXPORT char *
mx_os_time_string( struct timespec os_time,
			char *buffer,
			size_t buffer_length )
{
	static const char fname[] = "mx_os_time_string()";

	time_t time_in_seconds;
	struct tm *tm_struct_ptr;
	char *ptr;
	char local_buffer[20];
	double nsec_in_seconds;

	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The string buffer pointer passed was NULL." );

		return NULL;
	}

	time_in_seconds = os_time.tv_sec;

#if defined(OS_DJGPP)
	tm_struct_ptr = localtime( &time_in_seconds );

#elif defined(OS_WIN32)
#  if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	{
		struct tm tm_struct;

		localtime_s( &tm_struct, &time_in_seconds );

		tm_struct_ptr = &tm_struct;
	}
#  else
	tm_struct_ptr = localtime( &time_in_seconds );
#  endif
#else
	{
		struct tm tm_struct;

		localtime_r( &time_in_seconds, &tm_struct );

		tm_struct_ptr = &tm_struct;
	}
#endif

	strftime( buffer, buffer_length,
		"%a %b %d %Y %H:%M:%S", tm_struct_ptr );

	nsec_in_seconds = 1.0e-9 * (double) os_time.tv_nsec;

	snprintf( local_buffer, sizeof(local_buffer), "%f", nsec_in_seconds );

	ptr = strchr( local_buffer, '.' );

	if ( ptr == NULL ) {
		return buffer;
	}

	strlcat( buffer, ptr, buffer_length );

	return buffer;
}

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

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
mx_string_token( char **string_ptr, const char *delim )
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

MX_EXPORT int
mx_string_split( char *original_string,
			const char *delim,
			int *argc,
			char ***argv )
{
	unsigned long block_size, num_blocks, array_size;
	char *string_ptr, *token_ptr;

	if ( (original_string == NULL)
	  || (delim == NULL)
	  || (argc == NULL)
	  || (argv == NULL) )
	{
		errno = EINVAL;
		return -1;
	}

	*argc = 0;

	block_size = 10;
	num_blocks = 1;

	array_size = num_blocks * block_size;

	*argv = malloc( array_size * sizeof(char**) );

	if ( *argv == NULL ) {
		errno = ENOMEM;
		return -1;
	}

	errno = 0;

	string_ptr = original_string;

	while(TRUE) {
		token_ptr = mx_string_token( &string_ptr, delim );

		if ( (*argc) >= array_size ) {
			num_blocks++;
			array_size = num_blocks * block_size;

			*argv = realloc( *argv, array_size * sizeof(char**) );

			if ( *argv == NULL ) {
				errno = ENOMEM;
				return -1;
			}

			errno = 0;
		}

		if ( token_ptr == NULL ) {
			/* There are no more tokens.  One extra NULL goes
			 * at the end of argv.
			 */

			(*argv)[*argc] = NULL;

			return 0;
		}

		(*argv)[*argc] = token_ptr;

		(*argc)++;
	}
}

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

/*------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_verify_directory( char *directory_name, int create_flag )
{
	static const char fname[] = "mx_verify_directory()";

	struct stat stat_buf;
	int os_status, saved_errno;

	if ( directory_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The directory name pointer passed was NULL." );
	}

	/* Does a filesystem object with this name already exist? */

	os_status = access( directory_name, F_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno != ENOENT ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while testing for the presence of "
			"directory '%s'.  "
			"Errno = %d, error message = '%s'",
				directory_name,
				saved_errno, strerror( saved_errno ) );
		}

		if ( create_flag == FALSE ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The directory named '%s' does not exist.",
				directory_name );
		}

		/* The directory does not already exist, so create it. */

		os_status = mkdir( directory_name, 0777 );

		if ( os_status == 0 ) {
			/* Creating the directory succeeded, so we are done! */

			return MX_SUCCESSFUL_RESULT;
		} else {
			/* Creating the directory failed, so report the error.*/

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Creating subdirectory '%s' failed.  "
			"Errno = %d, error message = '%s'",
				directory_name,
				saved_errno, strerror(saved_errno) );
		}
	}

	/*------------*/

	/* A filesystem object with this name already exists, so 
	 * we must find out more about it.
	 */

	os_status = stat( directory_name, &stat_buf );

	if ( os_status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"stat() failed for file '%s'.  "
		"Errno = %d, error message = '%s'",
			directory_name,
			saved_errno, strerror(saved_errno) );
	}

	/* Is the object a directory? */

#if defined(OS_WIN32)
	if ( (stat_buf.st_mode & _S_IFDIR) == 0 ) {
#else
	if ( S_ISDIR(stat_buf.st_mode) == 0 ) {
#endif
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Existing file '%s' is not a directory.",
			directory_name );
	}

	/* Although we just did stat(), determining the access permissions
	 * is more portably done with access().
	 */

	os_status = access( directory_name, R_OK | W_OK | X_OK );

	if ( os_status != 0 ) {
		saved_errno = errno;

		if ( saved_errno == EACCES ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"We do not have read, write, and execute permission "
			"for directory '%s'.", directory_name );
		}

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Checking the access permissions for directory '%s' "
		"failed.  Errno = %d, error message = '%s'",
			directory_name,
			saved_errno, strerror( saved_errno ) );
	}

	/* If we get here, the directory already exists and is useable. */

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------*/

#if defined(OS_VXWORKS) || defined(OS_WIN32)

/* Some platforms define mkdir() differently or not at all, so we provide
 * our own front end here.
 */

/* WARNING: This code must be after any calls to mkdir() in this file
 * due to the #undef that follows.
 */

#ifdef mkdir
#undef mkdir
#endif

#if defined(__BORLANDC__)
#include <dir.h>
#endif

MX_EXPORT int
mx_mkdir( char *pathname, mode_t mode )
{
	int os_status;

#if defined(OS_WIN32)
	os_status = _mkdir( pathname );
#else
	os_status = mkdir( pathname );
#endif

	return os_status;
}

#endif

