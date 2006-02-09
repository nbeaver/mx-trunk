/*
 * Name:    mstop.c
 *
 * Purpose: Device stop function.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2006 Illinois Institute of Technology
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
motor_stop_fn( int argc, char *argv[] )
{
	MX_RECORD *record;
	int32_t int32_value;
	double double_value;
	mx_status_type mx_status;

	if ( argc != 3 ) {
		fprintf(output,
"Usage:  stop 'devicename' - stops moving or counting for 'devicename'\n"
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
		fprintf(output,"Stop is only supported for device records.\n");
		return FAILURE;
	}

	switch( record->mx_class ) {
	case MXC_MOTOR:
		mx_status = mx_motor_soft_abort( record );
		break;
	case MXC_SCALER:
		mx_status = mx_scaler_stop( record, &int32_value );
		break;
	case MXC_TIMER:
		mx_status = mx_timer_stop( record, &double_value );
		break;
	case MXC_MULTICHANNEL_ANALYZER:
		mx_status = mx_mca_stop( record );
		break;
	case MXC_MULTICHANNEL_SCALER:
		mx_status = mx_mcs_stop( record );
		break;
	case MXC_PULSE_GENERATOR:
		mx_status = mx_pulse_generator_stop( record );
		break;
	default:
		fprintf(output, "Stop is not supported for '%s' records.\n",
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

