/*
 * Name:    d_cubic_spline_motor.c 
 *
 * Purpose: MX pseudomotor driver that uses a cubic spline to compute the
 *          position of a dependent motor.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_cubic_spline_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_cubic_spline_motor_record_function_list = {
	NULL,
	mxd_cubic_spline_motor_create_record_structures,
	mxd_cubic_spline_motor_finish_record_initialization,
	mxd_cubic_spline_motor_delete_record,
	NULL,
	NULL,
	NULL,
	mxd_cubic_spline_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_cubic_spline_motor_motor_function_list = {
	NULL,
	mxd_cubic_spline_motor_move_absolute,
	NULL,
	NULL,
	mxd_cubic_spline_motor_soft_abort,
	mxd_cubic_spline_motor_immediate_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_cubic_spline_motor_get_parameter,
	mxd_cubic_spline_motor_set_parameter,
	NULL,
	mxd_cubic_spline_motor_get_status,
	mxd_cubic_spline_motor_get_extended_status
};

/* Cubic spline motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_cubic_spline_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_CUBIC_SPLINE_MOTOR_STANDARD_FIELDS
};

long mxd_cubic_spline_motor_num_record_fields
		= sizeof( mxd_cubic_spline_motor_record_field_defaults )
		    / sizeof( mxd_cubic_spline_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_cubic_spline_motor_rfield_def_ptr
			= &mxd_cubic_spline_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_cubic_spline_motor_get_pointers( MX_MOTOR *motor,
			MX_CUBIC_SPLINE_MOTOR **cubic_spline_motor,
			MX_RECORD **dependent_motor_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_cubic_spline_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( cubic_spline_motor == (MX_CUBIC_SPLINE_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The cubic_spline_motor pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*cubic_spline_motor = (MX_CUBIC_SPLINE_MOTOR *)
					motor->record->record_type_struct;

	if ( *cubic_spline_motor == (MX_CUBIC_SPLINE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CUBIC_SPLINE_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( dependent_motor_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dependent_motor_record pointer passed by '%s' was NULL.",
			motor->record->name );
	}

	*dependent_motor_record = (*cubic_spline_motor)->dependent_motor_record;

	if ( *dependent_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Dependent motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}


	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_cubic_spline_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	cubic_spline_motor = (MX_CUBIC_SPLINE_MOTOR *)
				malloc( sizeof(MX_CUBIC_SPLINE_MOTOR) );

	if ( cubic_spline_motor == (MX_CUBIC_SPLINE_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_CUBIC_SPLINE_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = cubic_spline_motor;
	record->class_specific_function_list
				= &mxd_cubic_spline_motor_motor_function_list;

	motor->record = record;

	/* A cubic spline motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_cubic_spline_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *)NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	cubic_spline_motor = (MX_CUBIC_SPLINE_MOTOR *)
					record->record_type_struct;

	if ( cubic_spline_motor == (MX_CUBIC_SPLINE_MOTOR *)NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CUBIC_SPLINE_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->real_motor_record = cubic_spline_motor->dependent_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxd_cubic_spline_motor_delete_record()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	cubic_spline_motor = (MX_CUBIC_SPLINE_MOTOR *)
					record->record_type_struct;

	if ( cubic_spline_motor != (MX_CUBIC_SPLINE_MOTOR *) NULL ) {
		mx_delete_cubic_spline( cubic_spline_motor->spline );

		mx_free( cubic_spline_motor );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_cubic_spline_motor_open()";

	MX_MOTOR *motor;
	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	MX_RECORD *x_array_record;
	MX_RECORD *y_array_record;
	MX_RECORD_FIELD *x_value_field;
	MX_RECORD_FIELD *y_value_field;
	long x_num_points, y_num_points;
	double *x_value_ptr, *y_value_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that x_array_record is a variable record
	 * of type MXFT_DOUBLE.
	 */

	x_array_record = cubic_spline_motor->x_array_record;

	if ( x_array_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The x_array_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( x_array_record->mx_superclass != MXR_VARIABLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by record '%s' is not a variable record.",
			x_array_record->name, record->name );
	}

	mx_status = mx_find_record_field( x_array_record,
					"value", &x_value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_value_field->datatype != MXFT_DOUBLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Variable record '%s' used by record '%s' is not "
		"of type MXFT_DOUBLE.", x_array_record->name, record->name );
	}

	if ( x_value_field->num_dimensions != 1 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Variable record '%s' used by record '%s' is not "
		"a 1-dimensional variable.",
		x_array_record->name, record->name );
	}

	x_num_points = x_value_field->dimension[0];

	x_value_ptr = mx_get_field_value_pointer( x_value_field );

	/* Verify that y_array_record is a variable record
	 * of type MXFT_DOUBLE.
	 */

	y_array_record = cubic_spline_motor->y_array_record;

	if ( y_array_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The y_array_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( y_array_record->mx_superclass != MXR_VARIABLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by record '%s' is not a variable record.",
			y_array_record->name, record->name );
	}

	mx_status = mx_find_record_field( y_array_record,
					"value", &y_value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( y_value_field->datatype != MXFT_DOUBLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Variable record '%s' used by record '%s' is not "
		"of type MXFT_DOUBLE.", y_array_record->name, record->name );
	}

	if ( x_value_field->num_dimensions != 1 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Variable record '%s' used by record '%s' is not "
		"a 1-dimensional variable.",
		y_array_record->name, record->name );
	}

	y_num_points = y_value_field->dimension[0];

	y_value_ptr = mx_get_field_value_pointer( y_value_field );

	/* See if the two arrays have the same number of points. */

	if ( x_num_points != y_num_points ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of points (%ld) in X variable record '%s' "
		"is not the same as the number of points (%ld) in "
		"Y variable record '%s' used by cubic spline record '%s'.",
			x_num_points, x_array_record->name,
			y_num_points, y_array_record->name,
			record->name );
	}

	/* The X and Y arrays must have at least two points. */

	if ( x_num_points < 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of points (%ld) in X array '%s' and Y array '%s' "
		"used by record '%s' is less than the minimum of 2 points.",
		    x_num_points, x_array_record->name, y_array_record->name,
		    record->name );
	}

	/* Initialize the cubic spline using the current X and Y arrays. */

	mx_status = mx_create_clamped_cubic_spline( x_num_points,
						x_value_ptr, y_value_ptr,
						0.0, 0.0,
						&(cubic_spline_motor->spline) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cubic_spline_motor_move_absolute()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_CUBIC_SPLINE *spline;
	MX_RECORD *dependent_motor_record;
	MX_RECORD_FIELD *x_array_field, *y_array_field;
	mx_bool_type recompute_cubic_spline;
	double *x_array, *y_array;
	double spline_x, spline_y;
	double xv, xs, yv, ys;
	unsigned long i, num_points;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do we need to recompute the cubic spline? */

	spline = cubic_spline_motor->spline;

	if ( spline == (MX_CUBIC_SPLINE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Cubic spline pseudomotor '%s' has not initialize "
		"the cubic spline data structure.", motor->record->name );
	}

	mx_status = mx_find_record_field( cubic_spline_motor->x_array_record,
						"value", &x_array_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	x_array = mx_get_field_value_pointer( x_array_field );
	
	mx_status = mx_find_record_field( cubic_spline_motor->y_array_record,
						"value", &y_array_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	y_array = mx_get_field_value_pointer( y_array_field );

	recompute_cubic_spline = FALSE;

	/* See if the X array has changed. */

	num_points = spline->num_points;

	for ( i = 0; i < num_points; i++ ) {
		xv = x_array[i];
		xs = spline->x_array[i];

		if ( mx_difference( xv, xs ) >= 0.001 ) {
			recompute_cubic_spline = TRUE;
			break;
		}
	}

	if ( recompute_cubic_spline == FALSE ) {
		/* See if the Y array has changed. */

		for ( i = 0; i < num_points; i++ ) {
			yv = y_array[i];
			ys = spline->y_array[i];

			if ( mx_difference( yv, ys ) >= 0.001 ) {
				recompute_cubic_spline = TRUE;
				break;
			}
		}
	}

	if ( recompute_cubic_spline ) {
		mx_delete_cubic_spline( spline );

		mx_status = mx_create_clamped_cubic_spline( num_points,
						x_array, y_array,
						0.0, 0.0,
						&(cubic_spline_motor->spline) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	spline = cubic_spline_motor->spline;

	/* Compute the spline Y value from the spline X value
	 * and move to that position.
	 */

	spline_x = motor->raw_destination.analog;

	spline_y = mx_get_cubic_spline_value( spline, spline_x );

	mx_status = mx_motor_move_absolute( dependent_motor_record,
						spline_y, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cubic_spline_motor_soft_abort()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( dependent_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cubic_spline_motor_immediate_abort()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( dependent_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cubic_spline_motor_get_parameter()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_ACCELERATION_TYPE:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
	case MXLV_MTR_ACCELERATION_DISTANCE:
		return mx_error( MXE_UNSUPPORTED, fname,
"Cubic spline pseudomotor '%s' cannot report the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
					dependent_motor_record,
					&(motor->acceleration_time) );
		break;

	default:
		mx_status = mx_motor_default_get_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cubic_spline_motor_set_parameter()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Cubic spline pseudomotor '%s' cannot set the value of parameter '%s' (%ld).",
			motor->record->name,
			mx_get_field_label_string( motor->record,
						motor->parameter_type ),
			motor->parameter_type );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed( dependent_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed( dependent_motor_record );
		break;

	default:
		mx_status = mx_motor_default_set_parameter_handler( motor );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_cubic_spline_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_cubic_spline_motor_get_status()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	unsigned long motor_status;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status(dependent_motor_record, &motor_status);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reverse the direction of the motor limit bits if needed. */

	motor->status = motor_status;

	if ( motor->scale >= 0.0 ) {
		if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_NEGATIVE_LIMIT_HIT );
		}

		if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_POSITIVE_LIMIT_HIT );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_cubic_spline_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_cubic_spline_motor_get_extended_status()";

	MX_CUBIC_SPLINE_MOTOR *cubic_spline_motor;
	MX_RECORD *dependent_motor_record;
	unsigned long motor_status;
	double theta;
	mx_status_type mx_status;

	mx_status = mxd_cubic_spline_motor_get_pointers( motor,
					&cubic_spline_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_extended_status( dependent_motor_record,
						&theta, &motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reverse the direction of the motor limit bits if needed. */

	motor->status = motor_status;

	if ( motor->scale >= 0.0 ) {
		if ( motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_NEGATIVE_LIMIT_HIT );
		}

		if ( motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) {
			motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
		} else {
			motor->status &= ( ~ MXSF_MTR_POSITIVE_LIMIT_HIT );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

