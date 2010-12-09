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
 * Copyright 2008-2009 Illinois Institute of Technology
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

#elif defined(__GNUC__)

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

#if defined(OS_RTEMS) || defined(OS_VXWORKS)

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
#  else
      extern const char __libc_version[];
#  endif

      static void
      mxp_generate_glibc_macros( FILE *version_file )
      {
          const char *libc_version;
	  unsigned long libc_major, libc_minor, libc_patchlevel;
	  int num_items;

#  if MXP_USE_GNU_GET_LIBC_VERSION
          libc_version = gnu_get_libc_version();
#  else
          libc_version = __libc_version[];
#  endif

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

	  fprintf( version_file, "#define MX_GLIBC_VERSION    %luL\n",
		libc_major * 1000000L
		+ libc_minor * 1000L
		+ libc_patchlevel );

	  fprintf( version_file, "\n" );
      }

#endif   /* __GLIBC__ */

/*-------------------------------------------------------------------------*/

#if defined(OS_SOLARIS)

#include <sys/utsname.h>

static void
mxp_generate_macros( FILE *version_file )
{
#if defined(OS_SOLARIS)
	static char os_name[] = "MX_SOLARIS_VERSION";
#endif
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

	fprintf( version_file, "#define %s    %luL\n",
		os_name, 
		os_major * 1000000L
		+ os_minor * 1000L
		+ os_update );

	fprintf( version_file, "\n" );

	return;
}

/*---*/

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

/*---*/

#elif defined(OS_IRIX) || defined(OS_WIN32) || defined(OS_VMS)

static void
mxp_generate_macros( FILE *version_file )
{
	return;
}

#else

#  error mx_private_version.c has not been configured for this build target.

#endif   /* OS_LINUX */

