/*
 * Name:    mx_private_version.c
 *
 * Purpose: This program is used to generate the mx_private_version.h
 *          file.  mx_private_version.h contains macro definitions for
 *          the version of MX in use as well as the versions of the
 *          compiler and runtime libraries.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008-2009, 2011, 2014-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#if defined(OS_ECOS) || defined(OS_RTEMS)
#  include "../../version_temp.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(OS_WIN32)
#  include <windows.h>
#endif

static void mxp_generate_macros( FILE *file );

int
main( int argc, char **argv )
{
	fprintf( stdout, "/*\n" );
	fprintf( stdout, " * Name:    mx_private_version.h\n" );
	fprintf( stdout, " *\n" );
	fprintf( stdout, " * Purpose: Macro definitions for the "
	    "version of MX and the versions\n" );
	fprintf( stdout, " *          of other software used by MX.\n" );
	fprintf( stdout, " *\n" );
	fprintf( stdout, " * WARNING: This file is MACHINE GENERATED.  "
	    "Do not edit it!\n" );
	fprintf( stdout, " */\n" );
	fprintf( stdout, "\n" );

	fprintf( stdout, "#ifndef __MX_PRIVATE_VERSION_H__\n");
	fprintf( stdout, "#define __MX_PRIVATE_VERSION_H__\n");

	fprintf( stdout, "\n" );

	fprintf( stdout, "#define MX_VERSION     %luL\n",
		MX_MAJOR_VERSION * 1000000L
		+ MX_MINOR_VERSION * 1000L
		+ MX_UPDATE_VERSION );

	fprintf( stdout, "\n" );

	fprintf( stdout, "#define MX_MAJOR_VERSION    %d\n", MX_MAJOR_VERSION );
	fprintf( stdout, "#define MX_MINOR_VERSION    %d\n", MX_MINOR_VERSION );
	fprintf( stdout, "#define MX_UPDATE_VERSION   %d\n", MX_UPDATE_VERSION);

	fprintf( stdout, "\n" );

	fprintf( stdout, "#define MX_BRANCH_LABEL " );
	fputc( '"', stdout );
	fprintf( stdout, MX_BRANCH_LABEL );
	fputc( '"', stdout );

	fprintf( stdout, "\n\n" );

	mxp_generate_macros( stdout );

	fprintf( stdout, "#endif /* __MX_PRIVATE_VERSION_H__ */\n");
	fprintf( stdout, "\n" );

	return 0;
}

/*-------------------------------------------------------------------------*/

#if defined(OS_VXWORKS)

/* FIXME - This is very fragile code. */

#define VERSION_TEMP_FILENAME	"version_temp.txt"

static void
mxp_generate_gnuc_macros( FILE *version_file )
{
	FILE *file;
	char buffer[500];
	int num_items;
	unsigned long major, minor, patchlevel;
	unsigned long version_ptr_offset, skip_length;
	char *version_ptr, *version_number_ptr;

	file = fopen( "version_temp.txt","r" );

	if ( file == NULL ) {
	    fprintf( stderr,
		"mx_private_version: Did not find '%s'.\n",
		VERSION_TEMP_FILENAME );

	    exit(1);
	}

	fgets( buffer, sizeof(buffer), file );

	while ( !feof(file) && !ferror(file) ) {
#if 0
	    fprintf(stderr, "buffer = '%s'\n", buffer );
#endif

	    /* Look for a line with the string 'gcc version' in it. */

	    version_ptr = strstr( buffer, "gcc version" );

	    if ( version_ptr != NULL ) {
		/* If 'gcc version' was found, then get a pointer to the
		 * first numerical character found after that string.
		 */

		version_ptr_offset = version_ptr - buffer;

		skip_length = strcspn( version_ptr, "0123456789" );

		if ( (version_ptr_offset + skip_length) >= strlen(buffer) ) {
			/* If version_ptr_offset + skip_length puts us past
			 * the end of the string, then there is no version
			 * number there.
			 */
			continue;
		}

		version_number_ptr = version_ptr + skip_length;

#if 0
		fprintf( stderr, "version_number_ptr = '%s'\n",
			version_number_ptr );
#endif

		num_items = sscanf( version_number_ptr, "%lu.%lu.%lu",
			&major, &minor, &patchlevel );

		if ( num_items < 2 ) {
			fprintf( stderr,
			"The selected GCC version number '%s' does not "
			"have a decimal point in it.\n",
				version_number_ptr );
			continue;
		} else
		if ( num_items == 2 ) {
			patchlevel = 0;
		}

		
		fprintf( version_file, "#define MX_GNUC_VERSION    %luL\n",
			major * 1000000L + minor * 1000L + patchlevel );

		fprintf( version_file, "\n" );

		fclose(file);

		return;
	    }

	    fgets( buffer, sizeof(buffer), file );
	}

	fprintf( stderr,
	"Did not find the target GCC version number in '%s'.\n",
		VERSION_TEMP_FILENAME );

	fclose(file);

	return;
}

/*---*/

#elif defined(OS_ECOS) || defined(OS_RTEMS)

static void
mxp_generate_gnuc_macros( FILE *version_file )
{
	int num_items;
	unsigned long major, minor, patchlevel;

	num_items = sscanf( MX_GNUC_TARGET_VERSION, "%lu.%lu.%lu",
			&major, &minor, &patchlevel );

	if ( num_items < 3 ) {
		patchlevel = 0;
	}

	fprintf( version_file, "#define MX_GNUC_VERSION    %luL\n",
		major * 1000000L + minor * 1000L + patchlevel );

	fprintf( version_file, "\n" );

	return;
}

/*---*/

#elif defined(__GNUC__) || defined(__ICC)

#  if !defined(__GNUC_PATCHLEVEL__)
#    define __GNUC_PATCHLEVEL__  0
#  endif

static void
mxp_generate_gnuc_macros( FILE *version_file )
{
	fprintf( version_file, "#define MX_GNUC_VERSION    %luL\n",
		__GNUC__ * 1000000L
		+ __GNUC_MINOR__ * 1000L
		+ __GNUC_PATCHLEVEL__ );

	fprintf( version_file, "\n" );
}

#endif   /* __GNUC__ */

/*-------------------------------------------------------------------------*/

#if defined(__ICC)

/* Intel C++ compiler */

static void
mxp_generate_icc_macros( FILE *version_file )
{
	fprintf( version_file, "#define MX_ICC_VERSION      %luL\n",
		(__ICC * 100L) + __INTEL_COMPILER_UPDATE );

	fprintf( version_file, "\n" );
}

#endif  /* __ICC */

/*-------------------------------------------------------------------------*/

#if defined(__clang__)

static void
mxp_generate_clang_macros( FILE *version_file )
{
	fprintf( version_file, "#define MX_CLANG_VERSION    %luL\n",
		__clang_major__ * 1000000L
		+ __clang_minor__ * 1000L
		+ __clang_patchlevel__ );

	fprintf( version_file, "\n" );
}

#endif	/* __clang__ */

/*-------------------------------------------------------------------------*/

#if defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_ANDROID)

      static void
      mxp_generate_glibc_macros( FILE *version_file )
      {
          return;
      }

/*---*/

#elif defined(__GLIBC__)

#  if (__GLIBC__ < 2)
#     define MXP_USE_GNU_GET_LIBC_VERSION   0
#  elif (__GLIBC__ == 2)
#     if (__GLIBC_MINOR__ < 1)
#        define MXP_USE_GNU_GET_LIBC_VERSION   0
#     else
#        define MXP_USE_GNU_GET_LIBC_VERSION   1
#     endif
#  else
#     define MXP_USE_GNU_GET_LIBC_VERSION   1
#  endif

#  if MXP_USE_GNU_GET_LIBC_VERSION
#     include <gnu/libc-version.h>
#  endif

      static void
      mxp_generate_glibc_macros( FILE *version_file )
      {
	  unsigned long libc_major, libc_minor, libc_patchlevel;

#  if MXP_USE_GNU_GET_LIBC_VERSION
          const char *libc_version;
	  int num_items;

          libc_version = gnu_get_libc_version();

	  num_items = sscanf( libc_version, "%lu.%lu.%lu",
	  		&libc_major, &libc_minor, &libc_patchlevel );

          switch( num_items ) {
	  case 3:
	  	break;
          case 2:
	  	libc_patchlevel = 0;
		break;
	  default:
	  	fprintf( stderr,
		"Error: Unparseable libc_version string '%s' found.\n",
			libc_version );
		exit(1);
	  }

#   else /* not MXP_USE_GNU_GET_LIBC_VERSION */

          libc_major = __GLIBC__;
          libc_minor = __GLIBC_MINOR__;
          libc_patchlevel = 0;

#   endif /* not MXP_USE_GNU_GET_LIBC_VERSION */

	  fprintf( version_file, "#define MX_GLIBC_VERSION   %luL\n",
		libc_major * 1000000L
		+ libc_minor * 1000L
		+ libc_patchlevel );

	  fprintf( version_file, "\n" );
      }

#endif   /* __GLIBC__ */

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32)

#if ( defined(_MSC_VER) && (_MSC_VER > 1200) )
# define HAVE_OSVERSIONINFOEX   TRUE
#elif ( defined(__BORLANDC__) || defined(__GNUC__) )
# define HAVE_OSVERSIONINFOEX   TRUE
#else
# define HAVE_OSVERSIONINFOEX   FALSE
#endif

static void
mxp_generate_macros( FILE *version_file )
{
	BOOL status;
	int use_extended_struct;

	unsigned long win32_major_version;
	unsigned long win32_minor_version;
	unsigned long win32_platform_id;
	unsigned char win32_product_type;

#if HAVE_OSVERSIONINFOEX
	OSVERSIONINFOEX osvi;
#else
	OSVERSIONINFO osvi;
#endif

	/* Try using OSVERSIONINFOEX first. */

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
			fprintf( version_file,
			"#error Cannot detect Windows version.\n" );
		
			fprintf( stderr,
			"GetVersionEx() could not detect the version "
			"of Windows we are using.\n" );

			return;
                }
        }

	win32_major_version = osvi.dwMajorVersion;
	win32_minor_version = osvi.dwMinorVersion;
	win32_platform_id = osvi.dwPlatformId;

#if HAVE_OSVERSIONINFOEX
	if ( use_extended_struct ) {
		win32_product_type = osvi.wProductType;
	} else {
		win32_product_type = 0;
	}
#else
	win32_product_type = 0;
#endif

	/* Now we know enough to generate the macros. */

	switch( win32_major_version ) {
	case 3:
		/* Windows NT 3.51 */

		fprintf( version_file, "#define MX_WINVER        0x030A\n" );
		fprintf( version_file, "#define MX_WIN32_WINNT   0x030A\n" );
		break;
	case 4:
		switch( win32_minor_version ) {
		case 0:
		case 1:
		case 3:
			fprintf( version_file,
					"#define MX_WINVER        0x0400\n" );

			if ( win32_platform_id == VER_PLATFORM_WIN32_NT ) {
				/* Windows NT 4.0 */

				fprintf( version_file,
					"#define MX_WIN32_WINNT   0x0400\n" );
			} else {
				/* Windows 95 */

				fprintf( version_file,
					"#define MX_WIN32_WINDOWS 0x0400\n" );
			}
			break;
		case 10:
			/* Windows 98 */

			fprintf( version_file,
					"#define MX_WINVER        0x0410\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINDOWS 0x0410\n" );
			break;
		case 90:
			/* Windows Me */

			fprintf( version_file,
					"#define MX_WINVER        0x0500\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINDOWS 0x0500\n" );
			break;
		default:
			fprintf( version_file,
				"#error Unrecognized Windows version 4\n" );
			fprintf( stderr,
				"Error: Unrecognized Windows version 4\n" );
			break;
		}
		break;
	case 5:
		switch( win32_minor_version ) {
		case 0:
			/* Windows 2000 */

			fprintf( version_file,
					"#define MX_WINVER        0x0500\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINNT   0x0500\n" );
			break;
		case 1:
			/* Windows XP */

			fprintf( version_file,
					"#define MX_WINVER        0x0501\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINNT   0x0501\n" );
			break;
		case 2:
			/* Windows Server 2003 */

			fprintf( version_file,
					"#define MX_WINVER        0x0502\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINNT   0x05022n" );
			break;
		default:
			fprintf( version_file,
				"#error Unrecognized Windows version 5\n" );
			fprintf( stderr,
				"Error: Unrecognized Windows version 5\n" );
			break;
		}
		break;
	case 6:
		switch( win32_minor_version ) {
		case 0:
			/* Windows Vista or Windows Server 2008 */

			fprintf( version_file,
					"#define MX_WINVER        0x0600\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINNT   0x0600\n" );
			break;
		case 1:
			/* Windows 7 or Windows Server 2008 R2 */

			fprintf( version_file,
					"#define MX_WINVER        0x0601\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINNT   0x0601\n" );
			break;
		case 2:
			/* Windows 8 or Windows Server 2012 */

			fprintf( version_file,
					"#define MX_WINVER        0x0602\n" );
			fprintf( version_file,
					"#define MX_WIN32_WINNT   0x0602\n" );
			break;
		default:
			fprintf( version_file,
				"#error Unrecognized Windows version 6\n" );
			fprintf( stderr,
				"Error: Unrecognized Windows version 6\n" );
			break;
		}
		break;
	default:
		fprintf( version_file,
				"#error Unrecognized Windows major version\n" );
		fprintf( stderr,
				"Error: Unrecognized Windows major version\n" );
		break;
	}

	fprintf( version_file, "\n" );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_SOLARIS) || defined(OS_MACOSX) || defined(OS_UNIXWARE)

#include <sys/utsname.h>

static void
mxp_generate_macros( FILE *version_file )
{
#if defined(OS_SOLARIS)
	static char os_name[] = "MX_SOLARIS_VERSION";
#elif defined(OS_MACOSX)
	static char os_name[] = "MX_DARWIN_VERSION";
#elif defined(OS_UNIXWARE)
	static char os_name[] = "MX_UNIXWARE_VERSION";
#endif
	struct utsname uname_struct;
	int os_status, saved_errno;
	int num_items, os_major, os_minor, os_update;

#if defined(__clang__)
	mxp_generate_clang_macros( version_file );
#endif

	os_status = uname( &uname_struct );

	if ( os_status < 0 ) {
		saved_errno = errno;

		fprintf( stderr,
		"uname() failed.  Errno = %d, error message = '%s'\n",
			saved_errno, strerror( saved_errno ) );
		exit(1);
	}

#if defined(OS_UNIXWARE)
	num_items = sscanf( uname_struct.version, "%d.%d.%d",
			&os_major, &os_minor, &os_update );
#else
	num_items = sscanf( uname_struct.release, "%d.%d.%d",
			&os_major, &os_minor, &os_update );
#endif

	if ( num_items < 2 ) {
		fprintf( stderr, "OS version not found in string '%s'\n",
			uname_struct.release );
		exit(1);
	}

	if ( num_items == 2 ) {
		os_update = 0;
	}

	fprintf( version_file, "#define %s    %luL\n",
		os_name, 
		os_major * 1000000L
		+ os_minor * 1000L
		+ os_update );

	fprintf( version_file, "\n" );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_CYGWIN)

#include <sys/utsname.h>

static void
mxp_generate_macros( FILE *version_file )
{
	mxp_generate_gnuc_macros( version_file );

	struct utsname uname_struct;
	int os_status, saved_errno;
	int num_items, os_major, os_minor, os_update;

	os_status = uname( &uname_struct );

	if ( os_status < 0 ) {
		saved_errno = errno;

		fprintf( stderr,
		"uname() failed.  Errno = %d, error message = '%s'\n",
			saved_errno, strerror( saved_errno ) );
		exit(1);
	}

	num_items = sscanf( uname_struct.release, "%d.%d.%d",
			&os_major, &os_minor, &os_update );

	if ( num_items < 2 ) {
		fprintf( stderr, "OS version not found in string '%s'\n",
			uname_struct.release );
		exit(1);
	}

	if ( num_items == 2 ) {
		os_update = 0;
	}

	fprintf( version_file, "#define MX_CYGWIN_VERSION    %luL\n",
		os_major * 1000000L
		+ os_minor * 1000L
		+ os_update );

	fprintf( version_file, "\n" );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_LINUX)

static void
mxp_generate_macros( FILE *version_file )
{
	/* Detect the compiler. */

#  if defined(__ICC)
	mxp_generate_icc_macros( version_file );
	mxp_generate_gnuc_macros( version_file );

#  elif defined(__clang__)
	mxp_generate_clang_macros( version_file );

#  elif defined(__GNUC__)
	mxp_generate_gnuc_macros( version_file );
#  else
#     error Unrecognized Linux C compiler.
#  endif

	/* Detect the C runtime library. */

#if defined(__GLIBC__)
	mxp_generate_glibc_macros( version_file );
#else
	/* Attempt to detect the runtime library by looking at
	 * the output of 'ldd --version', which was written to
	 * the file 'ldd_temp.txt' by tools/Makefile.linux.
	 */

	{
	    FILE *ldd_temp_file;
	    char buffer[80];

	    ldd_temp_file = fopen( "ldd_temp.txt", "r" );

	    if ( ldd_temp_file == NULL )
		return;

	    while (1) {
		fgets( buffer, sizeof(buffer), ldd_temp_file );

		if ( feof(ldd_temp_file) || ferror(ldd_temp_file) ) {
		    fclose( ldd_temp_file );
		    break;		/* Exit the while() loop. */
		}

		if ( strncmp( buffer, "musl libc", 9 ) == 0 ) {

		    /*--- musl libc ---*/

		    unsigned long musl_major_version;
		    unsigned long musl_minor_version;
		    unsigned long musl_update_version;

		    /* The next line contains the musl version number,
		     * which should resemble this:
		     *   Version 1.1.9
		     */

		    fgets( buffer, sizeof(buffer), ldd_temp_file );

		    if ( feof(ldd_temp_file) || ferror(ldd_temp_file) ) {
		 	fclose( ldd_temp_file );
			break;	/* Exit the while() loop. */
		    }

		    if ( strncmp( buffer, "Version", 7 ) == 0 ) {

		    	char *ptr;
			int num_items;

		        /* Note: We cannot use libMx functions here,
			 * since libMx probably has not yet been built
			 * at the time mx_private_version is executed.
			 */

			ptr = strchr( buffer, ' ' );
			ptr++;

			num_items = sscanf( ptr, "%lu.%lu.%lu",
					&musl_major_version,
					&musl_minor_version,
					&musl_update_version );

			if ( num_items < 2 ) {
			    fclose( ldd_temp_file );
			    break;	/* Exit the while() loop. */
			} else
			if ( num_items < 3 ) {
			    musl_update_version = 0;
			}

			fprintf( version_file,
			"#define MX_MUSL_VERSION    %luL\n\n",
				musl_major_version * 1000000L
				+ musl_minor_version * 1000L
				+ musl_update_version );

			fclose( ldd_temp_file );
			break;	/* Exit the while() loop. */
		    }
		}
	    }
	}
#endif   /* Not __GLIBC__ */

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(__clang__)

static void
mxp_generate_macros( FILE *version_file )
{
	mxp_generate_clang_macros( version_file );

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(__GNUC__)

static void
mxp_generate_macros( FILE *version_file )
{
	mxp_generate_gnuc_macros( version_file );

#if defined(__GLIBC__)
	mxp_generate_glibc_macros( version_file );
#endif

#if defined(OS_RTEMS)
	fprintf( version_file, "#define MX_RTEMS_VERSION    %luL\n",
		__RTEMS_MAJOR__ * 1000000L
		+ __RTEMS_MINOR__ * 1000L
		+ __RTEMS_REVISION__ );
#endif

	return;
}

/*-------------------------------------------------------------------------*/

#elif defined(OS_IRIX) || defined(OS_VMS)

static void
mxp_generate_macros( FILE *version_file )
{
	return;
}

#else

#  error mx_private_version.c has not been configured for this build target.

#endif

