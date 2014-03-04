/*
 * Name:     mx_syslog.c
 *
 * Purpose:  Handler for Unix style syslogd daemon support.
 *
 * Author:   William Lavender
 *
 * Warning:  Support for syslog on OpenVMS has not yet been tested.  All I
 *           know at present is that it successfully links with Compaq TCP/IP
 *           Services for OpenVMS.
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2005-2006, 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_osdef.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_syslog.h"

#if ( MX_SYSLOG_IS_AVAILABLE == FALSE )

MX_EXPORT mx_status_type
mx_install_syslog_handler( char *ident_string,
				int log_number,
				int log_options )
{

	mx_info(
    "Unix style syslog support is not available for this operating system." );

	return MX_SUCCESSFUL_RESULT;
}

#else /* MX_SYSLOG_IS_AVAILABLE */

/*---*/

static void mx_syslog_handler( int level, char *message )
{
	size_t i, length;

	if ( message == NULL ) {
		fprintf( stderr,
		"mx_syslog_handler(): Discarded NULL message pointer.\n" );

		return;
	}

	/* Sanitize format characters in the incoming string by changing
	 * all % characters into # characters.
	 */

	length = strlen( message );

	for ( i = 0; i < length; i++ ) {
		if ( message[i] == '%' ) {
			message[i] = '#';
		}
	}

	/* Now that we have sanitized the message, we can safely send
	 * it on to syslog().
	 */

#if ( MX_GNUC_VERSION >= 4006000L )
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

	syslog( level, message );

#if ( MX_GNUC_VERSION >= 4006000L )
#  pragma GCC diagnostic pop
#endif

	return;
}

/*---*/

static void mx_syslog_info_handler( char *message )
{
	mx_syslog_handler( LOG_INFO, message );
}

static void mx_syslog_warning_handler( char *message )
{
	mx_syslog_handler( LOG_WARNING, message );
}

static void mx_syslog_error_handler( char *message )
{
	mx_syslog_handler( LOG_ERR, message );
}

static void mx_syslog_debug_handler( char *message )
{
	mx_syslog_handler( LOG_DEBUG, message );
}

MX_EXPORT mx_status_type
mx_install_syslog_handler( char *ident_string,
				int log_number,
				int log_options )
{
	const char fname[] = "mx_install_syslog_handler()";

	int syslog_facility, syslog_options;

	syslog_facility = -1;
	syslog_options = 0;

	switch ( log_number ) {
	case 0:
		syslog_facility = LOG_LOCAL0;
		break;
	case 1:
		syslog_facility = LOG_LOCAL1;
		break;
	case 2:
		syslog_facility = LOG_LOCAL2;
		break;
	case 3:
		syslog_facility = LOG_LOCAL3;
		break;
	case 4:
		syslog_facility = LOG_LOCAL4;
		break;
	case 5:
		syslog_facility = LOG_LOCAL5;
		break;
	case 6:
		syslog_facility = LOG_LOCAL6;
		break;
	case 7:
		syslog_facility = LOG_LOCAL7;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal syslog log number %d.  The allowed values are from 0 to 7.",
			log_number );
	}

#ifdef LOG_PERROR
	if ( log_options & MXF_SYSLOG_USE_STDERR ) {
		syslog_options = LOG_PERROR | LOG_PID;
	} else {
		syslog_options = LOG_PID;
	}
#else
	syslog_options = LOG_PID;
#endif

	openlog( ident_string, syslog_options, syslog_facility );

	mx_set_info_output_function( mx_syslog_info_handler );
	mx_set_warning_output_function( mx_syslog_warning_handler );
	mx_set_error_output_function( mx_syslog_error_handler );
	mx_set_debug_output_function( mx_syslog_debug_handler );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* MX_SYSLOG_IS_AVAILABLE */

