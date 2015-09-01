/*
 * Name:    mx_console.c
 *
 * Purpose: Console handling functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009-2012, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_key.h"
#include "mx_console.h"

/*---------------------------------------------------------------------------*/

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_RTEMS)

#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>

#if defined(__FreeBSD__) || defined(MX_MUSL_VERSION)
#include <termios.h>
#elif !defined(OS_QNX)
#include <sys/termios.h>
#endif

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

	HANDLE csb_handle;
	CONSOLE_SCREEN_BUFFER_INFO csb_info;
	BOOL return_value;
	DWORD last_error_code;
	TCHAR message_buffer[100];

	if ( ( num_rows == NULL ) || ( num_columns == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the argument pointers passed was NULL." );
	}

	csb_handle = CreateFile(
			"CONOUT$", GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0 );

	if ( csb_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"CreateFile(\"CONOUT$\", ... ) failed with "
		"error code %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

	return_value = GetConsoleScreenBufferInfo( csb_handle, &csb_info );

	if ( return_value == 0 ) {
		last_error_code = GetLastError();

		CloseHandle( csb_handle );

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"GetConsoleScreenBufferInfo() for 'CONOUT$' failed with "
		"error code %ld, error message = '%s'.",
			last_error_code, message_buffer );
	}

#if 0
	MX_DEBUG(-2,("%s: dwSize.X = %d", fname, (int) csb_info.dwSize.X));
	MX_DEBUG(-2,("%s: dwSize.Y = %d", fname, (int) csb_info.dwSize.Y));
	MX_DEBUG(-2,("%s: dwCursorPosition.X = %d",
				fname, (int) csb_info.dwCursorPosition.X));
	MX_DEBUG(-2,("%s: dwCursorPosition.Y = %d",
				fname, (int) csb_info.dwCursorPosition.Y));
	MX_DEBUG(-2,("%s: wAttributes = %#x",
				fname, (int) csb_info.wAttributes));
	MX_DEBUG(-2,("%s: srWindow.Left   = %d",
				fname, (int) csb_info.srWindow.Left));
	MX_DEBUG(-2,("%s: srWindow.Top    = %d",
				fname, (int) csb_info.srWindow.Top));
	MX_DEBUG(-2,("%s: srWindow.Right  = %d",
				fname, (int) csb_info.srWindow.Right));
	MX_DEBUG(-2,("%s: srWindow.Bottom = %d",
				fname, (int) csb_info.srWindow.Bottom));
	MX_DEBUG(-2,("%s: dwMaximumWindowSize.X = %d",
				fname, (int) csb_info.dwMaximumWindowSize.X));
	MX_DEBUG(-2,("%s: dwMaximumWindowSize.Y = %d",
				fname, (int) csb_info.dwMaximumWindowSize.Y));

	mx_getch();
#endif

	*num_rows    = csb_info.srWindow.Bottom - csb_info.srWindow.Top + 1;
	*num_columns = csb_info.srWindow.Right - csb_info.srWindow.Left + 1;

	CloseHandle( csb_handle );

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#elif ( 0 && defined(OS_VMS) )

#include "mx_stdint.h"

#include <errno.h>
#include <ssdef.h>
#include <starlet.h>
#include <descrip.h>
#include <dvidef.h>

typedef struct {
	uint16_t buffer_length;
	uint16_t item_code;
	uint32_t *buffer_address;
	uint32_t *return_length_address;
} ILE;

MX_EXPORT mx_status_type
mx_get_console_size( unsigned long *num_rows, unsigned long *num_columns )
{
	static const char fname[] = "mx_get_console_size()";

	uint32_t terminal_width;

	int vms_status;
	$DESCRIPTOR(tt_descriptor, "TT:");

	short iosb[4];

	ILE item_list[] =
		{ {4, DVI$_DEVBUFSIZ, &terminal_width, NULL},
		  {0, 0, NULL, NULL} };

	if ( ( num_rows == NULL ) || ( num_columns == NULL ) ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"One of the argument pointers passed was NULL." );
	}

	vms_status = sys$getdviw( 0, 0, &tt_descriptor, &item_list, &iosb );

	if ( vms_status != SS$_NORMAL ) {
		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"The attempt to get the width of the terminal failed.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );
	}

	*num_rows = 25;		/* FIXME - Where do you get rows from? */

	*num_columns = terminal_width;

#if 1
	MX_DEBUG(-2,("%s: num_rows = %lu, num_columns = %lu",
		fname, *num_rows, *num_columns));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*---------------------------------------------------------------------------*/

#elif defined(OS_VXWORKS) || defined(OS_DJGPP) || defined(OS_VMS)

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

#if !defined(OS_SOLARIS)
	return MX_SUCCESSFUL_RESULT;
#endif
}

