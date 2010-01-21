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
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>
#include <sys/time.h>

#include "mx_util.h"
#include "mx_time.h"

#if 0

  MX_EXPORT char *asctime_r( const struct tm *, char * );

  MX_EXPORT char *ctime_r( const time_t *, char * );

  MX_EXPORT struct tm *gmtime_r( const time_t *, struct tm * );

  MX_EXPORT struct tm *localtime_r( const time_t *, struct tm * );

#endif /* 0 */

/*------------ MX OS time reporting functions. ------------*/

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
