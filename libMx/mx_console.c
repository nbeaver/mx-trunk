/*
 * Name:    mx_console.c
 *
 * Purpose: Console handling functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_key.h"
#include "mx_console.h"

/*---------------------------------------------------------------------------*/

#if defined(OS_UNIX)

#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

MX_EXPORT mx_status_type
mx_get_console_size( unsigned long *num_rows, unsigned long *num_columns )
{
	static const char fname[] = "mx_get_console_size()";

	int os_status, saved_errno;

	if ( ( num_rows == NULL ) || ( num_columns == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the argument pointers passed was NULL." );
	}

#if defined(TIOCGWINSZ)
	{
		struct winsize w;

		os_status = ioctl( 0, TIOCGWINSZ, &w );

		if ( os_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to get the console size failed.  "
			"Errno = %d, error message = '%s'",
				saved_errno, mx_strerror(saved_errno, NULL, 0));
		}

		*num_rows = w.ws_row;
		*num_columns = w.ws_col;
	}
#elif defined(TIOCGSIZE)
	{
		struct ttysize t;

		os_status = ioctl( 0, TIOCGSIZE, &t );

		if ( os_status < 0 ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"The attempt to get the console size failed.  "
			"Errno = %d, error message = '%s'",
				saved_errno, mx_strerror(saved_errno, NULL, 0));
		}

		*num_rows = t.ts_lines;
		*num_columns = t.ts_cols;
	}
#else

#error Neither TIOCGWINSZ nor TIOCGSIZE is defined for this Unix-like platform.

#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#elif 0

MX_EXPORT mx_status_type
mx_get_console_size( unsigned long *num_rows, unsigned long *num_columns )
{
	static const char fname[] = "mx_get_console_size()";

	if ( ( num_rows == NULL ) || ( num_columns == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the argument pointers passed was NULL." );
	}

	*num_rows = 25;
	*num_columns = 80;

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#else

#error Console handling functions are not defined for this platform.

#endif

/*===========================================================================*/

MX_EXPORT mx_status_type
mx_paged_console_output( FILE *console,
			char *output_text )
{
	static const char fname[] = "mx_paged_console_output()";

	char *ptr;
	unsigned long console_rows, console_columns, line;
	int c;
	mx_status_type mx_status;

	if ( console == (FILE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The FILE pointer passed was NULL." );
	}
	if ( output_text == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The output text pointer passed was NULL." );
	}

	mx_status = mx_get_console_size( &console_rows, &console_columns );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ptr = output_text;

	console_rows--;

	for (;;) {
		for ( line = 0; line < console_rows; line++ ) {
			while ( (c = (int) *ptr) != '\n' ) {
				if ( c == '\0' ) {
					return MX_SUCCESSFUL_RESULT;
				}
				fputc( c, console );
				ptr++;
			}
			fputc( '\n', console );
			ptr++;
		}
		fprintf( console,
		"Press q to quit or any other key to continue." );

		fflush( console );

		c = mx_getch();

		fputc( '\n', console );

		if ( (c == 'q') || (c == 'Q') ) {
			return MX_SUCCESSFUL_RESULT;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

