/*
 * Name:    mx_info.c
 *
 * Purpose: Functions for displaying informational output.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2002, 2005-2006, 2008 Illinois Institute of Technology
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

static void (*mx_info_entry_dialog_function)( char *,
						char *, int, char *, size_t )
					= mx_info_default_entry_dialog_function;

MX_EXPORT void
mx_info( const char *format, ... )
{
	va_list args;
	char buffer[2500];

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
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

/*--------------------------------------------------------------*/

MX_EXPORT void
mx_info_entry_dialog( char *text_prompt, char *gui_prompt,
			int echo_characters,
			char *response, size_t max_response_length )
{
	if ( mx_info_entry_dialog_function != NULL ) {
		( *mx_info_entry_dialog_function )( text_prompt, gui_prompt,
			echo_characters, response, max_response_length );
	}
	return;
}

MX_EXPORT void
mx_set_info_entry_dialog_function( void (*entry_dialog_function)(char *,
						char *, int, char *, size_t ) )
{
	if ( entry_dialog_function == NULL ) {
		mx_info_entry_dialog_function
				= mx_info_default_entry_dialog_function;
	} else {
		mx_info_entry_dialog_function = entry_dialog_function;
	}
	return;
}

MX_EXPORT void
mx_info_default_entry_dialog_function( char *text_prompt, char *gui_prompt,
			int echo_characters,
			char *response, size_t max_response_length )
{
#if defined(OS_WIN32)
	fprintf( stdout, "%s", text_prompt );  fflush( stdout );
#else
	fprintf( stderr, "%s", text_prompt );
#endif

	if ( echo_characters == FALSE ) {
		mx_key_echo_off();
	}

	mx_key_getline( response, max_response_length );

	if ( echo_characters == FALSE ) {
		mx_key_echo_on();

#if defined(OS_WIN32)
		fprintf( stdout, "\n" ); fflush( stdout );
#else
		fprintf( stderr, "\n" );
#endif
	}

	return;
}

/*--------------------------------------------------------------*/

/* Informational messages during a scan are handled specially,
 * since some applications may want to suppress them, but not
 * suppress other messages.
 */

static int mx_scanlog_enabled = TRUE;

MX_EXPORT void
mx_scanlog_info( const char *format, ... )
{
	va_list args;
	char buffer[2500];

	if ( mx_scanlog_enabled == FALSE )
		return;

	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	/* Display the message. */

	if ( mx_info_output_function != NULL ) {
		(*mx_info_output_function)( buffer );
	}
	return;
}

MX_EXPORT void
mx_set_scanlog_enable( int enable )
{
	if ( enable ) {
		mx_scanlog_enabled = TRUE;
	} else {
		mx_scanlog_enabled = FALSE;
	}
}

MX_EXPORT int
mx_get_scanlog_enable( void )
{
	return mx_scanlog_enabled;
}

