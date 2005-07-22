/*
 * Name:    mstart.c
 *
 * Purpose: Device start function.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>

#include "motor.h"

#include "mx_mca.h"
#include "mx_mcs.h"
#include "mx_pulse_generator.h"

int
motor_start_fn( int argc, char *argv[] )
{
	MX_RECORD *record;
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(output,
"Usage:  start 'devicename' - starts moving or counting for 'devicename'\n"
		);
		return FAILURE;
	}

	record = mx_get_record( motor_record_list, argv[2] );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf( output, "Record '%s' does not exist.\n",
			argv[2] );

		return FAILURE;
	}

	/* Find out what kind of device this is. */

	if ( record->mx_superclass == MXR_INTERFACE ) {
		fprintf(output,"Start is only supported for device records.\n");
		return FAILURE;
	}

	switch( record->mx_class ) {
	case MXC_MULTICHANNEL_ANALYZER:
		mx_status = mx_mca_start( record );
		break;
	case MXC_MULTICHANNEL_SCALER:
		mx_status = mx_mcs_start( record );
		break;
	case MXC_PULSE_GENERATOR:
		mx_status = mx_pulse_generator_start( record );
		break;
	default:
		fprintf(output, "Start is not supported for '%s' records.\n",
				mx_get_driver_name( record ) );

		return FAILURE;
		break;
	}

	if ( mx_status.code == MXE_SUCCESS ) {
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

