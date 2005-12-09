/*
 * Name:    mx_os_version.c
 *
 * Purpose: Report the operating system version.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"

static void
mx_split_version_number_string( char *version_number_string,
				int *os_major, int *os_minor, int *os_update )
{
	char *ptr_major, *ptr_minor, *ptr_update;

	if ( ( version_number_string == NULL ) || ( os_major == NULL )
		|| ( os_minor == NULL ) || ( os_update == NULL ) )
	{
		return;
	}

	ptr_major = version_number_string;

	/* Split the reported release string at the period characters. */

	ptr_minor = strchr( ptr_major, '.' );

	if ( ptr_minor == NULL ) {
		*os_major = atoi( ptr_major );
		*os_minor = 0;
		*os_update = 0;

		return;
	}

	*ptr_minor = '\0';	/* Split the string here into two strings. */
	ptr_minor++;		/* Point 'ptr_minor' to the second string. */

	ptr_update = strchr( ptr_minor, '.' );

	if ( ptr_update == NULL ) {
		*os_major = atoi( ptr_major );
		*os_minor = atoi( ptr_minor );
		*os_update = 0;

		return;
	}

	*ptr_update = '\0';	/* Split the string here into two strings. */
	ptr_update++;		/* Point 'ptr_update' to the second string. */

	*os_major = atoi( ptr_major );
	*os_minor = atoi( ptr_minor );
	*os_update = 0;

	return;
}

/*------------------------------------------------------------------------*/

#if defined( OS_VXWORKS )

#include "version.h"

MX_EXPORT mx_status_type
mx_get_os_version_string( char *version_string,
			size_t max_version_string_length )
{
	static const char fname[] = "mx_get_os_version_string()";

	if ( version_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The version_string pointer passed was NULL." );
	}

	snprintf( version_string, max_version_string_length,
		"VxWorks %s", VXWORKS_VERSION );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_os_version( int *os_major, int *os_minor, int *os_update )
{
	static const char fname[] = "mx_get_os_version()";

	if ( (os_major == NULL) || (os_minor == NULL) || (os_update == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One or more of the arguments passed were NULL." );
	}

	mx_split_version_number_string( VXWORKS_VERSION,
				os_major, os_minor, os_update );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

#elif defined( OS_UNIX ) || defined( OS_RTEMS )

#include <sys/utsname.h>

MX_EXPORT mx_status_type
mx_get_os_version_string( char *version_string,
			size_t max_version_string_length )
{
	static const char fname[] = "mx_get_os_version_string()";

	struct utsname uname_struct;
	int status, saved_errno;

	if ( version_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The version_string pointer passed was NULL." );
	}

	status = uname( &uname_struct );

	if ( status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"uname() failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

	snprintf( version_string, max_version_string_length,
		"%s %s", uname_struct.sysname, uname_struct.release );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_os_version( int *os_major, int *os_minor, int *os_update )
{
	static const char fname[] = "mx_get_os_version()";

	struct utsname uname_struct;
	int status, saved_errno;

	if ( (os_major == NULL) || (os_minor == NULL) || (os_update == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One or more of the arguments passed were NULL." );
	}

	status = uname( &uname_struct );

	if ( status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"uname() failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

#if 0
	MX_DEBUG(-2,("%s: sysname = '%s'",
			fname, uname_struct.sysname));
	MX_DEBUG(-2,("%s: nodename = '%s'",
			fname, uname_struct.nodename));
	MX_DEBUG(-2,("%s: release = '%s'",
			fname, uname_struct.release));
	MX_DEBUG(-2,("%s: version = '%s'",
			fname, uname_struct.version));
	MX_DEBUG(-2,("%s: machine = '%s'",
			fname, uname_struct.machine));
#endif

	mx_split_version_number_string( uname_struct.release,
				os_major, os_minor, os_update );

	return MX_SUCCESSFUL_RESULT;
}

#else

#error Reporting the operating system version has not been implemented for this platform.

#endif

