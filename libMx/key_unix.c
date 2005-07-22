/*
 * key_unix.c --  Unix emulation of DOS keyboard handling functions.  
 *
 *     Based on various stuff out of Stevens' books and from Usenet.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_key.h"

#if defined(OS_UNIX) || defined(OS_CYGWIN) || defined(OS_RTEMS)

#if defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_QNX) \
				|| defined(OS_RTEMS)
#   define USE_TERMIOS    1
#else
#   define USE_TERMIOS    0
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

/* This version of tty_raw() and tty_reset() are derived from the Unix
 * Programming FAQ found at http://www.erlenstar.demon.co.uk/unix/faq_toc.html
 */

static struct termios old_mode;		/* save tty mode here. */

static int
tty_raw( int fd )
{
	struct termios new_mode;
	int status;

	status = tcgetattr( fd, &old_mode );

	if ( status != 0 )
		return status;

	memcpy( &new_mode, &old_mode, sizeof( struct termios ) );

	/* Disable canonical mode and set buffer size to 1 byte */

	new_mode.c_lflag &= (~ICANON);
	new_mode.c_cc[VTIME] = 0;
	new_mode.c_cc[VMIN] = 1;

	return tcsetattr( fd, TCSANOW, &new_mode );
}

static int
tty_reset( int fd )
{
	return tcsetattr( fd, TCSANOW, &old_mode );
}

#else /* USE_TERMIOS */

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

#endif  /* USE_TERMIOS */

/*
 * Get a single character from standard input without permanently
 * changing terminal modes.
 */

MX_EXPORT int
mx_unix_getch( void )
{
	char ch;
	int result, nchars;

	if ( tty_raw( fileno(stdin) ) != 0 )
		return(-1);

	nchars = read( fileno(stdin), &ch, 1 );

	if ( nchars != 1 )
		return(-1);

	if ( tty_reset( fileno(stdin) ) != 0 )
		return(-1);

	result = (int) ch;

#if 0
	fprintf(stderr,"nchars = %d, ch = %d 0x%x '%c'\n", nchars, ch, ch, ch);
#endif

	return result;
}

/* 
 * kbhit() implemented using select().
 */

MX_EXPORT int
mx_unix_kbhit( void )
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

#if 0
	fprintf(stderr,"Before select: is_a_tty = %d\n", is_a_tty);
#endif

	if ( is_a_tty ) {
		if ( tty_raw( fileno(stdin) ) != 0 )
			return 0;
	}

	return_value = select(1 + fileno(stdin), &mask, NULL, NULL, &timeout);

	if ( is_a_tty ) {
		if ( tty_reset( fileno(stdin) ) != 0 )
			return 0;
	}

#if 0
	fprintf(stderr,"After select: return value = %d\n", return_value);
#endif
	return return_value;
}

#endif /* OS_UNIX */
