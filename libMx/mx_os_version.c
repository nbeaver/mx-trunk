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

#if defined( OS_UNIX )

#include <sys/utsname.h>

MX_EXPORT mx_status_type
mx_get_os_version_string( char *version_string,
			size_t max_version_string_length )
{
	static const char fname[] = "mx_get_os_version_string()";

	struct utsname uname_struct;
	int status, saved_errno;

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
	char *ptr;

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

	/* Find the period in the release name.  The part before
	 * the period is the OS major version, while the part
	 * after the period is the OS minor version.
	 */

	ptr = strchr( uname_struct.release, '.' );

	if ( ptr == NULL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The SunOS release string '%s' returned by the "
		"operating system does not have a '.' character "
		"in it.  How can this happen?", uname_struct.release );
	}

	*ptr = '\0';	/* Split the string into two strings. */
	ptr++;		/* Point 'ptr' to the second string. */

	*os_major = atoi( uname_struct.release );
	*os_minor = atoi( ptr );
	*os_update = 0;

	return MX_SUCCESSFUL_RESULT;
}

#else

#error Reporting the operating system version has not been implemented for this platform.

#endif

