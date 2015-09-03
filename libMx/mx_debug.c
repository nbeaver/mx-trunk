/*
 * Name:    mx_debug.c
 *
 * Purpose: Functions for displaying debugging output.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006, 2008-2009, 2011, 2015
 *     Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdarg.h>

#include "mx_util.h"

static int mx_debug_level = 0;
static void (*mx_debug_output_function)( char * )
				= mx_debug_default_output_function;

MX_EXPORT void
mx_debug_function( const char *format, ... )
{
	va_list args;
	static char buffer[2500];

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	(*mx_debug_output_function)( buffer );

	return;
}

MX_EXPORT void
mx_set_debug_level( int level )
{
	mx_debug_level = level;
}

MX_EXPORT int
mx_get_debug_level( void )
{
	return mx_debug_level;
}

MX_EXPORT void
mx_set_debug_output_function( void (*output_function)(char *) )
{
	if ( output_function == NULL ) {
		mx_debug_output_function = mx_debug_default_output_function;

	} else {
		mx_debug_output_function = output_function;
	}
	return;
}

MX_EXPORT void
mx_debug_default_output_function( char *string )
{
#if defined(OS_WIN32)
	fprintf( stdout, "%s\n", string );
	fflush( stdout );
#else
	fprintf( stderr, "%s\n", string );
#endif

	return;
}

MX_EXPORT void
mx_debug_pause( const char *format, ... )
{
	va_list args;
	static char buffer[250];

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	(*mx_debug_output_function)( buffer );

	mx_fgets( buffer, sizeof(buffer), stdin );

	return;
}

