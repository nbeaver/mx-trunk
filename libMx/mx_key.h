/*
 * Name:    mx_key.h
 *
 * Purpose: MSDOS style keyboard handling functions.  
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "mx_osdef.h"

#if USE_KEY_CURSES

MX_API int mx_curses_getch( void );
MX_API int mx_curses_kbhit( void );

#define mx_getch	mx_curses_getch
#define mx_kbhit	mx_curses_kbhit

#else /* not USE_KEY_CURSES */

#if defined(OS_UNIX) || defined(OS_CYGWIN)

MX_API int mx_unix_getch( void );
MX_API int mx_unix_kbhit( void );

#define mx_getch	mx_unix_getch
#define mx_kbhit	mx_unix_kbhit

#endif /* OS_UNIX */

#if defined(OS_WIN32)

MX_API int mx_win32_getch( void );
MX_API int mx_win32_kbhit( void );

#define mx_getch	mx_win32_getch
#define mx_kbhit	mx_win32_kbhit

#endif /* OS_WIN32 */

#if defined(OS_MSDOS)

MX_API int mx_msdos_getch( void );
MX_API int mx_msdos_kbhit( void );

#define mx_getch	mx_msdos_getch
#define mx_kbhit	mx_msdos_kbhit

#endif /* OS_MSDOS */

#if defined(OS_VMS)

MX_API int mx_vms_getch( void );
MX_API int mx_vms_kbhit( void );

#define mx_getch	mx_vms_getch
#define mx_kbhit	mx_vms_kbhit

#endif /* OS_VMS */

#if defined(OS_VXWORKS) || defined(OS_RTEMS)

/* Keyboard characters are never available. */

#define mx_getch()	(0)
#define mx_kbhit()	(0)

#endif /* OS_VXWORKS */

#endif /* not USE_KEY_CURSES */

#ifndef mx_getch
#error  "MX keyboard handling functions not defined for this operating system."
#endif

