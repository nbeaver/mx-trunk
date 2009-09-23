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

#elif defined(OS_WIN32)

#include <windows.h>

MX_EXPORT mx_status_type
mx_get_console_size( unsigned long *num_rows, unsigned long *num_columns )
{
	static const char fname[] = "mx_get_console_size()";

	HANDLE console_screen_buffer_handle;
	CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
	BOOL return_value;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( ( num_rows == NULL ) || ( num_columns == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the argument pointers passed was NULL." );
	}

	console_screen_buffer_handle = CreateFile(
			"CONOUT$", GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );

	if ( console_screen_buffer_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"CreateFile(\"CONOUT$\", ... ) failed with "
		"error code %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	return_value = GetConsoleScreenBufferInfo( console_screen_buffer_handle,
						&console_screen_buffer_info );

	if ( return_value == 0 ) {
		last_error_code = GetLastError();

		CloseHandle( console_screen_buffer_handle );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"GetConsoleScreenBufferInfo() for 'CONOUT$' failed with "
		"error code %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	*num_rows    = console_screen_buffer_info.dwSize.X;
	*num_columns = console_screen_buffer_info.dwSize.Y;

	CloseHandle( console_screen_buffer_handle );

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

