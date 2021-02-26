/*
 * Name:    mx_util.c
 *
 * Purpose: Define some of the utility functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2021 Illinois Institute of Technology
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

#if defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_ANDROID ) \
	|| defined( OS_MINIX )
#include <sys/time.h>
#include <pwd.h>
#endif

#include "mx_util.h"
#include "mx_time.h"
#include "mx_hrt.h"
#include "mx_unistd.h"
#include "mx_signal.h"
#include "mx_record.h"
#include "mx_signal_alloc.h"
#include "mx_atomic.h"
#include "mx_json.h"

/*-------------------------------------------------------------------------*/

#if defined( OS_WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )

/* For Visual C++, we define a custom invalid parameter handler that
 * generates a message and then attempts to continue execution.
 *
 * 2012-05-26 (WML) - We now suppress all output from the invalid parameter
 * handler.  The reason for this is that there are occasions where we do
 * want to pass potentially invalid parameters to the C runtime.  The case
 * that prompted this change was using _get_osfhandle() to see if a C
 * file descriptor is valid.
 */

#if defined( _DEBUG )
  #include <crtdbg.h>
#endif

static void
mxp_msc_invalid_parameter_handler( const wchar_t* expression,
				const wchar_t* function,
				const wchar_t* file,
				unsigned int line,
				uintptr_t pReserved )
{
#if 0
	wprintf(L"MX Visual C++ invalid parameter handler invoked.\n" );

#if defined( _DEBUG )
	wprintf(L"  Invalid parameter detected in function %s."
		L"  File: %s Line %d\n",
			function, file, line );

	wprintf(L"  Expression: %s\n", expression );
#endif

	wprintf(L"  Warning: This program may behave strangely "
		L"after this point.\n" );
#endif
}

#endif

/*--------*/

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

#if defined( OS_WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )

	/* Intercept the Microsoft invalid parameter handler. */

	{
		_invalid_parameter_handler old_handler, new_handler;

		new_handler = mxp_msc_invalid_parameter_handler;

		old_handler = _set_invalid_parameter_handler( new_handler );

#if defined( _DEBUG )
		_CrtSetReportMode( _CRT_ASSERT, 0 );
#endif
	}
#endif

	/* Initialize MX realtime signal management. */

#if defined( _POSIX_REALTIME_SIGNALS )
	mx_status = mx_signal_alloc_initialize();

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

	/* Initialize stack checking (if it needs it). */

	mx_stack_check();

	/* Initialize JSON support. */

#if !defined(OS_WIN32)
	mx_json_initialize();
#endif

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

#if defined(OS_VXWORKS)
#  include <remLib.h>
#endif

MX_EXPORT char *
mx_username( char *buffer, size_t max_buffer_length )
{
#if defined( OS_WIN32 ) || defined( OS_LINUX ) || defined( OS_TRU64 ) \
    || defined( OS_HURD ) || defined( OS_ANDROID )
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

#if defined( OS_LINUX ) || defined( OS_TRU64 ) || defined(OS_HURD) \
    || defined( OS_ANDROID )
	{
		char scratch_buffer[ 1024 ];
		struct passwd pw_buffer, *pw;
		uid_t uid;
		int status, saved_errno;

		pw = NULL;

		uid = getuid();

		/* FIXME: The apparent duplication of the #define below
		 * reflects the fact that "once upon a time" the #define
		 * that precedes this was supposed to include Solaris,
		 * HP/UX, etc.  However, the proposed implementations
		 * below did not work for some reason.  Rather than delete
		 * them from the code altogether, we instead reduce the
		 * number of OSes listed above.  Or something like that...
		 */

#if defined( OS_LINUX ) || defined( OS_TRU64 ) || defined(OS_HURD) \
	|| defined( OS_ANDROID )

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
   || defined( OS_HPUX ) || defined( OS_UNIXWARE ) || defined( OS_MINIX )
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

#elif defined( OS_VXWORKS )

	/* FIXME: remCurIdGet() does not have a way of specifying
	 * how long the username buffer should be.  So how long
	 * _should_ it be???
	 */

	remCurIdGet( ptr, NULL );

#elif defined( OS_RTEMS )

	strlcpy( ptr, "rtems", max_buffer_length );

#elif defined( OS_ECOS )

	strlcpy( ptr, "ecos", max_buffer_length );

#else

#error mx_username() has not been implemented for this platform.

#endif
	return ptr;
}

/*-------------------------------------------------------------------------*/

#if 0

/* Win32: Visual Studio 2005 and later have _putenv_s(). */

/* WARNING: Regrettably we _CANNOT_ use _putenv_s in MX.  If a third party DLL
 * like XIA Handel uses getenv() to read environment variables like XIAHOME,
 * it will _not_ see any changes made by _putenv_s().  Regrettably, after a
 * Win32 program starts up, the environment strings seen by getenv() are
 * totally unaffected by anything done by _putenv_s().  So we are forced to
 * stick with _putenv(), since we cannot force third party DLLs to change.
 *
 * If this was Linux, there would be ways around this.  But on Windows, there
 * is no reasonable way around this that I am aware of.  Unfortunate.
 *
 * (W. Lavender, 2019-11-21)
 */

MX_EXPORT int
mx_setenv( const char *env_name,
		const char *env_value )
{
	int saved_errno;

	saved_errno = _putenv_s( env_name, env_value );

	if ( saved_errno == 0 ) {
		errno = 0;
		return 0;
	} else {
		errno = saved_errno;
		return (-1);
	}
}

#elif ( defined(OS_WIN32) || defined(OS_VXWORKS) )

/* Some platforms have only putenv(). */

#if ( defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) )
#define putenv(x)  _putenv( (x) )
#endif

MX_EXPORT int
mx_setenv( const char *env_name,
		const char *env_value )
{
	/* We must dynamically allocate a buffer that is big enough
	 * to contain the entire string that we are going to send
	 * to putenv(), including the equals sign '=' and the
	 * trailing null byte.
	 */
	size_t buffer_size;
	char *buffer;
	int os_status;

	if ( (env_name == NULL) || (env_value == NULL ) ) {
		errno = EINVAL;
		return (-1);
	}

	buffer_size = strlen(env_name) + strlen(env_value) + 2;

	buffer = malloc( buffer_size );

	if ( buffer == NULL ) {
		errno = ENOMEM;
		return (-1);
	}

	snprintf( buffer, buffer_size, "%s=%s", env_name, env_value );

	os_status = putenv( buffer );

	/* Avoid a memory leak by unconditionally freeing the buffer
	 * regardless of what may have happened in putenv().
	 */

	free( buffer );

	/* Apparently putenv() does not set errno, so it is hard to
	 * know what to do if it fails.  For lack of a better idea,
	 * we set errno to EINVAL, since something about the arguments
	 * we were sent must be bad.
	 */

	if ( os_status == 0 ) {
		errno = 0;
		return 0;
	} else {
		errno = EINVAL;
		return (-1);
	}
}

#elif defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_SOLARIS) \
	|| defined(OS_BSD) || defined(OS_QNX) || defined(OS_RTEMS) \
	|| defined(OS_ANDROID) || defined(OS_CYGWIN) || defined(OS_VMS) \
	|| defined(OS_HURD) || defined(OS_DJGPP) || defined(OS_MINIX)

MX_EXPORT int
mx_setenv( const char *env_name,
		const char *env_value )
{
	int os_status;

	os_status = setenv( env_name, env_value, TRUE );

	if ( os_status == 0 ) {
		return 0;
	} else {
		return errno;
	}
}

#else
#error mx_setenv() has not been implemented for this platform.
#endif

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

MX_EXPORT int
mx_expand_env( const char *original_env_value,
		char *new_env_value, size_t max_env_size )
{
	int os_status;

	os_status = (int) ExpandEnvironmentStrings( original_env_value,
						new_env_value, max_env_size );

	if ( os_status == 0 ) {
		char error_message[200];
		DWORD last_error_code;

		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
				error_message, sizeof(error_message) );

		mx_warning( "mx_expand_env() failed with "
			"Win32 error code = %ld, error message = '%s'.  "
			"Original string was '%s'.",
				last_error_code, error_message,
				original_env_value );
	}

	return os_status;
}

#else
#error mx_expand_env() has not been implemented for this platform.
#endif

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
mx_hex_string_to_long( char *string )
{
	/* WARNING: This assumes twos complement arithmetic. */

	static const char fname[] = "mx_hex_string_to_unsigned_long()";

	char *endptr;
	unsigned long ulong_result;
	unsigned long high_order_nibble, num_bits_in_result, sign_mask;
	long result;

	/* Skip over any leading whitespace. */

	string += strspn( string, MX_WHITESPACE );

	/* Parse the string. */

	ulong_result = strtoul( string, &endptr, 16 );

	if ( *endptr != '\0' ) {
		result = 0;

		mx_warning(
		"%s: Supplied string '%s' is not a hexadecimal number.  "
		"Result set to zero.", fname, string );
	} else {
		if ( ulong_result <= LONG_MAX ) {

			/* Check to see if the original hexadecimal string was
			 * negative.  If it was, then return a negative result.
		 	 */

			high_order_nibble =
				mx_hex_char_to_unsigned_long( string[0] );

			if ( high_order_nibble >= 8 ) {
				num_bits_in_result = 4 * strlen(string);

				sign_mask = 1UL << (num_bits_in_result - 1);

				ulong_result = (ulong_result ^ sign_mask)
							- sign_mask;
			}

		}

		result = (long) ulong_result;
	}

	return result;
}

MX_EXPORT long
mx_string_to_long( char *string )
{
	static const char fname[] = "mx_string_to_long()";

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
	static const char fname[] = "mx_string_to_unsigned_long()";

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

MX_EXPORT const char *
mx_timestamp( char *buffer, size_t buffer_length )
{
	static const char fname[] = "mx_timestamp()";

	time_t time_struct;
	struct tm current_time;

	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed to this function was NULL." );
	}

	time( &time_struct );

	(void) localtime_r( &time_struct, &current_time );

	strftime( buffer, buffer_length,
		"%b %d %H:%M:%S", &current_time );

	return buffer;
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

#define MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY	FALSE

/* If you want to use something like snprintf() to print a list of arguments,
 * but you do not know how many arguments you will have until run time, then
 * snprintf() cannot help you, since C does not allow you to manually create
 * a va_list using portable C code.
 *
 * Instead, we use our homebrew mx_snprint_from_pointer_array(), where you
 * provide an array of void pointers to individual arguments that you
 * want to print.  snprintf() is actually used to implement the printing
 * of individual items from 'pointer_array'.
 */

MX_EXPORT int
mx_snprintf_from_pointer_array( char *destination,
				size_t maximum_length,
				const char *format,
				size_t num_argument_pointers,
				void **argument_pointer_array )
{
	static const char fname[] = "mx_snprintf_from_pointer_array()";

	long i, bytes_written, local_format_bytes;
	const char *format_ptr, *percent_ptr, *conversion_ptr;
	char *local_format, *snprintf_destination;
	void *argument_pointer;
	double double_value;
	char char_value;
	short short_value;
	int int_value;
	long long_value;
	int64_t int64_value;
	unsigned char uchar_value;
	unsigned short ushort_value;
	unsigned int uint_value;
	unsigned long ulong_value;
	uint64_t uint64_value;

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
	MX_DEBUG(-2,("%s: argument_pointer_array = %p",
		fname, argument_pointer_array));

	for ( i = 0; i < num_argument_pointers; i++ ) {
		MX_DEBUG(-2,("%s: argument_pointer_array[%ld] = %p",
			fname, i, argument_pointer_array[i]));
	}
#endif

	*destination = '\0';

	format_ptr = format;
	percent_ptr = NULL;
	conversion_ptr = NULL;

	i = 0;

	while( TRUE ) {

		/* Look for the next occurrence of the % character. */

		percent_ptr = strchr( format_ptr, '%' );

		if ( percent_ptr == NULL ) {
			/* We are now beyond the last conversion character,
			 * so we just copy the remainder of the format string
			 * to the destination buffer.
			 */

			strlcat( destination, format_ptr, maximum_length );

			bytes_written = strlen( destination );

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
			MX_DEBUG(-2,("%s: RETURN 1, destination = '%s'",
				fname, destination));
#endif

			return bytes_written;
		}

		if ( percent_ptr[1] == '%' ) {

			/* Is the next character after the % character also
			 * a % character?  Then we need to copy _both_
			 * % characters to the local format buffer to ensure
			 * that snprintf() correctly handles this.
			 */

			local_format_bytes = percent_ptr - format_ptr + 2;

		} else {
			/* Search for any occurences of the snprintf()
			 * conversion characters.
			 */

			conversion_ptr = strpbrk( percent_ptr,
						"aAcdeEfgGiopsuxX" );

			if ( conversion_ptr == (char *) NULL ) {

				/* No conversion characters were found,
				 * so just copy the rest of the format
				 * string to the destination.
				 */

				strlcat( destination, format_ptr,
						maximum_length );

				bytes_written = strlen( destination );

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
				MX_DEBUG(-2,("%s: RETURN 2, destination = '%s'",
					fname, destination));
#endif

				return bytes_written;
			}

			/* A conversion character was found.  We need
			 * to copy everything up to and including that
			 * character to a separate buffer that will be
			 * used by snprintf() to format the output.
			 */

			local_format_bytes = conversion_ptr - format_ptr + 1;
		}

		local_format = malloc( local_format_bytes + 1 );

		if ( local_format == NULL ) {
			(void) mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a "
			"%ld byte snprintf format array.",
				local_format_bytes + 1 );

			bytes_written = strlen( destination );

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
			MX_DEBUG(-2,("%s: RETURN 3, destination = '%s'",
				fname, destination));
#endif

			return bytes_written;
		}

		bytes_written = strlen( destination );

		snprintf_destination = destination + bytes_written;

		strlcpy( local_format, format_ptr, local_format_bytes+1 );

		local_format[local_format_bytes] = '\0';

		if ( percent_ptr[1] == '%' ) {

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
			MX_DEBUG(-2,("%s: MARKER 0, i = %ld", fname, i));
#endif
			/* Note: The unused trailing "" argument is to 
			 * suppress a warning from the GCC compiler on
			 * MacOS X that says:
			 *   "format not a string literal and no
			 *   format arguments"
			 */

			snprintf( snprintf_destination,
				maximum_length - bytes_written,
				local_format, "" );
		} else
		if ( i < num_argument_pointers )
		{
			argument_pointer = argument_pointer_array[i];

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
			MX_DEBUG(-2,
			("%s: MARKER 1, i = %ld, argument_pointer = %p",
				fname, i, argument_pointer));
#endif

			/* For non-string fields, we must give snprintf()
			 * the value of the item to be printed, instead
			 * of the pointer to the value.
			 */

			switch( *conversion_ptr ) {
			case 's':	/* string */
			case 'p':	/* pointer */

				snprintf( snprintf_destination,
					maximum_length - bytes_written,
					local_format,
					argument_pointer );
				break;

			case 'a':	/* double */
			case 'A':
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				double_value = *( (double *) argument_pointer );

				snprintf( snprintf_destination,
					maximum_length - bytes_written,
					local_format,
					double_value );
				break;

			case 'c':	/* char */
				char_value = *( (char *) argument_pointer );

				snprintf( snprintf_destination,
					maximum_length - bytes_written,
					local_format,
					char_value );
				break;

			case 'd':	/* int */
			case 'i':
				switch( *(conversion_ptr-1) ) {
				case 'h':

				    switch ( *(conversion_ptr-2) ) {
				    case 'h':
					char_value =
					    *( (char *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						char_value );
					break;

				    default:
					short_value =
					    *( (short *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						short_value );
					break;
				}
				break;

				case 'l':

				    switch ( *(conversion_ptr-2) ) {
				    case 'l':
					int64_value =
					    *( (int64_t *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						int64_value );
					break;

				    default:
					long_value =
					    *( (long *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						long_value );
					break;
				}
				break;

				default:
				    int_value = *( (int *) argument_pointer );

				    snprintf( snprintf_destination,
					maximum_length - bytes_written,
					local_format,
					int_value );
				}
				break;

			case 'u':	/* unsigned int */
			case 'o':
			case 'x':
			case 'X':
				switch( *(conversion_ptr-1) ) {
				case 'h':

				    switch ( *(conversion_ptr-2) ) {
				    case 'h':
					uchar_value =
					*( (unsigned char *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						uchar_value );
					break;

				    default:
					ushort_value =
				      *( (unsigned short *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						ushort_value );
					break;
				}
				break;

				case 'l':

				    switch ( *(conversion_ptr-2) ) {
				    case 'l':
					uint64_value =
				  *( (uint64_t *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						uint64_value );
					break;

				    default:
					ulong_value =
					*( (unsigned long *) argument_pointer );

					snprintf( snprintf_destination,
						maximum_length - bytes_written,
						local_format,
						ulong_value );
					break;
				}
				break;

				default:
				    uint_value =
					*( (unsigned int *) argument_pointer );

				    snprintf( snprintf_destination,
					maximum_length - bytes_written,
					local_format,
					uint_value );
				}
				break;

			}

			i++;
		} else {
			/* i >= num_argument_pointers */

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
			MX_DEBUG(-2,("%s: MARKER 2, i = %ld", fname, i));
#endif

			strlcat( destination, format_ptr, maximum_length );

			mx_free( local_format );

			break;		/* Exit the while() loop. */
		}

		mx_free( local_format );

		format_ptr += local_format_bytes;

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
		MX_DEBUG(-2,
		("%s: LOOP, destination = '%s'", fname, destination));
#endif
	}

	bytes_written = strlen( destination );

#if MX_DEBUG_SNPRINTF_FROM_POINTER_ARRAY
	MX_DEBUG(-2,("%s: RETURN X, destination = '%s'", fname, destination));
#endif

	return bytes_written;
}

/*-------------------------------------------------------------------------*/

/* mx_strncmp_end() is analogous to strncmp(), but it compares its second
 * argument to the _end_ of the first argument instead of the beginning.
 */

MX_EXPORT int
mx_strncmp_end( const char *s1, const char *s2, size_t compare_length )
{
	const char *s1_offset;
	size_t length1;
	int result;

	length1 = strlen(s1);

	if ( compare_length > length1 ) {
		result = strcmp( s1, s2 );
	} else {
		s1_offset = s1 + ( length1 - compare_length );

		result = strncmp( s1_offset, s2, compare_length );
	}

	return result;
}

/*-------------------------------------------------------------------------*/

/* On Windows, the use of mx_fopen() rather than fopen() directly makes sure
 * that the FILE pointer returned makes use of the stdio variables found
 * in the libMx DLL.  On most other platforms, mx_fopen() behaves just like
 * fopen(), since that is what the function is calling directly.
 *
 * Certain other plaforms, such as OpenVMS and some RTOSes, have unusual
 * variants on fopen().  However, we have not yet written the code to handle
 * these special cases.
 */

#if 1

MX_EXPORT FILE *
mx_fopen( const char *pathname, const char *mode )
{
	FILE *file = fopen( pathname, mode );

	return file;
}

MX_EXPORT int
mx_fclose( FILE *stream )
{
	int fclose_status = fclose( stream );

	return fclose_status;
}

#elif 0
#error mx_fopen() has not yet been implemented for this platform.
#endif

/*-------------------------------------------------------------------------*/

/* mx_fgets() is a replacement for fgets() that automatically trims off
 * any trailing newline character.
 *
 * mx_fgets() also serves as a way of suppressing the warning in
 * recent versions of Ubuntu about ignoring the return value of fgets().
 */

MX_EXPORT char *
mx_fgets( char *buffer, int buffer_size, FILE *stream )
{
	char *ptr;
	size_t length;

	ptr = fgets( buffer, buffer_size, stream );

	if ( ptr == NULL )
		return ptr;

	/* Make sure that the buffer is null terminated. */

	ptr[ buffer_size - 1 ] = '\0';

	/* Delete a trailing newline <NL> or <CR><NL> pair if present. */

	length = strlen( ptr );

	if ( length > 0 ) {
		if ( ptr[ length - 1 ] == '\n' ) {
			ptr[ length - 1 ] = '\0';

			if ( length > 1 ) {
				if ( ptr[ length - 2 ] == '\r' ) {
					ptr[ length - 2 ] = '\0';
				}
			}
		}
	}

	return ptr;
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

MX_EXPORT int
mx_get_true_or_false( char *true_false_string )
{
	static const char fname[] = "mx_get_true_or_false()";

	char *duplicate;
	int argc;
	char **argv;
	int result;

	duplicate = strdup( true_false_string );

	if ( duplicate == NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to duplicate the string '%s'.",
			true_false_string );

		return FALSE;
	}

	mx_string_split( duplicate, " ,", &argc, &argv );

	if ( argc < 1 ) {
		mx_free( argv ); mx_free( duplicate );

		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The supplied string '%s' did not contain any non-blank "
		"characters in it.", true_false_string );

		result = FALSE;
	} else
	if ( mx_strcasecmp( "0", argv[0] ) == 0 ) {
		result = FALSE;
	} else
	if ( mx_strcasecmp( "1", argv[0] ) == 0 ) {
		result = TRUE;
	} else
	if ( mx_strcasecmp( "off", argv[0] ) == 0 ) {
		result = FALSE;
	} else
	if ( mx_strcasecmp( "on", argv[0] ) == 0 ) {
		result = TRUE;
	} else
	if ( mx_strcasecmp( "false", argv[0] ) == 0 ) {
		result = FALSE;
	} else
	if ( mx_strcasecmp( "true", argv[0] ) == 0 ) {
		result = TRUE;
	} else {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized truth value '%s' specified for "
		"true/false string '%s'.  "
		"The allowed values are '0', '1', 'off', 'on', "
		"'false', and 'true'.",
			argv[0], true_false_string );

		result = FALSE;
	}

	mx_free( argv ); mx_free( duplicate );

	return result;
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

				if ( (*ptr) == '\0' ) {
					/* This case will be invoked if the
					 * length of the string after the
					 * delimiter is 0.
					 */

					*string_ptr = NULL;

					return start_ptr;
				}

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
	void *temp_ptr;

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

	/* Note: array_size has +1 added to it to make sure that there
	 * is always enough space to NULL terminate the argv array.
	 */

	*argv = malloc( (array_size + 1) * sizeof(char**) );

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

			temp_ptr = realloc( *argv,
				(array_size + 1) * sizeof(char**) );

			if ( temp_ptr == NULL ) {
				/* realloc() failed, so we must free()
				 * the previous value of *argv.
				 */

				free( *argv );
				*argv = NULL;

				errno = ENOMEM;
				return -1;
			}

			*argv = temp_ptr;

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

#if ( defined(OS_WIN32) && (MX_WINVER >= 0x0501) && (!defined(MX_IS_REACTOS)) )

/* For Windows XP and after.  Internally, it depends on RtlGenRandom(). */

/* Somehow MinGW does not find the prototype, so we must put it in ourself. */

#if defined(__GNUC__)
   _CRTIMP int __cdecl rand_s(unsigned int *randomValue);
#endif

MX_EXPORT unsigned long
mx_random( void )
{
	static const char fname[] = "mx_random()";

	unsigned int random_value;
	int result;

	result = rand_s( &random_value );

	if ( result != 0 ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"rand_s() failed with error code = %d.", result );

		return 0;
	}

	return (unsigned long) random_value;
}

MX_EXPORT void
mx_seed_random( unsigned long seed )
{
	/* Do nothing */
}

MX_EXPORT unsigned long
mx_get_random_max( void )
{
	return UINT_MAX;
}

/*--------*/

#elif defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

MX_EXPORT unsigned long
mx_random( void )
{
	return arc4random();
}

MX_EXPORT void
mx_seed_random( unsigned long seed )
{
	/* Do nothing */
}

MX_EXPORT unsigned long
mx_get_random_max( void )
{
	return 0xffffffff;
}

/*--------*/

#elif defined(OS_LINUX) || defined(OS_SOLARIS) || defined(OS_CYGWIN) \
	|| defined(OS_QNX) || defined(OS_UNIXWARE) || defined(OS_DJGPP) \
	|| defined(OS_VMS) || defined(OS_HURD)

MX_EXPORT unsigned long
mx_random( void )
{
	return random();
}

MX_EXPORT void
mx_seed_random( unsigned long seed )
{
	srandom( (unsigned int) seed );
}

MX_EXPORT unsigned long
mx_get_random_max( void )
{
	return RAND_MAX;
}

/*--------*/

#elif defined(OS_WIN32) || defined(OS_RTEMS) || defined(OS_VXWORKS)

/* Used by: Win32 before Windows XP. */

/* WARNING: rand() and srand() are really not that random in general.
 *          You should not use them unless nothing else is available.
 */

MX_EXPORT unsigned long
mx_random( void )
{
	return rand();
}

MX_EXPORT void
mx_seed_random( unsigned long seed )
{
	srand( (unsigned int) seed );
}

MX_EXPORT unsigned long
mx_get_random_max( void )
{
	return RAND_MAX;
}

/*--------*/

#else
#error The MX random number functions are not yet implemented for this platform.
#endif

/*------------------------------------------------------------------------*/

