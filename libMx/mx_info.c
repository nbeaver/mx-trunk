/*
 * Name:    mx_info.c
 *
 * Purpose: Functions for displaying informational output.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "mx_util.h"
#include "mx_key.h"

static void (*mx_info_output_function)( char * )
					= mx_info_default_output_function;

static void (*mx_info_dialog_function)( char *, char *, char * )
					= mx_info_default_dialog_function;

MX_EXPORT void
mx_info( char *format, ... )
{
	va_list args;
	char buffer[2500];

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	/* Display the message. */

	if ( mx_info_output_function != NULL ) {
		(*mx_info_output_function)( buffer );
	}
	return;
}

MX_EXPORT void
mx_set_info_output_function( void (*output_function)(char *) )
{
	if ( output_function == NULL ) {
		mx_info_output_function = mx_info_default_output_function;
	} else {
		mx_info_output_function = output_function;
	}
	return;
}

MX_EXPORT void
mx_info_default_output_function( char *string )
{
#if defined(OS_WIN32)
	fprintf( stdout, "%s\n", string );
	fflush( stdout );
#else
	fprintf( stderr, "%s\n", string );
#endif

	return;
}

/*--------------------------------------------------------------*/

MX_EXPORT void
mx_info_dialog( char *text_prompt, char *gui_prompt, char *button_label )
{
	if ( mx_info_dialog_function != NULL ) {
		( *mx_info_dialog_function )( text_prompt,
						gui_prompt, button_label );
	}
	return;
}

MX_EXPORT void
mx_set_info_dialog_function( void (*dialog_function)(char *, char *, char *) )
{
	if ( dialog_function == NULL ) {
		mx_info_dialog_function = mx_info_default_dialog_function;
	} else {
		mx_info_dialog_function = dialog_function;
	}
	return;
}

MX_EXPORT void
mx_info_default_dialog_function( char *text_prompt, char *gui_prompt,
					char *button_label )
{
#if defined(OS_WIN32)
	fprintf( stdout, "%s", text_prompt );
	fflush( stdout );

	(void) mx_getch();

	fprintf( stdout, "\n" );
	fflush( stdout );
#else
	fprintf( stderr, "%s", text_prompt );

	(void) mx_getch();

	fprintf( stderr, "\n" );
#endif

	return;
}

