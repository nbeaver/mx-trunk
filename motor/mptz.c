/*
 * Name:    mptz.c
 *
 * Purpose: Camera Pan/Tilt/Zoom support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "mx_ptz.h"
#include "mx_key.h"

#if 0

static mx_status_type
motor_wait_for_ptz( MX_RECORD *ptz_record, char *label )
{
	unsigned long busy, ptz_status;
	mx_status_type mx_status;

	busy = 1;

	while( busy ) {
		if ( mx_kbhit() ) {
			(void) mx_getch();

			mx_status = mx_ptz_zoom_stop( ptz_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			fprintf( output, "%s\n", label );
		}

		mx_status = mx_ptz_get_status(ptz_record, &ptz_status);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		busy = ptz_status & MXSF_PTZ_IS_BUSY;

		if ( busy ) {
			mx_msleep(500);
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif

int
motor_ptz_fn( int argc, char *argv[] )
{
	const char cname[] = "ptz";

	MX_RECORD *ptz_record;
	MX_PAN_TILT_ZOOM *ptz;
	unsigned long ulong_value;
	int status;
	mx_status_type mx_status;

	static char usage[] =
"Usage:  ptz 'ptz_name' pan [ left | right | stop | 'number' ]\n"
"        ptz 'ptz_name' tilt [ up | down | stop | 'number' ]\n"
"        ptz 'ptz_name' zoom [ in | out | stop | 'number' ]\n"
"        ptz 'ptz_name' focus [ manual | auto | far | near | stop | 'number' ]\n"
	;

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	ptz_record = mx_get_record( motor_record_list, argv[2] );

	if ( ptz_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	ptz = (MX_PAN_TILT_ZOOM *) ptz_record->record_class_struct;

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		fprintf( output, "MX_PTZ pointer for record '%s' is NULL.\n",
				ptz_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( strncmp( "zoom", argv[3], strlen(argv[3]) ) == 0 ) {

		MX_DEBUG(-2,("%s: argc = %d", cname, argc));

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "in", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_ptz_zoom_in( ptz_record );
		} else
		if ( strncmp( "out", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_ptz_zoom_out( ptz_record );
		} else
		if ( strncmp( "stop", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_ptz_zoom_stop( ptz_record );

		} else {
			ulong_value = mx_string_to_unsigned_long( argv[4] );

			mx_status = mx_ptz_zoom_to( ptz_record, ulong_value );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else {
		/* Unrecognized command. */

		fprintf( output, "Unrecognized option '%s'\n\n", argv[3] );
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}
	return status;
}

