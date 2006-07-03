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

/****************************************************************************/

#if defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_get_cpu_architecture( char *architecture_type,
			size_t max_architecture_type_length,
			char *architecture_subtype,
			size_t max_architecture_subtype_length )
{
	SYSTEM_INFO sysinfo;

	if ( architecture_type != NULL ) {
		GetSystemInfo( &sysinfo );

		switch( sysinfo.wProcessorArchitecture ) {

#if defined( PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 )
		case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
#endif
		case PROCESSOR_ARCHITECTURE_INTEL:
			strlcpy( architecture_type, "i386",
				max_architecture_type_length );
			break;

#if defined( PROCESSOR_ARCHITECTURE_AMD64 )
		case PROCESSOR_ARCHITECTURE_AMD64:
			strlcpy( architecture_type, "amd64",
				max_architecture_type_length );
			break;
#endif

		case PROCESSOR_ARCHITECTURE_IA64:
			strlcpy( architecture_type, "ia64",
				max_architecture_type_length );
			break;

		case PROCESSOR_ARCHITECTURE_UNKNOWN:
			strlcpy( architecture_type, "unknown",
				max_architecture_type_length );
			break;

		default:
			strlcpy( architecture_type, "illegal",
				max_architecture_type_length );
			break;

		}
	}

	if ( architecture_subtype != NULL ) {
		strlcpy( architecture_subtype, "",
				max_architecture_subtype_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/****************************************************************************/

#elif defined(OS_VXWORKS)

MX_EXPORT mx_status_type
mx_get_cpu_architecture( char *architecture_type,
			size_t max_architecture_type_length,
			char *architecture_subtype,
			size_t max_architecture_subtype_length )
{
	if ( architecture_type != NULL ) {

#if ( CPU_FAMILY == ARM )
	    strlcpy( architecture_type, "arm", max_architecture_type_length );
#elif ( CPU_FAMILY == I80X86 ) || ( CPU_FAMILY == SIMNT )
	    strlcpy( architecture_type, "i386", max_architecture_type_length );
#elif ( CPU_FAMILY == I960 )
	    strlcpy( architecture_type, "i960", max_architecture_type_length );
#elif ( CPU_FAMILY == MC680X0 )
	    strlcpy( architecture_type, "m68k", max_architecture_type_length );
#elif ( CPU_FAMILY == MIPS )
	    strlcpy( architecture_type, "mips", max_architecture_type_length );
#elif ( CPU_FAMILY == SIMHPPA )
	    strlcpy( architecture_type, "parisc", max_architecture_type_length);
#elif ( CPU_FAMILY == PPC )
	    strlcpy( architecture_type, "powerpc",
	    			max_architecture_type_length );
#elif ( CPU_FAMILY == SPARC ) \
	|| ( CPU_FAMILY == SIMSPARCSUNOS ) || ( CPU_FAMILY == SIMSPARCSOLARIS )
	    strlcpy( architecture_type, "sparc", max_architecture_type_length );
#else
#     error CPU architecture type not detected.
#endif
	}

	if ( architecture_subtype != NULL ) {
		strlcpy( architecture_subtype, "",
				max_architecture_subtype_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/****************************************************************************/

#elif defined(OS_VMS)

MX_EXPORT mx_status_type
mx_get_cpu_architecture( char *architecture_type,
			size_t max_architecture_type_length,
			char *architecture_subtype,
			size_t max_architecture_subtype_length )
{
	static const char fname[] = "mx_get_cpu_architecture()";

	if ( architecture_type != NULL ) {
#if defined(__alpha)
		strlcpy( architecture_type, "alpha",
				max_architecture_type_length );
#elif defined(__ia64)
		strlcpy( architecture_type, "ia64",
				max_architecture_type_length );
#elif defined(__vax)
		strlcpy( architecture_type, "vax",
				max_architecture_type_length );
#else
#     error CPU architecture type not detected.
#endif
	}

	if ( architecture_subtype != NULL ) {
		strlcpy( architecture_subtype, "",
				max_architecture_subtype_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

/****************************************************************************/

#elif defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_DJGPP) \
	|| defined(OS_ECOS) || defined(OS_RTEMS)

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

#  if defined(__alpha__) || defined(__alpha)

		strlcpy( architecture_type, "alpha",
				max_architecture_type_length );

#  elif defined(__x86_64__) || defined(__x86_64)

		strlcpy( architecture_type, "amd64",
				max_architecture_type_length );

#  elif defined(__i386__) || defined(__i386)

		strlcpy( architecture_type, "i386",
				max_architecture_type_length );

#  elif defined(__m68k__)

		strlcpy( architecture_type, "m68k",
				max_architecture_type_length );

#  elif defined(__mips__) || defined(__mips)

		strlcpy( architecture_type, "mips",
				max_architecture_type_length );

#  elif defined(__powerpc__) || defined(__ppc__)

		strlcpy( architecture_type, "powerpc",
				max_architecture_type_length );

#  elif defined(__s390__)

		strlcpy( architecture_type, "s390",
				max_architecture_type_length );

#  elif defined(__sparc__) || defined(__sparc)

		strlcpy( architecture_type, "sparc",
				max_architecture_type_length );

#  elif defined(__vax__) || defined(__vax)

		strlcpy( architecture_type, "vax",
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

#error Reporting the CPU architecture has not been implemented for this platform.

#endif

