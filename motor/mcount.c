/*
 * Name:    mcount.c
 *
 * Purpose: Scaler count function.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "motor.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_amplifier.h"

int
motor_count_fn( int argc, char *argv[] )
{
	const char cname[] = "count";

	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_RECORD *timer_record;
	MX_RECORD_FIELD *value_field;
	int i, j, num_input_devices, entries_to_skip, busy;
	double counting_time, remaining_time, double_value;
	unsigned long ulong_value;
	long long_value;
	mx_status_type mx_status;

	static char usage[] =
	  "Usage:  count 'counting_time' ['timer'] 'scaler1' 'scaler2' ...\n";

	if ( argc <= 3 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	counting_time = atof( argv[2] );

	/* Does argv[3] refer to a timer record? */

	timer_record = mx_get_record( motor_record_list, argv[3] );

	if ( timer_record == (MX_RECORD *) NULL ) {
		fprintf( output, "Record '%s' does not exist.\n", argv[3] );
		return FAILURE;
	}

	if ( ( timer_record->mx_superclass == MXR_DEVICE )
	  && ( timer_record->mx_class == MXC_TIMER ) )
	{
		/* Yes, it is a timer record, so skip it in the list
		 * of input devices.
		 */

		entries_to_skip = 4;
	} else {
		/* No, it is not a timer record, so we must look for 
		 * a timer called 'timer1'.
		 */

		entries_to_skip = 3;

		timer_record = mx_get_record( motor_record_list, "timer1" );

		if ( timer_record == (MX_RECORD *) NULL ) {
			fprintf( output,
			"Timer record 'timer1' does not exist.\n" );

			return FAILURE;
		}
	}

	/* Configure the timer for preset time mode. */

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Create an array of the input device records. */

	num_input_devices = argc - entries_to_skip;

	if ( num_input_devices == 0 ) {
		input_device_array = NULL;
	} else {
		input_device_array = (MX_RECORD **) malloc( num_input_devices
						* sizeof(MX_RECORD *) );

		if ( input_device_array == NULL ) {
			fprintf( output,
		"Attempt to allocate array of input devices failed.\n" );

			return FAILURE;
		}
	}

	for ( i = entries_to_skip; i < argc; i++ ) {
		j = i - entries_to_skip;

		input_device_array[j] = mx_get_record( motor_record_list,
							argv[i] );

		if ( input_device_array[j] == NULL ) {
			fprintf( output,
			"Input device record '%s' does not exist.\n", argv[i]);

			mx_free( input_device_array );
			return FAILURE;
		}
		if ( ( input_device_array[j]->mx_superclass != MXR_DEVICE )
		  && ( input_device_array[j]->mx_superclass != MXR_VARIABLE) )
		{
			fprintf( output,
			"Record '%s' is not an input device.\n", argv[i]);

			mx_free( input_device_array );
			return FAILURE;
		}

		/* Clear any input devices that need it. */

		switch( input_device_array[j]->mx_superclass ) {
		case MXR_VARIABLE:

		    /* Do nothing for variables. */

		    break;

		case MXR_DEVICE:

		    switch( input_device_array[j]->mx_class ) {
		    case MXC_SCALER:
#if 0
			mx_status = mx_scaler_set_mode( input_device_array[j],
							MXCM_COUNTER_MODE );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_free( input_device_array );
				return FAILURE;
			}
#endif
			mx_status = mx_scaler_clear( input_device_array[j] );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_free( input_device_array );
				return FAILURE;
			}
			break;
		    case MXC_ANALOG_INPUT:
		    case MXC_ANALOG_OUTPUT:
		    case MXC_DIGITAL_INPUT:
		    case MXC_DIGITAL_OUTPUT:
		    case MXC_MOTOR:
		    case MXC_AMPLIFIER:
			break;
		    default:
			fprintf( output,
			"Record '%s' cannot be used by the %s command.\n", 
				argv[i], cname );
			mx_free( input_device_array );
			return FAILURE;
			break;
		    }
		    break;

		default:
		    fprintf( output,
			"Record '%s' cannot be used by the %s command.\n", 
				argv[i], cname );
		    mx_free( input_device_array );
		    return FAILURE;
		    break;
		}
	}

	/* Perform the measurement. */

	mx_status = mx_timer_start( timer_record, counting_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( input_device_array );
		return FAILURE;
	}

	busy = TRUE;

	while ( busy ) {

		if ( mx_user_requested_interrupt() ) {
			(void) mx_timer_stop( timer_record, &remaining_time );

			fprintf( output,
			"Measurement integration time was interrupted.\n" );

			break;		/* Exit the while() loop. */
		}

		mx_status = mx_timer_is_busy( timer_record, &busy );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( input_device_array );
			return FAILURE;
		}

		mx_msleep(1);
	}

	/* Read out the input devices. */

	for ( i = 0; i < num_input_devices; i++ ) {
	    input_device = input_device_array[i];

	    switch( input_device->mx_superclass ) {
	    case MXR_VARIABLE:
		mx_status = mx_update_record_values( input_device );

		if ( mx_status.code == MXE_SUCCESS ) {
			mx_status = mx_find_record_field( input_device,
						"value", &value_field );

			if ( mx_status.code == MXE_SUCCESS ) {
				(void) motor_print_field_array( input_device,
						value_field, FALSE );

				fprintf( output, " " );
			}
		}
		break;
	    case MXR_DEVICE:

		switch( input_device->mx_class ) {
		case MXC_ANALOG_INPUT:
			mx_status = mx_analog_input_read(
						input_device, &double_value );
			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%.*g ",
						input_device->precision,
						double_value );
			}
			break;
		case MXC_ANALOG_OUTPUT:
			mx_status = mx_analog_output_read(
						input_device, &double_value );
			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%.*g ",
						input_device->precision,
						double_value );
			}
			break;
		case MXC_DIGITAL_INPUT:
			mx_status = mx_digital_input_read(
						input_device, &ulong_value );
			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%lu ", ulong_value );
			}
			break;
		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_read(
						input_device, &ulong_value );
			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%lu ", ulong_value );
			}
			break;
		case MXC_MOTOR:
			mx_status = mx_motor_get_position(
						input_device, &double_value );
			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%.*g ",
						input_device->precision,
						double_value );
			}
			break;
		case MXC_SCALER:
			mx_status = mx_scaler_read( input_device, &long_value );
			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%ld ", long_value );
			}
			break;
		case MXC_AMPLIFIER:
			mx_status = mx_amplifier_get_gain( input_device,
							&double_value );

			if ( mx_status.code == MXE_SUCCESS ) {
				fprintf( output, "%.*g ",
						input_device->precision,
						double_value );
			}
			break;
		}
		if ( mx_status.code != MXE_SUCCESS ) {
			mx_free( input_device_array );
			return FAILURE;
		}
		break;
	    }
	}
	fprintf( output, "\n" );

	mx_free( input_device_array );

	return SUCCESS;
}

