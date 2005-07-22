/*
 * Name:    key_win32.c
 *
 * Purpose: Emulation of DOS keyboard handling functions under Win32.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_key.h"

#ifdef OS_WIN32

#include <conio.h>

/*
 * Get a single character from the terminal.
 */

MX_EXPORT int
mx_win32_getch( void )
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
mx_win32_kbhit( void )
{
	return kbhit();
}

#endif /* OS_WIN32 */

