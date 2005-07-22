/*
 * Name:    mrelay.c
 *
 * Purpose: Generic relay control command for use with X-ray shutters,
 *          filters, and so forth.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"
#include "mx_relay.h"

int
motor_open_fn( int argc, char *argv[] )
{
	static char usage[] = "Usage:  open [shutter_or_filter_name]\n";

	MX_RECORD *relay_record;
	char relay_record_name[MXU_RECORD_NAME_LENGTH + 1];
	mx_status_type status;

	if ( argc < 2 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	if ( argc == 2 ) {
		strcpy(relay_record_name, "shutter");
	} else {
		strcpy(relay_record_name, "");
		strncat(relay_record_name, argv[2], MXU_RECORD_NAME_LENGTH);
	}

	/* Try to find the record with the given name. */

	relay_record = mx_get_record( motor_record_list,
						relay_record_name );

	if ( relay_record == (MX_RECORD *) NULL ) {
		fprintf(output, "The record '%s' does not exist.\n",
					relay_record_name);
		return FAILURE;
	}

	/* Send the relay open command. */

	status = mx_relay_command( relay_record, MXF_OPEN_RELAY );

	if ( status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

int
motor_close_fn( int argc, char *argv[] )
{
	static char usage[] = "Usage:  close [shutter_or_filter_name]\n";

	MX_RECORD *relay_record;
	char relay_record_name[MXU_RECORD_NAME_LENGTH + 1];
	mx_status_type status;

	if ( argc <  2 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	if ( argc == 2 ) {
		strcpy(relay_record_name, "shutter");
	} else {
		strcpy(relay_record_name, "");
		strncat(relay_record_name, argv[2], MXU_RECORD_NAME_LENGTH);
	}

	/* Try to find the record with the given name. */

	relay_record = mx_get_record( motor_record_list,
						relay_record_name );

	if ( relay_record == (MX_RECORD *) NULL ) {
		fprintf(output, "The record '%s' does not exist.\n",
					relay_record_name);
		return FAILURE;
	}

	/* Send the relay close command. */

	status = mx_relay_command( relay_record, MXF_CLOSE_RELAY );

	if ( status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

