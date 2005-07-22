/*
 * Name:    key_curses.c
 *
 * Purpose: Emulation of DOS keyboard handling functions using curses.
 *          The version of curses used must supply the function
 *          keypad() in order to be useable by the functions in
 *          this file.  Generally, the best solution in the absence
 *          of a recent enough copy of curses on a machine is to
 *          copy the freely available ncurses package to it.
 *
 *          Note that these functions expect curses to have already
 *          been initialized before any of them are called.
 *          They do no initialization of their own.
 *
 *          Also note that this code expects getch() to normally
 *          be a blocking call.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_key.h"

#if USE_KEY_CURSES

#ifdef OS_UNIX
#include <curses.h>
#else
#include "curses.h"
#endif

/*
 * Get a single character from the terminal.
 */

MX_EXPORT int
mx_curses_getch( void )
{
	return getch();
}

MX_EXPORT int
mx_curses_kbhit( void )
{
	int key, result;

	/* Set getch() to temporarily be non-blocking. */

	nodelay(stdscr, TRUE);

	/* Try to read a key. */

	key = getch();

	if ( key == ERR ) {		/* No key was seen. */
		result = FALSE;
	} else {			/* A key was seen. */

		/* Push the key back onto the input queue. */

		ungetch( key );

		result = TRUE;
	}

	/* Set getch() back to blocking. */

	nodelay(stdscr, FALSE);

	return result;
}

#endif /* USE_KEY_CURSES */

