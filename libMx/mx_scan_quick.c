/*
 * Name:    mx_scan_quick.c
 *
 * Purpose: Driver for MX_QUICK_SCAN scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_constants.h"
#include "mx_variable.h"
#include "mx_scan.h"
#include "mx_scan_quick.h"
#include "d_energy.h"

MX_EXPORT mx_status_type
mx_quick_scan_initialize_type( long record_type )
{
	static const char fname[] = "mx_quick_scan_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	mx_length_type num_record_fields;
	mx_length_type num_independent_variables_varargs_cookie;
	mx_length_type num_motors_varargs_cookie;
	mx_length_type num_input_devices_varargs_cookie;
	mx_status_type status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr = driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	record_field_defaults = *record_field_defaults_ptr;

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (mx_length_type *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	/**** Fix up the record fields common to all scan types. ****/

	num_record_fields = *(driver->num_record_fields);

	status = mx_scan_fixup_varargs_record_field_defaults(
			record_field_defaults, num_record_fields,
			&num_independent_variables_varargs_cookie,
			&num_motors_varargs_cookie,
			&num_input_devices_varargs_cookie );

	if ( status.code != MXE_SUCCESS )
		return status;

	/** Fix up the record fields specific to MX_XAFS_SCAN records **/

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"old_motor_speed", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_motors_varargs_cookie;

	/*--*/

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"start_position", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_motors_varargs_cookie;

	/*--*/

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"end_position", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_motors_varargs_cookie;

	/*--*/

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"saved_synchronous_motion_mode", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_motors_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_quick_scan_print_scan_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mx_quick_scan_print_scan_structure()";

	static const char fake_motor_units[] = "???";
	MX_SCAN *scan;
	MX_QUICK_SCAN *quick_scan;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	long i, j;
	double scan_distance, step_size;
	const char *motor_units;
	mx_status_type status;

	if ( record != NULL ) {
		fprintf( file, "SCAN parameters for quick scan '%s':\n",
					record->name );
	}

	status = mx_scan_print_scan_structure( file, record );

	if ( status.code != MXE_SUCCESS )
		return status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	quick_scan = (MX_QUICK_SCAN *) record->record_class_struct;

	if ( quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_QUICK_SCAN pointer for record '%s' is NULL.", record->name );
	}

	fprintf( file, "  Scan range:\n" );

	j = 0;

	for ( i = 0; i < scan->num_motors; i++ ) {
		if ( scan->motor_is_independent_variable[i] == TRUE ) {
			motor_record = scan->motor_record_array[i];

			motor = (MX_MOTOR *) motor_record->record_class_struct;

			if ( motor_record->mx_superclass == MXR_PLACEHOLDER ) {
				motor_units = fake_motor_units;
			} else {
				motor_units = motor->units;
			}

			scan_distance =
		quick_scan->end_position[j] - quick_scan->start_position[j],

			step_size = mx_divide_safely( scan_distance,
				quick_scan->requested_num_measurements - 1L );

			fprintf( file,
"    %s: %g %s to %g %s, %g %s steps, %ld req meas, %ld act meas\n",
				scan->motor_record_array[i]->name,
				quick_scan->start_position[j], motor_units,
				quick_scan->end_position[i], motor_units,
				step_size, motor_units,
				(long) quick_scan->requested_num_measurements,
				(long) quick_scan->actual_num_measurements );

			j++;
		}
	}
	fprintf( file, "\n" );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_quick_scan_get_theta( double d_spacing, double energy,
				double angle_scale, double *theta )
{
	static const char fname[] = "mx_quick_scan_get_theta()";

	double sin_theta, angle_in_radians;

	sin_theta = mx_divide_safely( MX_HC, 2.0 * d_spacing * energy );

	if ( sin_theta <= 1.0 ) {
		angle_in_radians = asin( sin_theta );
	} else {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Attempted to go to an unreachable energy of %g eV.  "
	"Minimum allowed energy for d spacing = %g angstroms is %g eV.",
			energy, d_spacing, MX_HC / ( 2.0 * d_spacing ) );
	}

	*theta = mx_divide_safely( angle_in_radians , angle_scale );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_quick_scan_set_new_motor_speed( MX_QUICK_SCAN *quick_scan, long motor_index )
{
	static const char fname[] = "mx_quick_scan_set_new_motor_speed()";

	MX_SCAN *scan;
	MX_RECORD *independent_variable_record, *motor_record_to_set;
	MX_MOTOR *motor_to_set;
	MX_ENERGY_MOTOR *energy_motor;
	double start_position, end_position;
	double old_speed, new_speed, measurement_time;
	double d_spacing;
	mx_length_type num_elements;
	void *pointer_to_value;
	mx_status_type status;

	if ( quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_QUICK_SCAN pointer passed was NULL." );
	}

	scan = quick_scan->scan;

	if ( motor_index < 0 || motor_index >= scan->num_motors ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The motor index %ld passed is outside the allowed range of (0-%ld).",
			motor_index, (long) scan->num_motors );
	}

	independent_variable_record = (scan->motor_record_array)[motor_index];

	switch( independent_variable_record->mx_type ) {
	case MXT_MTR_ENERGY:
		energy_motor = (MX_ENERGY_MOTOR *)
				independent_variable_record->record_type_struct;

		motor_record_to_set = energy_motor->dependent_motor_record;

		/* Get d spacing for the monochromator. */

		status = mx_get_1d_array(
				energy_motor->d_spacing_record,
				MXFT_DOUBLE, &num_elements, &pointer_to_value );

		if ( status.code != MXE_SUCCESS )
			return status;

		d_spacing = *((double *) pointer_to_value);

		/* Get start position in theta. */

		status = mx_quick_scan_get_theta( d_spacing,
					quick_scan->start_position[motor_index],
					energy_motor->angle_scale,
					&start_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Get end position in theta. */

		status = mx_quick_scan_get_theta( d_spacing,
					quick_scan->end_position[motor_index],
					energy_motor->angle_scale,
					&end_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		break;
	default:
		motor_record_to_set = independent_variable_record;

		start_position = quick_scan->start_position[motor_index];
		end_position = quick_scan->end_position[motor_index];
		break;
	}

	/* Save the old speed. */

	status = mx_motor_get_speed( motor_record_to_set, &old_speed );

	if ( status.code != MXE_SUCCESS )
		return status;

	quick_scan->old_motor_speed[motor_index] = old_speed;

	/* Set the new speed. */

	measurement_time = mx_quick_scan_get_measurement_time( quick_scan );

	new_speed = mx_divide_safely( end_position - start_position,
				measurement_time *
				(quick_scan->requested_num_measurements - 1));

	new_speed = fabs(new_speed);

	motor_to_set = (MX_MOTOR *) motor_record_to_set->record_class_struct;

	mx_info("Setting motor speed for '%s' to %g %s/sec.",
			motor_record_to_set->name,
			new_speed, motor_to_set->units);

	status = mx_motor_set_speed( motor_record_to_set, new_speed );

	return status;
}

MX_EXPORT mx_status_type
mx_quick_scan_restore_old_motor_speed( MX_QUICK_SCAN *quick_scan,
					long motor_index )
{
	static const char fname[] = "mx_quick_scan_restore_old_motor_speed()";

	MX_SCAN *scan;
	MX_RECORD *independent_variable_record, *motor_record_to_restore;
	MX_MOTOR *motor_to_restore;
	MX_ENERGY_MOTOR *energy_motor;
	double old_speed;
	mx_status_type status;

	if ( quick_scan == (MX_QUICK_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_QUICK_SCAN pointer passed was NULL." );
	}

	scan = quick_scan->scan;

	if ( motor_index < 0 || motor_index >= scan->num_motors ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The motor index %ld passed is outside the allowed range of (0-%ld).",
			motor_index, (long) scan->num_motors );
	}

	independent_variable_record = (scan->motor_record_array)[motor_index];

	switch( independent_variable_record->mx_type ) {
	case MXT_MTR_ENERGY:
		energy_motor = (MX_ENERGY_MOTOR *)
				independent_variable_record->record_type_struct;

		motor_record_to_restore = energy_motor->dependent_motor_record;
		break;
	default:
		motor_record_to_restore = independent_variable_record;
		break;
	}

	/* Restoring the old speed. */

	old_speed = quick_scan->old_motor_speed[motor_index];

	motor_to_restore = (MX_MOTOR *)
			motor_record_to_restore->record_class_struct;

	mx_info("Restoring motor speed of '%s' to %g %s/sec.",
			motor_record_to_restore->name,
			old_speed, motor_to_restore->units);

	status = mx_motor_set_speed( motor_record_to_restore, old_speed );

	return status;
}

MX_EXPORT double
mx_quick_scan_get_measurement_time( MX_QUICK_SCAN *quick_scan )
{
	static const char fname[] = "mx_quick_scan_get_measurement_time()";

	double measurement_time;
	int num_items;

	if ( quick_scan == (MX_QUICK_SCAN *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_QUICK_SCAN pointer passed was NULL." );

		return 0.0;
	}
	if ( quick_scan->scan == (MX_SCAN *) NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_SCAN pointer for the MX_QUICK_SCAN pointer passed is NULL.");

		return 0.0;
	}

	num_items = sscanf( quick_scan->scan->measurement_arguments, "%lg",
					&measurement_time );

	if ( num_items != 1 ) {
		(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The measurement time cannot be found in the "
			"measurement_arguments field '%s' for scan '%s'.",
			quick_scan->scan->measurement_arguments,
			quick_scan->scan->record->name );

		return 0.0;
	}

	return measurement_time;
}

