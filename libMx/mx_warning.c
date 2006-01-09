/*
 * Name:    mx_warning.c
 *
 * Purpose: Functions for displaying warning messages.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>

#include "mx_util.h"

static void (*mx_warning_output_function)( char * )
					= mx_warning_default_output_function;

MX_EXPORT void
mx_warning( char *format, ... )
{
	va_list args;
	char buffer[2500];

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	/* Display the message. */

	if ( mx_warning_output_function != NULL ) {
		(*mx_warning_output_function)( buffer );
	}
	return;
}

MX_EXPORT void
mx_set_warning_output_function( void (*output_function)(char *) )
{
	if ( output_function == NULL ) {
		mx_warning_output_function = mx_warning_default_output_function;
	} else {
		mx_warning_output_function = output_function;
	}
	return;
}

MX_EXPORT void
mx_warning_default_output_function( char *string )
{
#if defined(OS_WIN32)
	fprintf( stdout, "Warning: %s\n", string );
	fflush( stdout );
#else
	fprintf( stderr, "Warning: %s\n", string );
#endif

	return;
}

