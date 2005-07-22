/*
 * key_dos.c --  An OS independent interface to DOS style keyboard 
 *               handling functions.  
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */


#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_key.h"

#ifdef OS_MSDOS

#include <conio.h>	/* A DOS specific header file. */
#include <dos.h>	/* A DOS specific header file. */

/*
 * Get a single character from standard input without permanently
 * changing terminal modes.
 */

MX_EXPORT int
mx_msdos_getch( void )
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
mx_msdos_kbhit( void )
{
	int result;

	result = kbhit();

	return result;
}

#endif /* OS_MSDOS */
