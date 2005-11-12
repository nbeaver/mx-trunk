/*
 * Name:    mx_hrt_debug.c
 *
 * Purpose: Debugging functions for the MX high resolution time interface.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdarg.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"

MX_EXPORT void
MX_HRT_RESULTS( MX_HRT_TIMING x, const char *fname, char *format, ... )
{
	va_list args;
	static char buffer[250];

	if ( mx_get_debug_level() < -2 )
		return;

	fprintf( stderr, "%s: Elapsed time = %g usec ",
		fname, 1.0e6 * x.time_in_seconds );

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	fprintf( stderr, "%s\n", buffer );
	fflush( stderr );

	return;
}

