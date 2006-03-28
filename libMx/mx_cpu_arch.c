/*
 * Name:    mx_cpu_arch.c
 *
 * Purpose: Report the CPU architecture.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <errno.h>

#include "mx_osdef.h"

#include "mx_util.h"

#if defined(OS_UNIX)

#include <sys/utsname.h>

MX_EXPORT mx_status_type
mx_get_cpu_architecture( char *architecture_type,
			size_t max_architecture_type_length,
			char *architecture_subtype,
			size_t max_architecture_subtype_length )
{
	static const char fname[] = "mx_get_cpu_architecture()";

	struct utsname uname_struct;
	int status, saved_errno;

	status = uname( &uname_struct );

	if ( status < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"uname() failed.  Errno = %d, error message = '%s'",
			saved_errno, strerror( saved_errno ) );
	}

#if 1
	MX_DEBUG(-2,("%s: sysname = '%s'", fname, uname_struct.sysname));
	MX_DEBUG(-2,("%s: nodename = '%s'", fname, uname_struct.nodename));
	MX_DEBUG(-2,("%s: release = '%s'", fname, uname_struct.release));
	MX_DEBUG(-2,("%s: version = '%s'", fname, uname_struct.version));
	MX_DEBUG(-2,("%s: machine = '%s'", fname, uname_struct.machine));
#endif

	if ( architecture_type != NULL ) {

#  if defined(__i386__)
		strlcpy( architecture_type, "i386",
				max_architecture_type_length );

#  elif defined(__powerpc__) || defined(__ppc__)

		strlcpy( architecture_type, "powerpc",
				max_architecture_type_length );
#  elif defined(__mips)
		strlcpy( architecture_type, "mips",
				max_architecture_type_length );
#  else
#     error CPU architecture type not detected.
#  endif
	}

	if ( architecture_subtype != NULL ) {
		strlcpy( architecture_subtype, uname_struct.machine,
				max_architecture_subtype_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

#else

#error Reporting the operating system version has not been implemented for this platform.

#endif

