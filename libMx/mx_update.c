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
 * Copyright 1999-2009, 2014, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_area_detector.h"
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
#include "mx_operation.h"

MX_EXPORT mx_status_type
mx_update_record_values( MX_RECORD *record )
{
	static const char fname[] = "mx_update_record_values()";

	long long_value;
	unsigned long ulong_value;
	double double_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	switch ( record->mx_superclass ) {
	case MXR_DEVICE:
		switch( record->mx_class ) {
		case MXC_ANALOG_INPUT:
			mx_status = mx_analog_input_read(
						record, &double_value);
			break;
		case MXC_ANALOG_OUTPUT:
			mx_status = mx_analog_output_read(
						record, &double_value);
			break;
		case MXC_DIGITAL_INPUT:
			mx_status = mx_digital_input_read(
						record, &ulong_value);
			break;
		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_read(
						record, &ulong_value);
			break;
		case MXC_MOTOR:
			mx_status = mx_motor_get_position(
						record, &double_value);
			break;
		case MXC_ENCODER:
			mx_status = mx_encoder_read(record, &long_value);
			break;
		case MXC_SCALER:
			mx_status = mx_scaler_read( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_scaler_get_dark_current(
							record, NULL );
			break;
		case MXC_RELAY:
			mx_status = mx_get_relay_status(
						record, &long_value);
			break;
		case MXC_AMPLIFIER:
			mx_status = mx_amplifier_get_gain(
						record, &double_value);

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}

			mx_status = mx_amplifier_get_offset(
						record, &double_value);

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}

			mx_status = mx_amplifier_get_time_constant(
						record, &double_value);

			break;
		case MXC_AUTOSCALE:
			mx_status = mx_autoscale_get_limits( record,
					NULL, NULL, NULL, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}

			mx_status = mx_autoscale_get_offset_index( record,
							NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}

			mx_status = mx_autoscale_get_offset_array( record,
							NULL, NULL );
			break;
		case MXC_SINGLE_CHANNEL_ANALYZER:
			mx_status = mx_sca_get_lower_level( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_sca_get_upper_level( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_sca_get_gain( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_sca_get_time_constant(record, NULL);

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_sca_get_mode( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			break;
		case MXC_PULSE_GENERATOR:
			mx_status = mx_pulse_generator_get_pulse_period(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_pulse_generator_get_pulse_width(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_pulse_generator_get_num_pulses(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_pulse_generator_get_pulse_delay(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_pulse_generator_get_function_mode(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_pulse_generator_get_trigger_mode(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_pulse_generator_is_busy(
						record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			break;
		case MXC_SAMPLE_CHANGER:
			mx_status = mx_sample_changer_get_status(
						record, NULL);
			break;
		case MXC_MULTICHANNEL_ANALOG_INPUT:
			mx_status = mx_mcai_read( record, NULL, NULL );
			break;
		case MXC_PAN_TILT_ZOOM:
			mx_status = mx_ptz_get_pan( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_ptz_get_tilt( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_ptz_get_zoom( record, NULL );

			if ( mx_status.code != MXE_SUCCESS ) {
				break;
			}
			mx_status = mx_ptz_get_focus( record, NULL );
			break;
		case MXC_TIMER:
		case MXC_MULTICHANNEL_ANALYZER:
		case MXC_MULTICHANNEL_ENCODER:
		case MXC_MULTICHANNEL_SCALER:
		case MXC_TABLE:
		case MXC_AREA_DETECTOR:
		case MXC_VIDEO_INPUT:
		case MXC_WAVEFORM_OUTPUT:
			mx_status = MX_SUCCESSFUL_RESULT;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"%s does not yet have a generic response defined for device record class %ld",
				fname, record->mx_class );
		}
		break;

	case MXR_VARIABLE:
		mx_status = mx_receive_variable( record );
		break;

	case MXR_OPERATION:
		mx_status = mx_operation_get_status( record, NULL );
		break;

	default:
		/* Do nothing. */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	}

	return mx_status;
}

