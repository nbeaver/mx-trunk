/*
 * Name:    mx_update.c
 *
 * Purpose: This function attempts to ensure that the data values contained
 *          in the MX record structures are consistent with the current
 *          status of the actual hardware.  Normally, for performance
 *          reasons, only values that expected to change frequently are
 *          updated.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_encoder.h"
#include "mx_scaler.h"
#include "mx_relay.h"
#include "mx_amplifier.h"
#include "mx_mca.h"
#include "mx_autoscale.h"
#include "mx_sca.h"
#include "mx_sample_changer.h"
#include "mx_mcai.h"
#include "mx_pulse_generator.h"
#include "mx_ptz.h"
#include "mx_variable.h"

MX_EXPORT mx_status_type
mx_update_record_values( MX_RECORD *record )
{
	const char fname[] = "mx_update_record_values()";

	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_RECORD * );
	int int_value;
	long long_value;
	uint32_t uint32_value;
	double double_value;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	flist = (MX_RECORD_FUNCTION_LIST *) record->record_function_list;

	if ( flist == (MX_RECORD_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD_FUNCTION_LIST pointer for record '%s' is NULL.",
			record->name );
	}

#if 0
	/* Does this record have an 'update_record_values' function? */

	fptr = flist->update_record_values;

	if ( fptr != NULL ) {
		/* It does, so let's invoke it. */

		status = ( *fptr )( record );

	} else {
		/* Otherwise, we use a generic response. */
#else
	fptr = NULL;

	if ( 1 ) {
#endif

		switch ( record->mx_superclass ) {
		case MXR_DEVICE:
			switch( record->mx_class ) {
			case MXC_ANALOG_INPUT:
				status = mx_analog_input_read(
							record, &double_value);
				break;
			case MXC_ANALOG_OUTPUT:
				status = mx_analog_output_read(
							record, &double_value);
				break;
			case MXC_DIGITAL_INPUT:
				status = mx_digital_input_read(
							record, &uint32_value);
				break;
			case MXC_DIGITAL_OUTPUT:
				status = mx_digital_output_read(
							record, &uint32_value);
				break;
			case MXC_MOTOR:
				status = mx_motor_get_position(
							record, &double_value);
				break;
			case MXC_ENCODER:
				status = mx_encoder_read(record, &long_value);
				break;
			case MXC_SCALER:
				status = mx_scaler_read( record, NULL );

				if ( status.code != MXE_SUCCESS )
					return status;

				status = mx_scaler_get_dark_current(
								record, NULL );
				break;
			case MXC_RELAY:
				status = mx_get_relay_status(
							record, &int_value);
				break;
			case MXC_AMPLIFIER:
				status = mx_amplifier_get_gain(
							record, &double_value);

				if ( status.code != MXE_SUCCESS ) {
					break;
				}

				status = mx_amplifier_get_offset(
							record, &double_value);

				if ( status.code != MXE_SUCCESS ) {
					break;
				}

				status = mx_amplifier_get_time_constant(
							record, &double_value);

				break;
			case MXC_AUTOSCALE:
				status = mx_autoscale_get_limits( record,
						NULL, NULL, NULL, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}

				status = mx_autoscale_get_offset_index( record,
								NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}

				status = mx_autoscale_get_offset_array( record,
								NULL, NULL );
				break;
			case MXC_SINGLE_CHANNEL_ANALYZER:
				status = mx_sca_get_lower_level( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_sca_get_upper_level( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_sca_get_gain( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_sca_get_time_constant(record, NULL);

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_sca_get_mode( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				break;
			case MXC_PULSE_GENERATOR:
				status = mx_pulse_generator_get_pulse_period(
							record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_pulse_generator_get_pulse_width(
							record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_pulse_generator_get_num_pulses(
							record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_pulse_generator_get_pulse_delay(
							record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_pulse_generator_get_mode(
							record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_pulse_generator_is_busy(
							record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				break;
			case MXC_SAMPLE_CHANGER:
				status = mx_sample_changer_get_status(
							record, NULL);
				break;
			case MXC_MULTICHANNEL_ANALOG_INPUT:
				status = mx_mcai_read( record, NULL, NULL );
				break;
			case MXC_PAN_TILT_ZOOM:
				status = mx_ptz_get_pan( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_ptz_get_tilt( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_ptz_get_zoom( record, NULL );

				if ( status.code != MXE_SUCCESS ) {
					break;
				}
				status = mx_ptz_get_focus( record, NULL );
				break;
			case MXC_TIMER:
			case MXC_MULTICHANNEL_ANALYZER:
			case MXC_MULTICHANNEL_ENCODER:
			case MXC_MULTICHANNEL_SCALER:
			case MXC_TABLE:
			case MXC_CCD:
				status = MX_SUCCESSFUL_RESULT;
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"%s does not yet have a generic response defined for device record class %ld",
					fname, record->mx_class );
				break;
			}
			break;

		case MXR_VARIABLE:
			status = mx_receive_variable( record );
			break;

		default:
			/* Do nothing. */

			status = MX_SUCCESSFUL_RESULT;
			break;
		}
	}
	return status;
}

