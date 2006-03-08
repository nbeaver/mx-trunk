/*
 * Name:    mx_key.c
 *
 * Purpose: Emulation of MSDOS-style keyboard handling functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_ascii.h"
#include "mx_key.h"

/************************** Microsoft Win32 **************************/

#if defined(OS_WIN32)

#include <windows.h>
#include <conio.h>

/*
 * Get a single character from the terminal.
 */

MX_EXPORT int
mx_getch( void )
{
	int c;

	c = getch();

	/* Was the keystroke a function key or an arrow key? */

	if ( c == 0 || c == 0xE0 ) {
		c = getch();

		/* The following shift is for compatibility with MSDOS
		 * based versions of the keycodes and also allows function
		 * keys and arrow keys to be easily distinguished from
		 * ordinary keys.
		 */

		c <<= 8;
	}

	return c;
}

MX_EXPORT int
mx_kbhit( void )
{
	return kbhit();
}

static HANDLE mx_win32_stdin_handle = INVALID_HANDLE_VALUE;

static mx_status_type
mx_win32_get_stdin_handle( void )
{
	static const char fname[] = "mx_win32_get_stdin_handle()";

	DWORD last_error_code;
	char message_buffer[100];

#if 0
	mx_win32_stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
#else
	mx_win32_stdin_handle = CreateFile( "CONIN$",
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					NULL );
#endif

	if ( mx_win32_stdin_handle == INVALID_HANDLE_VALUE ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		return mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get handle for standard input.  "
		"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );
	}
	return MX_SUCCESSFUL_RESULT;
}

static void
mx_win32_key_echo( int echo_characters )
{
	static const char fname[] = "mx_win32_key_echo()";

	DWORD console_mode, last_error_code;
	BOOL console_status;
	char message_buffer[100];

	if ( mx_win32_stdin_handle == INVALID_HANDLE_VALUE ) {
		mx_status_type mx_status;

		mx_status = mx_win32_get_stdin_handle();

		if ( mx_status.code != MXE_SUCCESS )
			return;
	}

	console_status = GetConsoleMode( mx_win32_stdin_handle, &console_mode );

	if ( console_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to get the current console mode.  "
		"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );

		return;
	}

	if ( echo_characters ) {
		console_mode |= ENABLE_ECHO_INPUT;
	} else {
		console_mode &= ~( (DWORD) ENABLE_ECHO_INPUT );
	}

	console_status = SetConsoleMode( mx_win32_stdin_handle, console_mode );

	if ( console_status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"Unable to set the current console mode.  "
		"Win32 error code = %ld, error_message = '%s'",
			last_error_code, message_buffer );

		return;
	}
}

MX_EXPORT void
mx_key_echo_off( void )
{
	mx_win32_key_echo( FALSE );
}

MX_EXPORT void
mx_key_echo_on( void )
{
	mx_win32_key_echo( TRUE );
}

MX_EXPORT void
mx_key_getline( char *buffer, size_t max_buffer_length )
{
	size_t length;

	if ( ( buffer == NULL ) || ( max_buffer_length == 0 ) ) {
		return;
	}

	fgets( buffer, max_buffer_length, stdin );

	/* Delete any trailing newline. */

	length = strlen( buffer );

	if ( length > 0 ) {
		if ( buffer[length-1] == '\n' ) {
			buffer[length-1] = '\0';
		}
	}

	return;
}

/************************** MSDOS **************************/

#elif defined(OS_MSDOS)

#include <conio.h>	/* A DOS specific header file. */
#include <dos.h>	/* A DOS specific header file. */

/*
 * Get a single character from standard input without permanently
 * changing terminal modes.
 */

MX_EXPORT int
mx_getch( void )
{
	union REGS regs;
	int result;

	regs.h.ah = 0;

#ifdef OS_DOS_EXT_WATCOM

	int386( 0x16, &regs, &regs );

	result = regs.x.eax;

#else  /* not OS_DOS_EXT_WATCOM */

	int86( 0x16, &regs, &regs );

	result = regs.x.ax;

#endif /* OS_DOS_EXT_WATCOM */

	result &= 0xFFFF;	/* mask to 16 bits. */

	return result;
}

/* 
 * Has the keyboard been hit recently?
 */

MX_EXPORT int
mx_kbhit( void )
{
	return kbhit();
}

/* --- */

static int echo_off = FALSE;

MX_EXPORT void
mx_key_echo_off( void )
{
	echo_off = TRUE;
}

MX_EXPORT void
mx_key_echo_on( void )
{
	echo_off = FALSE;
}

MX_EXPORT void
mx_key_getline( char *buffer, size_t max_buffer_length )
{
	long i;
	int c;

	if ( (buffer == NULL) || (max_buffer_length == 0) ) {
		return;
	}

	for ( i = 0; i < max_buffer_length; i++ ) {

		if ( i < 0 ) {
			/* Just in case we have a programming mistake
			 * with backspacing.
			 */

			i = 0;
		}

		if ( echo_off ) {
			c = getch();
		} else {
			c = getche();
		}

		if ( ( c == MX_CR )
		  || ( c == MX_LF ) )
		{
			/* We have reached the end of the line, so
			 * null terminate the buffer and return.
			 */

			buffer[i] = '\0';

			if ( echo_off ) {
				fputc( '\n', stdout );
			} else {
				if ( c == MX_CR ) {
					fputc( MX_LF, stdout );
				} else {
					fputc( MX_CR, stdout );
				}
			}
			return;
		} else
		if ( c == MX_CTRL_H ) {
			/* Back up one character and try again. */

			i--;
		} else {
			buffer[i] = c;
		}
	}

	/* Make sure we are null terminated. */

	if ( i >= max_buffer_length ) {
		buffer[max_buffer_length-1] = '\0';
	} else
	if ( i >= 0 ) {
		buffer[i] = '\0';
	} else {
		buffer[0] = '\0';
	}

	return;
}

/************************** VMS **************************/

#elif defined(OS_VMS)

#include <errno.h>
#include <ssdef.h>
#include <iodef.h>
#include <descrip.h>
#include <starlet.h>
#include <ttdef.h>
#include <tt2def.h>

static int tt_channel = -1;

static int
mx_vms_assign_tt_channel( void )
{
	static const char fname[] = "mx_vms_assign_tt_channel()";

	int vms_status;
	$DESCRIPTOR(tt_descriptor,"TT:");

	vms_status = sys$assign(
			&tt_descriptor,
			&tt_channel,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
    "An attempt to assign a channel for terminal input failed.  "
    "VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

	}

	return vms_status;
}

MX_EXPORT int
mx_getch( void )
{
	static const char fname[] = "mx_getch()";

	char c;
	short iosb[4];
	int vms_status;

	if ( tt_channel == -1 ) {
		vms_status = mx_vms_assign_tt_channel();

		if ( vms_status != SS$_NORMAL ) {
			return 0;
		}
	}

	vms_status = sys$qiow(
			0,
			tt_channel,
			IO$_READVBLK,
			iosb,
			0,
			0,
			&c,
			1,
			0,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to read a character from the terminal failed.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

		return 0;
	}

	return (int) c;
}

MX_EXPORT int
mx_kbhit( void )
{
	static const char fname[] = "mx_kbhit()";

	struct {
		unsigned short typeahead_count;
		char first_character;
		char reserved[5];
	} typeahead_struct;

	short iosb[4];
	int vms_status;

	if ( tt_channel == -1 ) {
		vms_status = mx_vms_assign_tt_channel();

		if ( vms_status != SS$_NORMAL ) {
			return 0;
		}
	}

	vms_status = sys$qiow(
			0,
			tt_channel,
			IO$_SENSEMODE | IO$M_TYPEAHDCNT,
			iosb,
			0,
			0,
			&typeahead_struct,
			1,
			0,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
"An attempt to see if characters were available from the terminal failed.  "
"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

		return 0;
	}

	if ( typeahead_struct.typeahead_count > 0 ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static void
mx_vms_key_echo( int echo_characters )
{
	static const char fname[] = "mx_vms_key_echo()";

	unsigned long old_tt_mode[2], new_tt_mode[2];
	short iosb[4];
	int vms_status;

	if ( tt_channel == -1 ) {
		vms_status = mx_vms_assign_tt_channel();

		if ( vms_status != SS$_NORMAL ) {
			return;
		}
	}

	/* Get the current terminal status for TT: */

	vms_status = sys$qiow(
			0,
			tt_channel,
			IO$_SENSEMODE,
			&iosb,
			0,
			0,
			old_tt_mode,
			8,
			0,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to get the current terminal status failed.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

		return;
	}

	new_tt_mode[0] = old_tt_mode[0];
	new_tt_mode[1] = old_tt_mode[1];

	/* Modify the echoing status of the terminal depending on the
	 * value of the 'echo_characters' argument.
	 */

	if ( echo_characters ) {
		/* Clear NOECHO bit */

		new_tt_mode[1] &= ~((unsigned long) TT$M_NOECHO);
	} else {
		/* Set NOECHO bit */

		new_tt_mode[1] |= TT$M_NOECHO;
	}

	/* Set the new terminal status for TT: */

	vms_status = sys$qiow(
			0,
			tt_channel,
			IO$_SETMODE,
			&iosb,
			0,
			0,
			new_tt_mode,
			8,
			0,
			0,
			0,
			0 );

	if ( vms_status != SS$_NORMAL ) {
		(void) mx_error( MXE_OPERATING_SYSTEM_ERROR, fname,
		"An attempt to modify the current terminal status failed.  "
		"VMS error number %d, error message = '%s'",
			vms_status, strerror( EVMSERR, vms_status ) );

		return;
	}

	return;
}

MX_EXPORT void
mx_key_echo_off( void )
{
	mx_vms_key_echo( FALSE );
}

MX_EXPORT void
mx_key_echo_on( void )
{
	mx_vms_key_echo( TRUE );
}

MX_EXPORT void
mx_key_getline( char *buffer, size_t max_buffer_length )
{
	size_t length;

	if ( ( buffer == NULL ) || ( max_buffer_length == 0 ) ) {
		return;
	}

	fgets( buffer, max_buffer_length, stdin );

	/* Delete any trailing newline. */

	length = strlen( buffer );

	if ( length > 0 ) {
		if ( buffer[length-1] == '\n' ) {
			buffer[length-1] = '\0';
		}
	}

	return;
}

/************************** Unix **************************/

#elif defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_RTEMS) \
	|| defined(OS_ECOS)

/* Do we use new-style (termios) or old-style (termio) terminal controls? */

#if defined(OS_SUNOS4) || defined(OS_AIX)
#   define USE_TERMIOS    0
#else
#   define USE_TERMIOS    1
#endif

#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#if USE_TERMIOS
#   include <termios.h>
#else
#   include <termio.h>
#endif

#if defined(OS_LINUX) || defined(OS_BSD)
#   include <sys/ioctl.h>
#else
    extern int ioctl( int fd, int request, ... );
#endif

#include "mx_select.h"

#if USE_TERMIOS

/* The termios versions of these functions are derived from the Unix
 * Programming FAQ found at http://www.erlenstar.demon.co.uk/unix/faq_toc.html
 */

static struct termios saved_canonical_mode;

static int
tty_raw( int fd )
{
	struct termios raw_mode;
	int status;

	status = tcgetattr( fd, &saved_canonical_mode );

	if ( status != 0 )
		return status;

	memcpy( &raw_mode, &saved_canonical_mode, sizeof( struct termios ) );

	/* Disable canonical mode and set buffer size to 1 byte */

	raw_mode.c_lflag &= (~ICANON);
	raw_mode.c_cc[VTIME] = 0;
	raw_mode.c_cc[VMIN] = 1;

	return tcsetattr( fd, TCSANOW, &raw_mode );
}

static int
tty_reset( int fd )
{
	return tcsetattr( fd, TCSANOW, &saved_canonical_mode );
}

/*---*/

static struct termios saved_echo_on_mode;

static int echo_off = FALSE;

MX_EXPORT void
mx_key_echo_off( void )
{
	struct termios echo_off_mode;

	if ( echo_off ) {
		return;
	}

	echo_off = TRUE;

	tcgetattr( fileno(stdin), &saved_echo_on_mode );

	memcpy( &echo_off_mode, &saved_echo_on_mode, sizeof( struct termios ) );

	echo_off_mode.c_lflag &= (~ECHO);

	tcsetattr( fileno(stdin), TCSANOW, &echo_off_mode );

	return;
}

MX_EXPORT void
mx_key_echo_on( void )
{
	if ( echo_off == FALSE ) {
		return;
	}

	tcsetattr( fileno(stdin), TCSANOW, &saved_echo_on_mode );
}

#else /* --- Not USE_TERMIOS --- */

/* The following versions of tty_raw() and tty_reset() are based on
 * W. R. Stevens' "Unix Network Programming", page 609-610 and is
 * set up for System V style terminal I/O.
 */

/*
 * Put a terminal device into RAW mode with ECHO off.  Before doing so
 * we first save the terminal's current mode, assuming the caller will 
 * call the tty_reset() function (also in this file) when it is done
 * with raw mode.
 */

static struct termio tty_mode;		/* save tty mode here. */

static int
tty_raw( int fd )
{
	struct termio temp_mode;

	if ( ioctl( fd, TCGETA, (char *) &temp_mode ) < 0 )
		return(-1);

	tty_mode = temp_mode;		/* save for restoring later */

	temp_mode.c_iflag  = 0;		/* turn off all input control */
	temp_mode.c_oflag &= ~OPOST;	/* disable output post-processing */

#ifdef OS_CYGWIN
	temp_mode.c_lflag &= ~(ISIG | ICANON | ECHO);
#else
	temp_mode.c_lflag &= ~(ISIG | ICANON | ECHO | XCASE);
#endif
					/* disable signal generation */
					/* disable canonical input */
					/* disable echo */
					/* disable upper/lower output */
	temp_mode.c_cflag &= ~(CSIZE | PARENB);
					/* clear char size, disable parity */
	temp_mode.c_cflag |= CS8;	/* 8-bit chars */

	temp_mode.c_cc[VMIN]  = 1;	/* min #chars to satisfy read */
	temp_mode.c_cc[VTIME] = 1;	/* 10'ths of seconds between chars */

	if ( ioctl( fd, TCSETA, (char *) &temp_mode ) < 0 )
		return(-1);

	return(0);
}

/*
 * Restore a terminal's mode to whatever it was on the most recent call
 * to the tty_raw() function above.
 */

static int
tty_reset( int fd )
{
	if ( ioctl( fd, TCSETA, (char *) &tty_mode ) < 0 )
		return(-1);

	return(0);
}

#endif  /* --- USE_TERMIOS --- */

/*
 * Get a single character from standard input without permanently
 * changing terminal modes.
 */

MX_EXPORT int
mx_getch( void )
{
	char ch;
	int result, nchars;

	if ( tty_raw( fileno(stdin) ) != 0 )
		return(-1);

	nchars = (int) read( fileno(stdin), &ch, 1 );

	if ( nchars != 1 )
		return(-1);

	if ( tty_reset( fileno(stdin) ) != 0 )
		return(-1);

	result = (int) ch;

	return result;
}

/* 
 * kbhit() implemented using select().
 */

MX_EXPORT int
mx_kbhit( void )
{
	struct timeval timeout;
	int return_value, is_a_tty;

#if HAVE_FD_SET
	fd_set mask;

	FD_ZERO( &mask );

	FD_SET( fileno(stdin), &mask );
#else /* HAVE_FD_SET */
	long mask;

	mask = 1 << fileno(stdin);
#endif /* HAVE_FD_SET */

	timeout.tv_usec = 0;
	timeout.tv_sec  = 0;

	is_a_tty = isatty( fileno(stdin) );

	if ( is_a_tty ) {
		if ( tty_raw( fileno(stdin) ) != 0 )
			return 0;
	}

	return_value = select(1 + fileno(stdin), &mask, NULL, NULL, &timeout);

	if ( is_a_tty ) {
		if ( tty_reset( fileno(stdin) ) != 0 )
			return 0;
	}

	return return_value;
}

MX_EXPORT void
mx_key_getline( char *buffer, size_t max_buffer_length )
{
	size_t length;

	if ( ( buffer == NULL ) || ( max_buffer_length == 0 ) ) {
		return;
	}

	fgets( buffer, (int) max_buffer_length, stdin );

	/* Delete any trailing newline. */

	length = strlen( buffer );

	if ( length > 0 ) {
		if ( buffer[length-1] == '\n' ) {
			buffer[length-1] = '\0';
		}
	}

	return;
}

/*************************************************************************/

/* If the system does not support a keyboard, we stub the functions out. */

#elif defined(OS_VXWORKS)

MX_EXPORT int
mx_getch( void )
{
	return 0;
}

MX_EXPORT int
mx_kbhit( void )
{
	return 0;
}

MX_EXPORT void
mx_key_getline( char *buffer, size_t max_buffer_length )
{
	if ( ( buffer != NULL ) && ( max_buffer_length > 0 ) ) {
		*buffer = '\0';
	}

	return;
}

#else

#error MX keyboard handling functions are not yet defined for this operating system.

#endif

