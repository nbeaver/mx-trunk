/*
 * Name:    mx_util_poison.c
 *
 * Purpose: A few MX routines (on some platforms) have to use functions
 *          that are otherwise banned from use elsewhere in MX by
 *          the GCC "poison" pragma.  This file provides a place for
 *          such routines to live.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_NO_POISON

#include <stdio.h>

#include "mx_util.h"

/*-------------------------------------------------------------------------*/

#if defined(OS_WIN32) && defined(_MSC_VER)

/* The Win32 functions _snprintf() and _vsnprintf() do not null terminate
 * the strings they return if the output was truncated.  This is inconsitent
 * with the Unix definition of these functions, so we cannot simply define
 * snprintf() as _snprintf().  Instead, we provide wrapper functions that
 * make sure that the output string is always terminated.  Thanks to the
 * authors of http://www.di-mgt.com.au/cprog.html for pointing this out.
 */

MX_EXPORT int
mx_snprintf( char *dest, size_t maxlen, const char *format, ... )
{
	va_list args;
	int result;

	va_start( args, format );

	result = _vsnprintf( dest, maxlen-1, format, args );

	if ( result < 0 ) {
		dest[ maxlen-1 ] = '\0';
	}

	va_end( args );

	return result;
}

MX_EXPORT int
mx_vsnprintf( char *dest, size_t maxlen, const char *format, va_list args  )
{
	int result;

	result = _vsnprintf( dest, maxlen-1, format, args );

	if ( result < 0 ) {
		dest[ maxlen-1 ] = '\0';
	}

	return result;
}

#elif defined(OS_VXWORKS) || defined(OS_DJGPP) \
	|| (defined(OS_VMS) && (__VMS_VER < 70320000))

/* Some platforms do not provide snprintf() and vsnprintf().  For those
 * platforms we fall back to sprintf() and vsprintf().  The hope in doing
 * this is that any buffer overruns will be found on the plaforms that
 * _do_ support snprintf() and vsnprint(), since I really do not want to
 * bundle my own version of vsnprintf() with MX.
 */

MX_EXPORT int
mx_snprintf( char *dest, size_t maxlen, const char *format, ... )
{
	va_list args;
	int result;

	va_start( args, format );

	result = vsprintf( dest, format, args );

	va_end( args );

	return result;
}

MX_EXPORT int
mx_vsnprintf( char *dest, size_t maxlen, const char *format, va_list args  )
{
	return vsprintf( dest, format, args );
}

#endif

/*-------------------------------------------------------------------------*/
