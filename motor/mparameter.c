/*
 * Name:    mparameter.c
 *
 * Purpose: Motor read/write parameters functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"

int
motor_readp_fn( int argc, char *argv[] )
{
	static const char cname[] = "readparams";

#if 1
	fprintf( output, "%s has been deleted.\n", cname );
#else
	MX_RECORD *record;
	mx_status_type status;

	if ( argc != 3 ) {
		fprintf( output, "Usage:  %s 'record_name'\n", cname );
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == NULL ) {
		fprintf( output, "%s: Record '%s' does not exist.\n",
					cname, argv[2] );
		return FAILURE;
	}

	status = mx_read_parms_from_hardware( record );

	if ( status.code != MXE_SUCCESS )
		return FAILURE;

#endif
	return SUCCESS;
}

int
motor_writep_fn( int argc, char *argv[] )
{
	static const char cname[] = "writeparams";

#if 1
	fprintf( output, "%s has been deleted.\n", cname );
#else
	MX_RECORD *record;
	mx_status_type status;

	if ( argc != 3 ) {
		fprintf( output, "Usage:  %s 'record_name'\n", cname );
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == NULL ) {
		fprintf( output, "%s: Record '%s' does not exist.\n",
					cname, argv[2] );
		return FAILURE;
	}

	status = mx_write_parms_to_hardware( record );

	if ( status.code != MXE_SUCCESS )
		return FAILURE;

#endif
	return SUCCESS;
}

