/*
 * Name:     mx_measurement.c
 *
 * Purpose:  Functions used to implement scan measurements.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004-2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <limits.h>

#include "mxconfig.h"
#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_relay.h"
#include "mx_scaler.h"
#include "mx_amplifier.h"
#include "mx_relay.h"
#include "mx_mca.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_variable.h"
#include "mx_scan.h"

#include "mx_measurement.h"

#include "m_none.h"
#include "m_time.h"
#include "m_count.h"
#include "m_pulse_period.h"

MX_MEASUREMENT_TYPE_ENTRY mx_measurement_type_list[] = {
	{ MXM_NONE,         "none",         &mxm_none_function_list },
	{ MXM_PRESET_TIME,  "preset_time",  &mxm_preset_time_function_list },
	{ MXM_PRESET_COUNT, "preset_count", &mxm_preset_count_function_list },
	{ MXM_PRESET_PULSE_PERIOD, "preset_pulse_period",
				&mxm_preset_pulse_period_function_list },
	{ -1, "", NULL }
};

/* --------------- */

MX_EXPORT mx_status_type mx_get_measurement_type_by_name( 
	MX_MEASUREMENT_TYPE_ENTRY *measurement_type_list,
	char *name,
	MX_MEASUREMENT_TYPE_ENTRY **measurement_type_entry )
{
	static const char fname[] = "mx_get_measurement_type_by_name()";

	char *list_name;
	int i;

	if ( measurement_type_list == NULL ) {
		*measurement_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Measurement type list passed was NULL." );
	}

	if ( name == NULL ) {
		*measurement_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Measurement name pointer passed is NULL.");
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( measurement_type_list[i].type < 0 ) {
			*measurement_type_entry = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Measurement type '%s' was not found.", name );
		}

		list_name = measurement_type_list[i].name;

		if ( list_name == NULL ) {
			*measurement_type_entry = NULL;

			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"NULL name ptr for measurement type %ld.",
				measurement_type_list[i].type );
		}

		if ( strcmp( name, list_name ) == 0 ) {
			*measurement_type_entry = &(measurement_type_list[i]);

			MX_DEBUG( 8,
			("%s: ptr = 0x%p, type = %ld", fname,
				*measurement_type_entry,
				(*measurement_type_entry)->type));

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

MX_EXPORT mx_status_type mx_get_measurement_type_by_value( 
	MX_MEASUREMENT_TYPE_ENTRY *measurement_type_list,
	long measurement_type,
	MX_MEASUREMENT_TYPE_ENTRY **measurement_type_entry )
{
	static const char fname[] = "mx_get_measurement_type_by_value()";

	int i;

	if ( measurement_type_list == NULL ) {
		*measurement_type_entry = NULL;

		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Measurement type list passed was NULL." );
	}

	if ( measurement_type <= 0 ) {
		*measurement_type_entry = NULL;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Measurement type %ld is illegal.", measurement_type );
	}

	for ( i=0; ; i++ ) {
		/* Check for the end of the list. */

		if ( measurement_type_list[i].type < 0 ) {
			*measurement_type_entry = NULL;

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Measurement type %ld was not found.",
				measurement_type );
		}

		if ( measurement_type_list[i].type == measurement_type ) {
			*measurement_type_entry = &(measurement_type_list[i]);

			MX_DEBUG( 8,
			("%s: ptr = 0x%p, type = %ld", fname,
				*measurement_type_entry,
				(*measurement_type_entry)->type));

			return MX_SUCCESSFUL_RESULT;
		}
	}
}

/* --------------- */

MX_EXPORT mx_status_type
mx_configure_measurement_type( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mx_configure_measurement_type()";

	MX_MEASUREMENT_TYPE_ENTRY *measurement_type_entry;
	MX_MEASUREMENT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT * );
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed was NULL.");
	}

	mx_status = mx_get_measurement_type_by_value( mx_measurement_type_list,
				measurement->type, &measurement_type_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( measurement_type_entry == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"mx_get_measurement_type_by_value() returned a NULL "
			"measurement_type_entry pointer." );
	}

	flist = measurement_type_entry->measurement_function_list;

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MEASUREMENT_FUNCTION_LIST pointer for "
			"measurement type '%s' is NULL",
			measurement_type_entry->name );
	}

	/* Save a copy of the function list pointer. */

	measurement->measurement_function_list = flist;

	measurement->measurement_type_struct = NULL;

	/* Now invoke the configure function. */

	fptr = flist->configure;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"configure function pointer for measurement type '%s' is NULL",
			measurement_type_entry->name );
	}

	mx_status = (*fptr) ( measurement );

	return mx_status;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_deconfigure_measurement_type( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mx_deconfigure_measurement_type()";

	MX_MEASUREMENT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT * );
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed was NULL.");
	}

	flist = (MX_MEASUREMENT_FUNCTION_LIST *)
			(measurement->measurement_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "MX_MEASUREMENT_FUNCTION_LIST pointer for measurement is NULL");
	}

	/* Now invoke the deconfigure function. */

	fptr = flist->deconfigure;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "deconfigure function pointer for measurement type '%s' is NULL.",
			measurement->mx_typename );
	}

	mx_status = (*fptr) ( measurement );

	return mx_status;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_prescan_measurement_processing( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mx_prescan_measurement_processing()";

	MX_MEASUREMENT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT * );
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed was NULL.");
	}

	flist = (MX_MEASUREMENT_FUNCTION_LIST *)
			(measurement->measurement_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "MX_MEASUREMENT_FUNCTION_LIST pointer for measurement is NULL");
	}

	/* Now invoke the prescan processing function. */

	fptr = flist->prescan_processing;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "prescan_processing function pointer for measurement type '%s' is NULL.",
			measurement->mx_typename );
	}

	mx_status = (*fptr) ( measurement );

	return mx_status;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_postscan_measurement_processing( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mx_postscan_measurement_processing()";

	MX_MEASUREMENT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT * );
	mx_status_type mx_status;

	if ( measurement == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed was NULL.");
	}

	flist = (MX_MEASUREMENT_FUNCTION_LIST *)
			(measurement->measurement_function_list);

	if ( flist == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "MX_MEASUREMENT_FUNCTION_LIST pointer for measurement is NULL");
	}

	/* Now invoke the postscan processing function. */

	fptr = flist->postscan_processing;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "postscan_processing function pointer for measurement type '%s' is NULL.",
			measurement->mx_typename );
	}

	mx_status = (*fptr) ( measurement );

	return mx_status;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_acquire_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mx_acquire_data()";

	MX_MEASUREMENT_FUNCTION_LIST *flist;
	mx_status_type ( *fptr ) ( MX_MEASUREMENT * );

	MX_SCAN *scan;
	unsigned long seconds, milliseconds;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.",fname));

	if ( measurement == (MX_MEASUREMENT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed is NULL." );
	}

	flist = (MX_MEASUREMENT_FUNCTION_LIST *)
				measurement->measurement_function_list;

	if ( flist == (MX_MEASUREMENT_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_MEASUREMENT_FUNCTION_LIST pointer for measurement is NULL.");
	}

	fptr = flist->acquire_data;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"acquire_data function pointer for measurement is NULL.");
	}

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement pointer %p is NULL.",
			measurement );
	}

	/* Open the shutter if requested. */

	if ( scan->shutter_policy == MXF_SCAN_SHUTTER_OPEN_FOR_DATAPOINT ) {
		mx_status = mx_relay_command( scan->shutter_record,
							MXF_OPEN_RELAY );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Wait for the settling time. */

	if ( scan->settling_time >= (0.001 * (double) MX_ULONG_MAX) ) {

		seconds = mx_round( scan->settling_time );

		mx_sleep( seconds );
	} else {
		milliseconds = mx_round( 1000.0 * scan->settling_time );

		mx_msleep( milliseconds );
	}

	/* Perform the measurement. */

	mx_status = (*fptr) ( measurement );

	/* Close the shutter if requested. */

	if ( scan->shutter_policy == MXF_SCAN_SHUTTER_OPEN_FOR_DATAPOINT ) {
		(void) mx_relay_command( scan->shutter_record,
							MXF_CLOSE_RELAY );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_readout_data( MX_MEASUREMENT *measurement )
{
	static const char fname[] = "mx_readout_data()";

	MX_SCAN *scan;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_SCALER *scaler;
	MX_AREA_DETECTOR *ad;
	double double_value;
	long long_value;
	unsigned long ulong_value;
	long i;
	mx_status_type mx_status;

	scan = (MX_SCAN *) measurement->scan;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SCAN pointer for measurement pointer %p is NULL.",
			measurement );
	}

	input_device_array = scan->input_device_array;

	if ( input_device_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"input_device_array pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	/* Read out and save the values from the input devices. */

	for ( i = 0; i < scan->num_input_devices; i++ ) {
		input_device = input_device_array[i];

		switch ( input_device->mx_superclass ) {
		case MXR_VARIABLE:
			mx_status = mx_receive_variable( input_device );

			if ( mx_status.code != MXE_SUCCESS ) {
				return mx_status;
			}
			break;

		case MXR_DEVICE:
			switch( input_device->mx_class ) {
			case MXC_ANALOG_INPUT:
				mx_status = mx_analog_input_read(
						input_device, &double_value );
				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_ANALOG_OUTPUT:
				mx_status = mx_analog_output_read(
						input_device, &double_value );
				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_DIGITAL_INPUT:
				mx_status = mx_digital_input_read(
						input_device, &ulong_value );
				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_DIGITAL_OUTPUT:
				mx_status = mx_digital_output_read(
						input_device, &ulong_value );
				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_MOTOR:
				mx_status = mx_motor_get_position(
						input_device, &double_value );
				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_SCALER:
				mx_status = mx_scaler_read(
						input_device, &long_value );
				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				scaler = (MX_SCALER *)
					(input_device->record_class_struct);

				scaler->value = long_value;
				break;
			case MXC_TIMER:
				mx_status = mx_timer_read(
						input_device, &double_value );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_AMPLIFIER:
				mx_status = mx_amplifier_get_gain(
						input_device, &double_value );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_RELAY:
				mx_status = mx_get_relay_status(
						input_device, NULL );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_MULTICHANNEL_ANALYZER:
				mx_status = mx_mca_read( input_device,
								NULL, NULL);

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;
			case MXC_AREA_DETECTOR:
				ad = input_device->record_class_struct;

				if ( ad == (MX_AREA_DETECTOR *) NULL ) {
					return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
					"The MX_AREA_DETECTOR pointer for "
					"input device '%s' is NULL.",
						input_device->name );
				}

				if ( ad->transfer_image_during_scan == FALSE ) {
					return MX_SUCCESSFUL_RESULT;
				}

				/* Retrieve the most recently acquired image. */

				mx_status = mx_area_detector_get_frame(
					input_device, -1, &(ad->image_frame) );

				if ( mx_status.code != MXE_SUCCESS ) {
					return MX_SUCCESSFUL_RESULT;
				}
				break;
			default:
				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Device type %ld cannot be a scan input device.",
					input_device->mx_class );
				break;
			}
			break;

		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record superclass %ld cannot be a scan input device.",
				input_device->mx_superclass );
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_measurement_time( MX_MEASUREMENT *measurement,
			double *measurement_time )
{
	static const char fname[] = "mx_get_measurement_time()";
	
	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;

	MX_DEBUG( 2,("%s invoked.",fname));

	if ( measurement == (MX_MEASUREMENT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed is NULL." );
	}
	if ( measurement_time == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"measurement_time pointer passed is NULL." );
	}
	if ( measurement->measurement_type_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The measurement_type_struct pointer for the "
			"MX_MEASUREMENT structure passed is NULL." );
	}

	switch( measurement->type ) {
	case MXM_PRESET_TIME:
		preset_time_struct = (MX_MEASUREMENT_PRESET_TIME *)
					measurement->measurement_type_struct;

		*measurement_time = preset_time_struct->integration_time;
		break;
	case MXM_PRESET_PULSE_PERIOD:
		preset_pulse_period_struct = 
			(MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
			measurement->measurement_type_struct;

		*measurement_time = preset_pulse_period_struct->pulse_period;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The measurement time was requested for a measurement that is not "
"a preset time or preset pulse period measurement.  Instead, it was of "
"measurement type %ld.", measurement->type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_get_measurement_counts( MX_MEASUREMENT *measurement,
			long *measurement_counts )
{
	static const char fname[] = "mx_get_measurement_counts()";
	
	MX_MEASUREMENT_PRESET_COUNT *preset_count_struct;

	MX_DEBUG(-2,("%s invoked.",fname));

	if ( measurement == (MX_MEASUREMENT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MEASUREMENT pointer passed is NULL." );
	}
	if ( measurement_counts == (long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"measurement_counts pointer passed is NULL." );
	}
	if ( measurement->measurement_type_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The measurement_type_struct pointer for the "
			"MX_MEASUREMENT structure passed is NULL." );
	}

	switch( measurement->type ) {
	case MXM_PRESET_COUNT:
		preset_count_struct = (MX_MEASUREMENT_PRESET_COUNT *)
					measurement->measurement_type_struct;

		*measurement_counts = preset_count_struct->preset_count;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Measurement counts were requested for a measurement that is not "
"a preset count measurement.  Instead, it was of "
"measurement type %ld.", measurement->type );
	}

	return MX_SUCCESSFUL_RESULT;
}

