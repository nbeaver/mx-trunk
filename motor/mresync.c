/*
 * Name:    mresync.c
 *
 * Purpose: Motor command to invoked the resynchronize method for a record.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"

int
motor_resync_fn( int argc, char *argv[] )
{
	const char cname[] = "resynchronize";

	MX_RECORD *record;
	MX_RECORD_FUNCTION_LIST *record_function_list;
	mx_status_type ( *resynchronize_fn ) ( MX_RECORD * );
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf( output, "\nUsage: %s 'record_name'\n\n", cname );
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == NULL ) {
		fprintf( output, "\n%s: There is no record named '%s'.\n\n",
				cname, argv[2] );
		return FAILURE;
	}

	record_function_list = (MX_RECORD_FUNCTION_LIST *)
					record->record_function_list;

	resynchronize_fn = record_function_list->resynchronize;

	if ( resynchronize_fn == NULL ) {
		fprintf( output,
			"\n%s: Not currently supported for record '%s'.\n\n",
			cname, record->name );
		return FAILURE;
	}

	mx_status = ( *resynchronize_fn ) ( record );

	if ( mx_status.code == MXE_SUCCESS ) {
		fprintf( output,
			"\nResynchronization of record '%s' succeeded.\n\n",
			record->name );
		return SUCCESS;
	} else {
		fprintf( output,
			"\nResynchronization of record '%s' failed.\n\n",
			record->name );
		return FAILURE;
	}
}

