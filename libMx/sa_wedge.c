/*
 * Name:    sa_wedge.c
 *
 * Purpose: Scan description file for area detector wedge scans.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_constants.h"
#include "mx_record.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_scan.h"
#include "mx_scan_area_detector.h"
#include "sa_wedge.h"

MX_AREA_DETECTOR_SCAN_FUNCTION_LIST
			mxs_wedge_area_detector_scan_function_list = {
	mxs_wedge_scan_create_record_structures,
	mxs_wedge_scan_finish_record_initialization,
	NULL,
	mxs_wedge_scan_execute_scan_body
};

MX_RECORD_FIELD_DEFAULTS mxs_wedge_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_AREA_DETECTOR_SCAN_STANDARD_FIELDS,
	MXS_WEDGE_SCAN_STANDARD_FIELDS
};

long mxs_wedge_scan_num_record_fields = sizeof( mxs_wedge_scan_defaults )
					/ sizeof( mxs_wedge_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_wedge_scan_def_ptr
			= &mxs_wedge_scan_defaults[0];

static mx_status_type
mxp_wedge_scan_take_frame( MX_SCAN *, MX_AREA_DETECTOR_SCAN *, MX_WEDGE_SCAN *);

MX_EXPORT mx_status_type
mxs_wedge_scan_create_record_structures( MX_RECORD *record,
					MX_SCAN *scan,
					MX_AREA_DETECTOR_SCAN *ad_scan )
{
	static const char fname[] = "mxs_wedge_scan_create_record_structures()";

	MX_WEDGE_SCAN *wedge_scan;

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, record->name));

	wedge_scan = (MX_WEDGE_SCAN *) malloc( sizeof(MX_WEDGE_SCAN) );

	if ( wedge_scan == (MX_WEDGE_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate an MX_WEDGE_SCAN structure." );
	}

	record->record_type_struct = wedge_scan;
	ad_scan->record_type_struct = wedge_scan;

	record->class_specific_function_list =
			&mxs_wedge_area_detector_scan_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_wedge_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_wedge_scan_finish_record_initialization()";

	MX_SCAN *scan;
	long i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, record->name));

	scan = (MX_SCAN *) record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for record '%s' is NULL.", record->name );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_wedge_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_wedge_scan_execute_scan_body()";

	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;
	MX_AREA_DETECTOR_SCAN *ad_scan;
	MX_WEDGE_SCAN *wedge_scan;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	unsigned long i, n, w, start_step, end_step;
	double motor_start, motor_step_size, motor_range, motor_end;
	unsigned long motor_num_steps, num_wedges, num_energies;
	double abs_range, abs_step_size;
	double raw_start_step, raw_end_step;
	double abs_wedge_size, wedge_size, wedges_per_run;
	double wedge_start, wedge_end, energy, position;
	double inverse_distance;
	size_t units_length;
	mx_bool_type use_inverse_beam;
	mx_status_type mx_status;

	mx_status_type (*initialize_datafile_naming_fn)( MX_SCAN *);
	mx_status_type (*construct_next_datafile_name_fn)( MX_SCAN *);

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	ad_scan = scan->record->record_class_struct;

	wedge_scan = scan->record->record_type_struct;

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));

	flist = scan->record->class_specific_function_list;

	/* Initialize the data structures for computing datafile names. */

	if ( flist->initialize_datafile_naming == NULL ) {
	    initialize_datafile_naming_fn =
	    	mxs_area_detector_scan_default_initialize_datafile_naming;
	} else {
	    initialize_datafile_naming_fn = flist->initialize_datafile_naming;
	}

	mx_status = (*initialize_datafile_naming_fn)( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find the function for constructing datafile names. */

	if ( flist->construct_next_datafile_name == NULL ) {
	    construct_next_datafile_name_fn =
	    	mxs_area_detector_scan_default_construct_next_datafile_name;
	} else {
	    construct_next_datafile_name_fn =
	    	flist->construct_next_datafile_name;
	}

#if 1
	for ( i = 0; i < scan->num_motors; i++ ) {
		MX_DEBUG(-2,
("%s: motor(%lu) = '%s', start = %f, step size = %f, # measurements = %ld",
			fname, i, scan->motor_record_array[i]->name,
			ad_scan->start_position[i],
			ad_scan->step_size[i],
			ad_scan->num_frames[i]));
	}
	MX_DEBUG(-2,("%s: wedge size = %f",
		fname, wedge_scan->wedge_size));
	MX_DEBUG(-2,("%s: energy motor = '%s'",
			fname, ad_scan->energy_record->name));

	for ( i = 0; i < ad_scan->num_energies; i++ ) {
		MX_DEBUG(-2,("%s: energy #%lu = %f",
			fname, i, ad_scan->energy_array[i]));
	}
	for ( i = 0; i < ad_scan->num_static_motors; i++ ) {
		MX_DEBUG(-2,("%s: motor(%lu) = '%s'",
			fname, i, ad_scan->static_motor_array[i]->name));
	}
#endif
	/* FIXME? From here on, we assume that num_motors = 1. */

	motor_record = scan->motor_record_array[0];

	motor = motor_record->record_class_struct;

	use_inverse_beam = ad_scan->use_inverse_beam;

	MX_DEBUG(-2,("%s: use_inverse_beam = %d",
			fname, (int) use_inverse_beam));

	units_length = strlen( motor->units );

	if (mx_strncasecmp( motor->units, "radians", units_length ) == 0) {
		inverse_distance = MX_PI;
	} else
	if (mx_strncasecmp( motor->units, "milliradians", units_length ) == 0) {
		inverse_distance = 1000.0 * MX_PI;
	} else
	if (mx_strncasecmp( motor->units, "microradians", units_length ) == 0) {
		inverse_distance = 1000000.0 * MX_PI;
	} else
	if (mx_strncasecmp( motor->units, "degrees", units_length ) == 0) {
		inverse_distance = 180;
	} else
	if (mx_strncasecmp( motor->units, "arcminutes", units_length ) == 0) {
		inverse_distance = 60.0 * 180;
	} else
	if (mx_strncasecmp( motor->units, "arcseconds", units_length ) == 0) {
		inverse_distance = 3600.0 * 180;
	} else {
		inverse_distance = 180.0;
	}

	motor_start     = ad_scan->start_position[0];
	motor_step_size = ad_scan->step_size[0];
	motor_num_steps = ad_scan->num_frames[0];
	motor_range     = motor_step_size * ( motor_num_steps - 1 );
	motor_end       = motor_start + motor_range;

	wedge_size = wedge_scan->wedge_size;

	abs_range      = fabs(motor_range);
	abs_step_size  = fabs(motor_step_size);
	abs_wedge_size = fabs(wedge_size);

	if ( motor_num_steps <= 1 ) {
		num_wedges = 1;
	} else {
		if ( abs_wedge_size < 1.0e-12 ) {
			/* If the wedge size is very close to 0, then
			 * we assume that the user really only wants
			 * one wedge.
			 */

			num_wedges = 1;
		} else {
			wedges_per_run = mx_divide_safely( abs_range,
							abs_wedge_size );

			num_wedges = mx_round_up( wedges_per_run );
		}
	}

	num_energies = ad_scan->num_energies;

	MX_DEBUG(-2,("%s: num_wedges = %lu, num_energies = %lu",
		fname, num_wedges, num_energies));

	for ( w = 0; w < num_wedges; w++ ) {
		wedge_scan->current_wedge_number = w;

		wedge_start = motor_start + w * wedge_size;
		wedge_end   = motor_start + (w+1) * wedge_size;

		MX_DEBUG(-2,("%s: w = %lu, wedge_start = %f, wedge_end = %f",
			fname, w, wedge_start, wedge_end));

		raw_start_step = mx_divide_safely( w * abs_wedge_size,
							abs_step_size );

		start_step = mx_round( raw_start_step );

		MX_DEBUG(-2,("%s: raw_start_step = %f, start_step = %lu",
			fname, raw_start_step, start_step));

		raw_end_step = mx_divide_safely( (w+1) * abs_wedge_size,
							abs_step_size );

		end_step = mx_round( raw_end_step );

		MX_DEBUG(-2,("%s: raw_end_step = %f, end_step = %lu",
			fname, raw_end_step, end_step));

		if ( end_step > motor_num_steps ) {
			end_step = motor_num_steps;

			MX_DEBUG(-2,("%s: truncating end_step to %lu",
					fname, end_step ));
		}

		for ( n = 0; n < num_energies; n++ ) {
			ad_scan->current_energy_number = n;

			energy = ad_scan->energy_array[n];

			for ( i = start_step; i < end_step; i++ ) {
				ad_scan->current_frame_number = i;

				position = motor_start + i * motor_step_size;

				MX_DEBUG(-2,("%s: energy = %f, position = %f",
					fname, energy, position));

				/* Move to the new position. */

				mx_status = mx_motor_move_absolute(
						motor_record, position, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status = mx_wait_for_motor_stop(
						motor_record, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Construct the name of the next image frame.*/

				mx_status =
				    (*construct_next_datafile_name_fn)( scan );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Perform the measurement. */

				mx_status = mxp_wedge_scan_take_frame( 
						scan, ad_scan, wedge_scan );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			/* Do we do inverse beam measurements? */

			if ( use_inverse_beam == FALSE ) {
				continue;
			}

			/* Yes, we do use inverse beam. */

			for ( i = start_step; i < end_step; i++ ) {
				ad_scan->current_frame_number = i + 180;

				position = motor_start + inverse_distance
						+ i * motor_step_size;

				MX_DEBUG(-2,("%s: energy = %f, position = %f",
					fname, energy, position));

				/* Move to the new position. */

				mx_status = mx_motor_move_absolute(
						motor_record, position, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status = mx_wait_for_motor_stop(
						motor_record, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Construct the name of the next image frame.*/

				mx_status =
				    (*construct_next_datafile_name_fn)( scan );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				/* Perform the measurement. */

				mx_status = mxp_wedge_scan_take_frame( 
						scan, ad_scan, wedge_scan );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxp_wedge_scan_take_frame( MX_SCAN *scan,
			MX_AREA_DETECTOR_SCAN *ad_scan,
			MX_WEDGE_SCAN *wedge_scan )
{
	static const char fname[] = "mxp_wedge_scan_take_frame()";

	MX_RECORD *ad_record;
	MX_AREA_DETECTOR *ad;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_RECORD *shutter_record;
	double delta, exposure_time;
	unsigned long ad_status;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for scan '%s'", fname, scan->record->name));

	ad_record = scan->input_device_array[0];

	ad = ad_record->record_class_struct;

	shutter_record = scan->input_device_array[1];

	delta = ad_scan->step_size[0];

	mx_status = mx_get_measurement_time( &(scan->measurement),
						&exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor_record = scan->motor_record_array[0];

	motor = motor_record->record_class_struct;

	MX_DEBUG(-2,("%s: Starting exposure of '%s' for %f seconds "
	"using shutter '%s' with an oscillation of '%s' for %f %s",
		fname, ad_record->name, exposure_time,
		shutter_record->name, motor_record->name,
		delta, motor->units ));

	/* FIXME - Move this call to mx_area_detector_setup_exposure()
	 * to somewhere else, so that we are not doing it for each frame.
	 */

	mx_status = mx_area_detector_setup_exposure( ad_record,
						motor_record,
						shutter_record,
						delta,
						exposure_time );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Trigger the exposure. */

	mx_status = mx_area_detector_trigger_exposure( ad_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the exposure to finish. */

	while(1) {
		mx_status = mx_area_detector_get_status( ad_record,
							&ad_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( ad_status & MXSF_AD_IS_BUSY ) == 0 ) {
			break;		/* Exit the for() loop. */
		}

		mx_msleep(1);
	}

	mx_status = mx_add_measurement_to_datafile( &(scan->datafile) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: frame complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

