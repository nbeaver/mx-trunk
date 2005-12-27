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

#if !defined( OS_WIN32 )

static void
mx_split_version_number_string( char *version_number_string,
				int *os_major, int *os_minor, int *os_update )
{
	char version_buffer[100];
	char *ptr_major, *ptr_minor, *ptr_update;

	if ( ( version_number_string == NULL ) || ( os_major == NULL )
		|| ( os_minor == NULL ) || ( os_update == NULL ) )
	{
		return;
	}

	strlcpy(version_buffer, version_number_string, sizeof(version_buffer));

	ptr_major = version_buffer;

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
	*os_update = atoi( ptr_update );

	return;
}
#endif

/*------------------------------------------------------------------------*/

#if defined( OS_WIN32 )

#include <windows.h>

MX_EXPORT mx_status_type
mx_get_os_version_string( char *version_string,
			size_t max_version_string_length )
{
	static const char fname[] = "mx_get_os_version_string()";

	OSVERSIONINFOEX osvi;
	BOOL status;
	int use_extended_struct;

	if ( version_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The version_string pointer passed was NULL." );
	}

	/* Try using OSVERSIONINFOEX first. */

	memset( &osvi, 0, sizeof(OSVERSIONINFOEX) );

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	status = GetVersionEx( (OSVERSIONINFO *) &osvi );

	if ( status != 0 ) {
		use_extended_struct = TRUE;
	} else {
		use_extended_struct = FALSE;

		/* Using OSVERSIONINFOEX failed, so try again
		 * with OSVERSIONINFO.
		 */

		memset( &osvi, 0, sizeof(OSVERSIONINFO) );

		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		status = GetVersionEx( (OSVERSIONINFO *) &osvi );

		if ( status == 0 ) {
			TCHAR message_buffer[100];
			DWORD error_code;

			error_code = GetLastError();

			mx_win32_error_message( error_code,
				message_buffer, sizeof( message_buffer ) );

			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unable to get the Windows version number.  "
			"Win32 error code = %ld, error message = '%s'.",
			error_code, message_buffer );
		}
	}

#if 0
	MX_DEBUG(-2,("%s: dwMajorVersion = %lu",
		fname, (unsigned long) osvi.dwMajorVersion));
	MX_DEBUG(-2,("%s: dwMinorVersion = %lu",
		fname, (unsigned long) osvi.dwMinorVersion));
	MX_DEBUG(-2,("%s: szCSDVersion = '%s'",
		fname, osvi.szCSDVersion));
	MX_DEBUG(-2,("%s: wSuiteMask = %#lx",
		fname, (unsigned long) osvi.wSuiteMask));
#endif

	/* A twisty little maze of passages, all alike. */

	switch( osvi.dwMajorVersion ) {
	case 3:
		strlcpy( version_string, "Windows NT 3.51",
			max_version_string_length );
		break;
	case 4:
		switch( osvi.dwMinorVersion ) {
		case 0:
			strlcpy( version_string, "Windows 95",
				max_version_string_length );
			break;
		case 1:
		case 3:
			strlcpy( version_string, "Windows NT 4.0",
				max_version_string_length );
			break;
		case 10:
			strlcpy( version_string, "Windows 98",
				max_version_string_length );
			break;
		case 90:
			strlcpy( version_string, "Windows Me",
				max_version_string_length );
			break;
		default:
			snprintf( version_string, max_version_string_length,
				"Windows ??? (major = %lu, minor = %lu)",
				(unsigned long) osvi.dwMajorVersion,
				(unsigned long) osvi.dwMinorVersion );
			break;
		}
		break;
	case 5:
		switch( osvi.dwMinorVersion ) {
		case 0:
			strlcpy( version_string, "Windows 2000",
				max_version_string_length );
			break;
		case 1:
			strlcpy( version_string, "Windows XP",
				max_version_string_length );
			break;
		case 2:
			strlcpy( version_string, "Windows Server 2003",
				max_version_string_length );
			break;
		default:
			snprintf( version_string, max_version_string_length,
				"Windows ??? (major = %lu, minor = %lu)",
				(unsigned long) osvi.dwMajorVersion,
				(unsigned long) osvi.dwMinorVersion );
			break;
		}
		break;
	case 6:
		switch( osvi.dwMinorVersion ) {
		case 0:
			strlcpy( version_string, "Windows Vista",
				max_version_string_length );
			break;
		default:
			snprintf( version_string, max_version_string_length,
				"Windows ??? (major = %lu, minor = %lu)",
				(unsigned long) osvi.dwMajorVersion,
				(unsigned long) osvi.dwMinorVersion );
			break;
		}
		break;
	default:
		snprintf( version_string, max_version_string_length,
			"Windows ??? (major = %lu, minor = %lu)",
			(unsigned long) osvi.dwMajorVersion,
			(unsigned long) osvi.dwMinorVersion );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_os_version( int *os_major, int *os_minor, int *os_update )
{
	static const char fname[] = "mx_get_os_version()";

	OSVERSIONINFO osvi;
	BOOL status;

	if ( (os_major == NULL) || (os_minor == NULL) || (os_update == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One or more of the arguments passed were NULL." );
	}

	memset( &osvi, 0, sizeof(osvi) );

	osvi.dwOSVersionInfoSize = sizeof(osvi);

	status = GetVersionEx( (OSVERSIONINFO *) &osvi );

	if ( status == 0 ) {
		TCHAR message_buffer[100];
		DWORD error_code;

		error_code = GetLastError();

		mx_win32_error_message( error_code,
			message_buffer, sizeof( message_buffer ) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get the Windows version number.  "
		"Win32 error code = %ld, error message = '%s'.",
		error_code, message_buffer );
	}

	*os_major = (int) osvi.dwMajorVersion;
	*os_minor = (int) osvi.dwMinorVersion;
	*os_update = 0;

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

#elif defined( OS_VMS )

#include <ssdef.h>
#include <stsdef.h>
#include <syidef.h>
#include <descrip.h>
#include <lib$routines.h>

static void
mx_get_vms_version_string( char *vms_version_buffer,
				size_t max_vms_version_buffer_length )
{
	static const char fname[] = "mx_get_vms_version_string()";

	long item_code;
	unsigned long vms_status;

	struct dsc$descriptor_s version_desc;

	version_desc.dsc$b_dtype = DSC$K_DTYPE_T;
	version_desc.dsc$b_class = DSC$K_CLASS_S;
	version_desc.dsc$a_pointer = vms_version_buffer;
	version_desc.dsc$w_length = max_vms_version_buffer_length - 1;

	item_code = SYI$_VERSION;

	vms_status = lib$getsyi( &item_code,
				0,
				&version_desc,
				&version_desc.dsc$w_length,
				0,
				0 );

	return;
}

MX_EXPORT mx_status_type
mx_get_os_version_string( char *version_string,
			size_t max_version_string_length )
{
	static const char fname[] = "mx_get_os_version_string()";

	char vms_version_buffer[20];
	char *ptr, *terminator_ptr;
	int os_major, os_minor, os_update;

	if ( version_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The version_string pointer passed was NULL." );
	}

	mx_get_vms_version_string( vms_version_buffer,
					sizeof(vms_version_buffer) );

	ptr = vms_version_buffer;

	if ( *ptr == 'V' ) {
		ptr++;
	}

	/* Null terminate the string at the first trailing space character. */

	terminator_ptr = strchr( ptr, ' ' );

	*terminator_ptr = '\0';

	mx_split_version_number_string( ptr,
				&os_major, &os_minor, &os_update );

	if ( os_major >= 6 ) {

		snprintf( version_string, max_version_string_length,
			"OpenVMS %s", ptr );
	} else {
		snprintf( version_string, max_version_string_length,
			"VMS %s", ptr );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_os_version( int *os_major, int *os_minor, int *os_update )
{
	static const char fname[] = "mx_get_os_version()";

	char vms_version_buffer[20];
	char *ptr;

	if ( (os_major == NULL) || (os_minor == NULL) || (os_update == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One or more of the arguments passed were NULL." );
	}

	mx_get_vms_version_string( vms_version_buffer,
					sizeof(vms_version_buffer) );

	ptr = vms_version_buffer;

	if ( *ptr == 'V' ) {
		ptr++;
	}

	mx_split_version_number_string( ptr,
				os_major, os_minor, os_update );

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------------*/

#elif defined( OS_VXWORKS )

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

#elif defined( OS_UNIX ) || defined( OS_CYGWIN ) || defined( OS_DJGPP ) \
	|| defined( OS_RTEMS )

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

#if defined( OS_CYGWIN )
	{
		/* Extract the version number from the release string. */

		int os_major, os_minor, os_update;

		mx_split_version_number_string( uname_struct.release,
				&os_major, &os_minor, &os_update );

		snprintf( version_string, max_version_string_length,
			"%s %d.%d.%d", uname_struct.sysname,
			os_major, os_minor, os_update );

		return MX_SUCCESSFUL_RESULT;
	}
#endif

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

