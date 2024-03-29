/*
 * Name:    mx_time.c
 *
 * Purpose: This file provides thread-safe Posix time functions for
 *          those platforms that do not have them and also adds
 *          some MX-specific functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010, 2015-2018, 2021-2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_NO_POISON

#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_time.h"

#if defined(OS_UNIX) || defined(OS_CYGWIN)
  #include <sys/time.h>
#endif

#if defined(OS_WIN32)
  #include <windows.h>
#endif

/* 26 is a magic number from the asctime_r and ctime_r man pages on Linux. */

#define MX_TIME_BUFFER_LENGTH	26

/*-------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_VMS) \
	|| defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_ANDROID) \
	|| defined(OS_MINIX)

  /* These platforms already provide the thread-safe Posix time functions. */

  #define MX_USE_LOCALTIME_R

/*- - - -*/

#elif defined(OS_WIN32)

  #if defined(_MSC_VER)

    #if (_MSC_VER >= 1400)
      #define MX_USE_LOCALTIME_S
    #else
      /* Apparently the msvcrt.dll DLLs used by Visual C++ 2003 and before
       * use thread local storage for the internal buffers of the functions,
       * which makes the functions thread safe on this platform.
       */
      #define MX_USE_LOCALTIME
    #endif

  #elif defined(__BORLANDC__) || defined(__GNUC__)
      /* Apparently the msvcrt.dll DLLs used by Borland and Mingw use thread
       * local storage for the internal buffers of the functions, which makes
       * the functions thread safe on these platforms.
       */
    #define MX_USE_LOCALTIME

  #else
    #error Unrecognized Win32 compiler.
  #endif

/*- - - -*/

#elif defined(OS_DJGPP)

  #define MX_USE_LOCALTIME

/*- - - -*/

#else
#error Thread-safe time functions have not been defined for this platform.
#endif

/*-------------------------------------------------------------------------*/

#if defined(MX_USE_LOCALTIME_R)

/* We do not need to do anything for this case, since the thread-safe
 * Posix time functions are already defined.
 */

/*-------------------------------------------------------------------------*/

#elif defined(MX_USE_LOCALTIME_S)

MX_EXPORT char *
asctime_r( const struct tm *tm_struct, char *buffer )
{
	errno_t os_status;

	os_status = asctime_s( buffer, MX_TIME_BUFFER_LENGTH, tm_struct );

	if ( os_status == 0 ) {
		return buffer;
	} else {
		errno = os_status;
		return NULL;
	}
}

MX_EXPORT char *
ctime_r( const time_t *time_struct, char *buffer )
{
	errno_t os_status;

	os_status = ctime_s( buffer, MX_TIME_BUFFER_LENGTH, time_struct );

	if ( os_status == 0 ) {
		return buffer;
	} else {
		errno = os_status;
		return NULL;
	}
}

MX_EXPORT struct tm *
gmtime_r( const time_t *time_struct, struct tm *tm_struct )
{
	errno_t os_status;

	os_status = gmtime_s( tm_struct, time_struct );

	if ( os_status == 0 ) {
		return tm_struct;
	} else {
		errno = os_status;
		return NULL;
	}
}

MX_EXPORT struct tm *
localtime_r( const time_t *time_struct, struct tm *tm_struct )
{
	errno_t os_status;

	os_status = localtime_s( tm_struct, time_struct );

	if ( os_status == 0 ) {
		return tm_struct;
	} else {
		errno = os_status;
		return NULL;
	}
}

/*-------------------------------------------------------------------------*/

#elif defined(MX_USE_LOCALTIME)

MX_EXPORT char *
asctime_r( const struct tm *tm_struct, char *buffer )
{
	char *ptr;

	if ( tm_struct == NULL ) {
		errno = EINVAL;

		return NULL;
	}
	
	ptr = asctime( tm_struct );

	if ( buffer != NULL ) {
		memcpy( buffer, ptr, MX_TIME_BUFFER_LENGTH );
	}

	return ptr;
}

MX_EXPORT char *
ctime_r( const time_t *time_struct, char *buffer )
{
	char *ptr;

	if ( time_struct == NULL ) {
		errno = EINVAL;

		return NULL;
	}
	
	ptr = ctime( time_struct );

	if ( buffer != NULL ) {
		memcpy( buffer, ptr, MX_TIME_BUFFER_LENGTH );
	}

	return ptr;
}

MX_EXPORT struct tm *
gmtime_r( const time_t *time_struct, struct tm *tm_struct )
{
	struct tm *ptr;

	if ( time_struct == NULL ) {
		errno = EINVAL;

		return NULL;
	}
	
	ptr = gmtime( time_struct );

	if ( tm_struct != NULL ) {
		memcpy( tm_struct, ptr, sizeof(struct tm) );
	}

	return ptr;
}

MX_EXPORT struct tm *
localtime_r( const time_t *time_struct, struct tm *tm_struct )
{
	struct tm *ptr;

	if ( time_struct == NULL ) {
		errno = EINVAL;

		return NULL;
	}
	
	ptr = localtime( time_struct );

	if ( tm_struct != NULL ) {
		memcpy( tm_struct, ptr, sizeof(struct tm) );
	}

	return ptr;
}

/*-------------------------------------------------------------------------*/

#else
#error Thread-safe time functions have not been defined for this platform.
#endif

/*=========================================================================*/

/*------------ MX OS time reporting functions. ------------*/

#if defined(OS_WIN32)

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

#if defined(OS_RTEMS)
#  include <sys/time.h>
#  include <unistd.h>
#endif

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
	struct tm tm_struct;
	char *ptr;
	char local_buffer[20];
	double nsec_in_seconds;

	if ( buffer == NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
			"The string buffer pointer passed was NULL." );

		return NULL;
	}

	time_in_seconds = os_time.tv_sec;

	localtime_r( &time_in_seconds, &tm_struct );

	strftime( buffer, buffer_length,
		"%a %b %d %Y %H:%M:%S", &tm_struct );

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
mx_ctime_tz_string( char *buffer, size_t buffer_length )
{
	/* This format generates a string that looks like the string
	 * generated with ctime(), but with timezone information as well.
	 */

#if ( defined( MX_GNUC_VERSION ) && ( MX_GNUC_VERSION < 3004000L ) )
	static const char format[] = "%a %b %d %H:%M:%S %Y";
#else
	static const char format[] = "%c %Z";
#endif

	time_t now;
	struct tm tm_struct;

	memset( &tm_struct, 0, sizeof(tm_struct) );

	now = time( NULL );

	localtime_r( &now, &tm_struct );

	strftime( buffer, buffer_length, format, &tm_struct );

	return buffer;
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

/* FIXME: The time() system call does not return a 64-bit time on 32-bit
 *        platforms.  We need to find a way to get the correct 64-bit time
 *        relative to the Posix epoch on such platforms to avoid the
 *        Y-2038 problem.
 */

#if defined(OS_WIN32) || defined(OS_LINUX) || defined(OS_MACOSX) \
	|| defined(OS_SOLARIS) || defined(OS_BSD) || defined(OS_CYGWIN) \
	|| defined(OS_VMS) || defined(OS_UNIXWARE) || defined(OS_DJGPP) \
	|| defined(OS_QNX) || defined(OS_RTEMS) || defined(OS_VXWORKS) \
	|| defined(OS_HURD) || defined(OS_ANDROID) || defined(OS_MINIX)

MX_EXPORT uint64_t
mx_posix_time( void ) {

	uint64_t posix_time;

	posix_time = time( NULL );

	return posix_time;
}

#else

#error mx_posix_time() is not yet defined for this platform.

#endif

/*--------------------------------------------------------------------------*/

MX_EXPORT struct timespec
mx_add_timespec_times( struct timespec time1, struct timespec time2 )
{
	struct timespec result;
	long nanoseconds;

	nanoseconds = time1.tv_nsec + time2.tv_nsec;

	result.tv_nsec = nanoseconds % 1000000000L;

	result.tv_sec = (time_t)
		( time1.tv_sec + time2.tv_sec + nanoseconds / 1000000000L );

	return result;
}

MX_EXPORT struct timespec
mx_subtract_timespec_times( struct timespec time1, struct timespec time2 )
{
	struct timespec result;

	if ( time1.tv_nsec < time2.tv_nsec ) {
		time1.tv_sec  -= 1L;
		time1.tv_nsec += 1000000000L;
	}

	result.tv_nsec = time1.tv_nsec - time2.tv_nsec;

	result.tv_sec = time1.tv_sec - time2.tv_sec;

	return result;
}

MX_EXPORT int
mx_compare_timespec_times( struct timespec time1, struct timespec time2 )
{
	unsigned long H1, H2, L1, L2;

	H1 = time1.tv_sec;
	H2 = time2.tv_sec;

	L1 = time1.tv_nsec;
	L2 = time2.tv_nsec;

	if ( H1 < H2 ) {
		return (-1);
	} else if ( H1 > H2 ) {
		return 1;
	} else {
		if ( L1 < L2 ) {
			return (-1);

		} else if ( L1 > L2 ) {
			return 1;

		} else {
			return 0;
		}
	}
}

MX_EXPORT struct timespec
mx_multiply_timespec_time( double multiplier,
			struct timespec original_time )
{
	struct timespec new_time;
	double new_seconds, new_nanoseconds, extra_seconds;

	new_seconds = multiplier * original_time.tv_sec;
	new_nanoseconds = multiplier * original_time.tv_nsec;

	if ( new_nanoseconds >= 1.0e9 ) {
		extra_seconds = (long) (new_nanoseconds / 1.0e9);

		new_seconds = new_seconds + extra_seconds;
		new_nanoseconds = fmod( new_nanoseconds, 1.0e9 );
	}

	new_time.tv_sec = mx_round( new_seconds );

	new_time.tv_nsec = mx_round( new_nanoseconds );

	return new_time;
}

MX_EXPORT struct timespec
mx_convert_seconds_to_timespec_time( double seconds )
{
	struct timespec result;

	result.tv_sec = (time_t) seconds;

	seconds -= (double) result.tv_sec;

	result.tv_nsec = (long) ( 1.0e9 * seconds );

	return result;
}

/*--------------------------------------------------------------------------*/
