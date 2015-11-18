/*
 * Name:    mx_os_version.c
 *
 * Purpose: Report the operating system version.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2005, 2009-2010, 2012, 2014 Illinois Institute of Technology
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
#include "mx_dynamic_library.h"

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

#if ( defined(_MSC_VER) && (_MSC_VER > 1200) )
# define HAVE_OSVERSIONINFOEX	TRUE
#elif ( defined(__BORLANDC__) || defined(__GNUC__) )
# define HAVE_OSVERSIONINFOEX	TRUE
#else
# define HAVE_OSVERSIONINFOEX	FALSE
#endif

#include <windows.h>

/*----*/

MX_EXPORT mx_status_type
mx_win32_get_osversioninfo( unsigned long *win32_major_version,
			unsigned long *win32_minor_version,
			unsigned long *win32_platform_id,
			unsigned char *win32_product_type )
{
	static const char fname[] = "mx_win32_get_osversioninfo()";

#if HAVE_OSVERSIONINFOEX
	OSVERSIONINFOEX osvi;
#else
	OSVERSIONINFO osvi;
#endif
	BOOL status;
	int use_extended_struct;
	mx_status_type mx_status;

	/* If this is an NT version of Windows, try RtlGetVersion(). */

	if ( TRUE ) {
		typedef NTSTATUS (*RtlGetVersion_type)( RTL_OSVERSIONINFOW * );
		RtlGetVersion_type ptr_RtlGetVersion = NULL;

		RTL_OSVERSIONINFOW rtl_osvi;
		NTSTATUS nt_status;
		void *void_ptr;

		mx_status = mx_dynamic_library_get_library_and_symbol(
				"ntdll.dll", "RtlGetVersion",
				NULL, &void_ptr );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ptr_RtlGetVersion = void_ptr;

		memset( &rtl_osvi, 0, sizeof(RTL_OSVERSIONINFOW) );

		rtl_osvi.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

		nt_status = ptr_RtlGetVersion( &rtl_osvi );

		if ( nt_status != 0 ) {
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"RtlGetVersion() returned error code %d", nt_status );
		}

		*win32_major_version = rtl_osvi.dwMajorVersion;
		*win32_minor_version = rtl_osvi.dwMinorVersion;
		*win32_platform_id   = rtl_osvi.dwPlatformId;

		*win32_product_type  = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Try using OSVERSIONINFOEX next. */

#if HAVE_OSVERSIONINFOEX
	memset( &osvi, 0, sizeof(OSVERSIONINFOEX) );

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	status = GetVersionEx( (OSVERSIONINFO *) &osvi );
#else
	status = 0;
#endif

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
	MX_DEBUG(-2,("%s: dwBuildNumber = %lu",
		fname, (unsigned long) osvi.dwBuildNumber));
	MX_DEBUG(-2,("%s: dwPlatformId = %lu",
		fname, (unsigned long) osvi.dwPlatformId));
	MX_DEBUG(-2,("%s: szCSDVersion = '%s'",
		fname, osvi.szCSDVersion));

#if HAVE_OSVERSIONINFOEX

	MX_DEBUG(-2,("%s: wServicePackMajor = %lu",
		fname, (unsigned long) osvi.wServicePackMajor));
	MX_DEBUG(-2,("%s: wServicePackMinor = %lu",
		fname, (unsigned long) osvi.wServicePackMinor));
	MX_DEBUG(-2,("%s: wSuiteMask = %#lx",
		fname, (unsigned long) osvi.wSuiteMask));
	MX_DEBUG(-2,("%s: wProductType = %lu",
		fname, (unsigned long) osvi.wProductType));
	MX_DEBUG(-2,("%s: wReserved = %lu",
		fname, (unsigned long) osvi.wReserved));

#endif /* HAVE_OSVERSIONINFOEX */

#endif

	*win32_major_version = osvi.dwMajorVersion;
	*win32_minor_version = osvi.dwMinorVersion;
	*win32_platform_id   = osvi.dwPlatformId;

#if HAVE_OSVERSIONINFOEX
	*win32_product_type  = osvi.wProductType;
#else
	*win32_product_type  = 0;
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_win32_is_windows_9x( int *is_windows_9x )
{
	static const char fname[] = "mx_win32_is_windows_9x()";

	unsigned long major, minor, platform_id;
	unsigned char product_type;
	mx_status_type mx_status;

	if ( is_windows_9x == (int *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The is_windows_9x pointer passed was NULL." );
	}

	mx_status = mx_win32_get_osversioninfo( &major, &minor,
						&platform_id, &product_type );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( major ) {
	case 4:
		switch( minor ) {
		case 0:
		case 1:
		case 3:
			if ( platform_id == VER_PLATFORM_WIN32_NT ) {
				*is_windows_9x = FALSE;
			} else {
				*is_windows_9x = TRUE;
			}
			break;
		case 10:
		case 90:
			*is_windows_9x = TRUE;
			break;
		default:
			return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
			"Unrecognized Windows operating system type." );
			break;
		}
		break;
	default:
		*is_windows_9x = FALSE;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mx_get_os_version_string( char *version_string,
			size_t max_version_string_length )
{
	static const char fname[] = "mx_get_os_version_string()";

	DWORD win32_major_version, win32_minor_version, win32_platform_id;
	BYTE  win32_product_type;
	mx_status_type mx_status;

	if ( version_string == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The version_string pointer passed was NULL." );
	}

	mx_status = mx_win32_get_osversioninfo( &win32_major_version,
						&win32_minor_version,
						&win32_platform_id,
						&win32_product_type );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* A twisty little maze of passages, all alike. */

	switch( win32_major_version ) {
	case 3:
		strlcpy( version_string, "Windows NT 3.51",
			max_version_string_length );
		break;
	case 4:
		switch( win32_minor_version ) {
		case 0:
		case 1:
		case 3:
			if ( win32_platform_id == VER_PLATFORM_WIN32_NT ) {
				strlcpy( version_string, "Windows NT 4.0",
					max_version_string_length );
			} else {
				strlcpy( version_string, "Windows 95",
					max_version_string_length );
			}
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
				(unsigned long) win32_major_version,
				(unsigned long) win32_minor_version );
			break;
		}
		break;
	case 5:
		switch( win32_minor_version ) {
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
				(unsigned long) win32_major_version,
				(unsigned long) win32_minor_version );
			break;
		}
		break;
	case 6:
		switch( win32_minor_version ) {

#if defined(VER_NT_WORKSTATION)
		case 0:
			if ( win32_product_type == VER_NT_WORKSTATION ) {
				strlcpy( version_string, "Windows Vista",
					max_version_string_length );
			} else {
				strlcpy( version_string, "Windows Server 2008",
					max_version_string_length );
			}
			break;
		case 1:
			if ( win32_product_type == VER_NT_WORKSTATION ) {
				strlcpy( version_string, "Windows 7",
					max_version_string_length );
			} else {
				strlcpy( version_string,
					"Windows Server 2008 R2",
					max_version_string_length );
			}
			break;
		case 2:
			if ( win32_product_type == VER_NT_WORKSTATION ) {
				strlcpy( version_string, "Windows 8",
					max_version_string_length );
			} else {
				strlcpy( version_string, "Windows Server 2012",
					max_version_string_length );
			}
		case 3:
			strlcpy( version_string, "Windows 8.1",
					max_version_string_length );
			break;

#else /* !defined(VER_NT_WORKSTATION) */
		case 0:
			strlcpy( version_string, "Windows Vista",
					max_version_string_length );
			break;
		case 1:
			strlcpy( version_string, "Windows 7",
					max_version_string_length );
			break;

#endif /* !defined(VER_NT_WORKSTATION) */

		default:
			snprintf( version_string, max_version_string_length,
				"Windows ??? (major = %lu, minor = %lu)",
				(unsigned long) win32_major_version,
				(unsigned long) win32_minor_version );
			break;
		}
		break;

	case 10:
		switch( win32_minor_version ) {
		case 0:
			strlcpy( version_string, "Windows 10",
					max_version_string_length );
			break;
		default:
			snprintf( version_string, max_version_string_length,
				"Windows 10.%d", win32_minor_version );
			break;
		}
		break;

	default:
		snprintf( version_string, max_version_string_length,
			"Windows ??? (major = %lu, minor = %lu)",
			(unsigned long) win32_major_version,
			(unsigned long) win32_minor_version );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_os_version( int *os_major, int *os_minor, int *os_update )
{
	static const char fname[] = "mx_get_os_version()";

	DWORD win32_major_version, win32_minor_version, win32_platform_id;
	BYTE win32_product_type;
	mx_status_type mx_status;

	if ( (os_major == NULL) || (os_minor == NULL) || (os_update == NULL) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One or more of the arguments passed were NULL." );
	}

	mx_status = mx_win32_get_osversioninfo( &win32_major_version,
						&win32_minor_version,
						&win32_platform_id,
						&win32_product_type );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*os_major = (int) win32_major_version;

	*os_minor = (int) win32_minor_version;

#if defined(VER_NT_WORKSTATION)
	*os_update = win32_product_type;
#else
	*os_update = win32_platform_id;
#endif

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
	|| defined( OS_RTEMS ) || defined( OS_ECOS )

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

#if defined(OS_UNIXWARE)
	snprintf( version_string, max_version_string_length,
		"%s %s", uname_struct.sysname, uname_struct.version );
#else
	snprintf( version_string, max_version_string_length,
		"%s %s", uname_struct.sysname, uname_struct.release );
#endif

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

/*--------------------------------------------------------------------------*/

MX_EXPORT unsigned long
mx_get_os_version_number( void )
{
	int os_major, os_minor, os_update;
	unsigned long os_version_number;
	mx_status_type mx_status;

	mx_status = mx_get_os_version( &os_major, &os_minor, &os_update );

	if ( mx_status.code != MXE_SUCCESS ) {
		return 0;
	}

	os_version_number = ( 1000000L * os_major )
				+ ( 1000L * os_minor )
				+ os_update;

	return os_version_number;
}

