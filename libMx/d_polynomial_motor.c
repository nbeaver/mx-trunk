/*
 * Name:    d_polynomial_motor.c 
 *
 * Purpose: MX pseudomotor driver that uses a polynomial to compute the
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
#include "d_polynomial_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_polynomial_motor_record_function_list = {
	NULL,
	mxd_polynomial_motor_create_record_structures,
	mxd_polynomial_motor_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_polynomial_motor_open
};

MX_MOTOR_FUNCTION_LIST mxd_polynomial_motor_motor_function_list = {
	NULL,
	mxd_polynomial_motor_move_absolute,
	NULL,
	NULL,
	mxd_polynomial_motor_soft_abort,
	mxd_polynomial_motor_immediate_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_polynomial_motor_get_parameter,
	mxd_polynomial_motor_set_parameter,
	NULL,
	mxd_polynomial_motor_get_status,
	mxd_polynomial_motor_get_extended_status
};

/* Polynomial motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_polynomial_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_POLYNOMIAL_MOTOR_STANDARD_FIELDS
};

long mxd_polynomial_motor_num_record_fields
		= sizeof( mxd_polynomial_motor_record_field_defaults )
		    / sizeof( mxd_polynomial_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_polynomial_motor_rfield_def_ptr
			= &mxd_polynomial_motor_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_polynomial_motor_get_pointers( MX_MOTOR *motor,
			MX_POLYNOMIAL_MOTOR **polynomial_motor,
			MX_RECORD **dependent_motor_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_polynomial_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( polynomial_motor == (MX_POLYNOMIAL_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The polynomial_motor pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*polynomial_motor = (MX_POLYNOMIAL_MOTOR *)
					motor->record->record_type_struct;

	if ( *polynomial_motor == (MX_POLYNOMIAL_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POLYNOMIAL_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( dependent_motor_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The dependent_motor_record pointer passed by '%s' was NULL.",
			motor->record->name );
	}

	*dependent_motor_record = (*polynomial_motor)->dependent_motor_record;

	if ( *dependent_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Dependent motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}


	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_polynomial_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_polynomial_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_POLYNOMIAL_MOTOR *polynomial_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	polynomial_motor = (MX_POLYNOMIAL_MOTOR *)
				malloc( sizeof(MX_POLYNOMIAL_MOTOR) );

	if ( polynomial_motor == (MX_POLYNOMIAL_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_POLYNOMIAL_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = polynomial_motor;
	record->class_specific_function_list
				= &mxd_polynomial_motor_motor_function_list;

	motor->record = record;

	/* A polynomial motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_polynomial_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_polynomial_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_POLYNOMIAL_MOTOR *polynomial_motor;

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

	polynomial_motor = (MX_POLYNOMIAL_MOTOR *)
					record->record_type_struct;

	if ( polynomial_motor == (MX_POLYNOMIAL_MOTOR *)NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POLYNOMIAL_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->real_motor_record = polynomial_motor->dependent_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_polynomial_motor_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_polynomial_motor_open()";

	MX_MOTOR *motor;
	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	MX_RECORD *polynomial_record;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
					&dependent_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that polynomial_record is a variable record
	 * of type MXFT_DOUBLE.
	 */

	polynomial_record = polynomial_motor->polynomial_record;

	if ( polynomial_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The polynomial_record pointer for record '%s' is NULL.",
			record->name );
	}

	if ( polynomial_record->mx_superclass != MXR_VARIABLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' used by record '%s' is not a variable record.",
			polynomial_record->name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_polynomial_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_polynomial_motor_move_absolute()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record, *polynomial_record;
	MX_RECORD_FIELD *value_field;
	long i, num_parameters;
	double *poly;
	double x, y;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
					&dependent_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the current pseudomotor position to the requested destination. */

	motor->raw_position.analog = motor->raw_destination.analog;

	motor->position =
		motor->offset + motor->scale * motor->raw_position.analog;

	/* Find the 'value' field of the polynomial. */

	polynomial_record = polynomial_motor->polynomial_record;

	mx_status = mx_find_record_field( polynomial_record,
						"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( value_field->datatype != MXFT_DOUBLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Polynomial variable '%s' used by polynomial "
			"motor '%s' is not of type MXFT_DOUBLE.",
				polynomial_record->name,
				motor->record->name );
	}
	if ( value_field->num_dimensions != 1 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"Polynomial variable '%s' used by polynomial "
			"motor '%s' is not a 1-dimensional array.  "
			"Instead, it has %ld dimensions.",
				polynomial_record->name,
				motor->record->name,
				value_field->num_dimensions );
	}

	/* Get a pointer to the polynomial itself. */

	poly = mx_get_field_value_pointer( value_field );

	if ( poly == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Polynomial variable '%s' used by polynomial motor '%s' "
		"somehow has a value pointer that is NULL.",
			polynomial_record->name, motor->record->name );
	}

	/* Get the number of parameters in the polynomial. */

	num_parameters = value_field->dimension[0];

	if ( num_parameters == 0 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Polynomial variable '%s' used by polynomial motor '%s' "
		"somehow has a length of 0.",
			polynomial_record->name, motor->record->name );
	}

	/* Compute the polynomial value and move to that position. */

	x = motor->raw_destination.analog;

	y = poly[ num_parameters - 1 ];

	for ( i = num_parameters - 2; i >= 0; i-- ) {
		y = poly[i] + x * y;
	}

	mx_status = mx_motor_move_absolute( dependent_motor_record, y, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_polynomial_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_polynomial_motor_soft_abort()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
					&dependent_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( dependent_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_polynomial_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_polynomial_motor_immediate_abort()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
					&dependent_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( dependent_motor_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_polynomial_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_polynomial_motor_get_parameter()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
					&dependent_motor_record, fname );

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
"Polynomial pseudomotor '%s' cannot report the value of parameter '%s' (%ld).",
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
mxd_polynomial_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_polynomial_motor_set_parameter()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
					&dependent_motor_record,fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_BASE_SPEED:
	case MXLV_MTR_MAXIMUM_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Polynomial pseudomotor '%s' cannot set the value of parameter '%s' (%ld).",
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
mxd_polynomial_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_polynomial_motor_get_status()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	unsigned long motor_status;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
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
mxd_polynomial_motor_get_extended_status( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_polynomial_motor_get_extended_status()";

	MX_POLYNOMIAL_MOTOR *polynomial_motor;
	MX_RECORD *dependent_motor_record;
	unsigned long motor_status;
	double theta;
	mx_status_type mx_status;

	mx_status = mxd_polynomial_motor_get_pointers( motor,
					&polynomial_motor,
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

