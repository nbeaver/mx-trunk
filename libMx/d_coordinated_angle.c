/*
 * Name:    d_coordinated_angle.c
 *
 * Purpose: MX driver that coordinates the moves of several motors to
 *          maintain them all at the same relative angle.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_COORDINATED_ANGLE_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_motor.h"
#include "d_coordinated_angle.h"

MX_RECORD_FUNCTION_LIST mxd_coordinated_angle_record_function_list = {
	mxd_coordinated_angle_initialize_driver,
	mxd_coordinated_angle_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_coordinated_angle_open,
	NULL,
	NULL,
	NULL,
	mxd_coordinated_angle_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_coordinated_angle_motor_function_list = {
	NULL,
	mxd_coordinated_angle_move_absolute,
	mxd_coordinated_angle_get_position,
	NULL,
	mxd_coordinated_angle_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_coordinated_angle_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_coordinated_angle_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_COORDINATED_ANGLE_STANDARD_FIELDS
};

long mxd_coordinated_angle_num_record_fields =
		sizeof( mxd_coordinated_angle_record_field_defaults )
		/ sizeof( mxd_coordinated_angle_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_coordinated_angle_rfield_def_ptr
			= &mxd_coordinated_angle_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_coordinated_angle_get_pointers( MX_MOTOR *motor,
			MX_COORDINATED_ANGLE **coordinated_angle,
			const char *calling_fname )
{
	static const char fname[] = "mxd_coordinated_angle_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( coordinated_angle == (MX_COORDINATED_ANGLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "The MX_COORDINATED_ANGLE pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_MOTOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	*coordinated_angle = (MX_COORDINATED_ANGLE *)
				motor->record->record_type_struct;

	if ( *coordinated_angle == (MX_COORDINATED_ANGLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_COORDINATED_ANGLE pointer for motor "
			"record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

static mx_status_type
mxd_coordinated_angle_process_function( void *record_ptr,
				void *record_field_ptr,
				int operation )
{
	MX_RECORD *record;
	MX_RECORD_FIELD *field;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	field = (MX_RECORD_FIELD *) record_field_ptr;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( field->label_value ) {
		case MXLV_COORDINATED_ANGLE_MOTOR_ERROR_ARRAY:
		case MXLV_COORDINATED_ANGLE_STANDARD_DEVIATION:
			mx_status = mx_motor_get_position( record, NULL );
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_coordinated_angle_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] =
		"mxd_coordinated_angle_initialize_driver()";

	static char field_name[][MXU_FIELD_NAME_LENGTH+1] = {
		"motor_record_array",
		"longitudinal_distance_array",
		"motor_scale_array",
		"motor_offset_array",
		"raw_angle_array",
		"motor_angle_array",
		"motor_error_array",
	};

	static int num_fields = sizeof(field_name) / sizeof(field_name[0]);

	MX_RECORD_FIELD_DEFAULTS *field;
	int i;
	long referenced_field_index;
	long num_motors_varargs_cookie;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_motors",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
			referenced_field_index, 0, &num_motors_varargs_cookie);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < num_fields; i++ ) {
		mx_status = mx_find_record_field_defaults( driver,
						field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_motors_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_coordinated_angle_create_record_structures()";

	MX_MOTOR *motor;
	MX_COORDINATED_ANGLE *coordinated_angle;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	coordinated_angle = (MX_COORDINATED_ANGLE *)
				malloc( sizeof(MX_COORDINATED_ANGLE) );

	if ( coordinated_angle == (MX_COORDINATED_ANGLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_COORDINATED_ANGLE structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = coordinated_angle;
	record->class_specific_function_list
				= &mxd_coordinated_angle_motor_function_list;

	motor->record = record;
	coordinated_angle->record = record;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	/* A BioCAT mirror positioner is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_coordinated_angle_open()";

	MX_MOTOR *motor;
	MX_COORDINATED_ANGLE *coordinated_angle;
	char *type_name;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_coordinated_angle_get_pointers( motor,
						&coordinated_angle, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	type_name = coordinated_angle->angle_type_name;

	if ( mx_strcasecmp( type_name, "arc" ) == 0 ) {
	    coordinated_angle->angle_type = MXT_COORDINATED_ANGLE_ARC;
	} else
	if ( mx_strcasecmp( type_name, "tangent_arm" ) == 0 ) {
	    coordinated_angle->angle_type = MXT_COORDINATED_ANGLE_TANGENT_ARM;
	} else
	if ( mx_strcasecmp( type_name, "sine_arm" ) == 0 ) {
	    coordinated_angle->angle_type = MXT_COORDINATED_ANGLE_SINE_ARM;
	} else {
	    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Unrecognized angle type '%s' specified for record '%s'.  "
	    "The allowed types are 'arc', 'tangent_arm', and 'sine_arm'.",
		type_name, record->name );
	}

	/* Force the driver to initialize all of the in-memory parameters. */

	mx_status = mx_motor_get_position( motor->record, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_coordinated_angle_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_COORDINATED_ANGLE_STANDARD_DEVIATION:
		case MXLV_COORDINATED_ANGLE_MOTOR_ERROR_ARRAY:
			record_field->process_function
				= mxd_coordinated_angle_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_coordinated_angle_move_absolute()";

	MX_COORDINATED_ANGLE *coordinated_angle;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	double raw_angle, longitudinal_distance, transverse_distance;
	double child_destination, child_scale, child_offset;
	mx_status_type mx_status;

	mx_status = mxd_coordinated_angle_get_pointers( motor,
						&coordinated_angle, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The raw angle _must_ be in radians. */

	raw_angle = motor->raw_destination.analog;

#if MXD_COORDINATED_ANGLE_DEBUG
	MX_DEBUG(-2,("%s: moving '%s' to %f",
			fname, motor->record->name, raw_angle));
#endif
	/*---*/

	num_motors = coordinated_angle->num_motors;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = coordinated_angle->motor_record_array[i];

		if ( child_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor record %ld for pseudomotor '%s' is NULL.",
				i, motor->record->name );
		}

		longitudinal_distance =
			coordinated_angle->longitudinal_distance_array[i];

		switch( coordinated_angle->angle_type ) {
		case MXT_COORDINATED_ANGLE_ARC:
			transverse_distance = raw_angle;
			break;
		case MXT_COORDINATED_ANGLE_TANGENT_ARM:
			transverse_distance =
				longitudinal_distance * tan( raw_angle );
			break;
		case MXT_COORDINATED_ANGLE_SINE_ARM:
			transverse_distance =
				longitudinal_distance * sin( raw_angle );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal angle type %lu requested for motor '%s'.",
				coordinated_angle->angle_type,
				motor->record->name );
			break;
		}

#if MXD_COORDINATED_ANGLE_DEBUG
		MX_DEBUG(-2,("%s: child '%s', transverse_distance = %f",
			fname, child_motor_record->name, transverse_distance));
#endif

		child_scale = coordinated_angle->motor_scale_array[i];

		child_offset = coordinated_angle->motor_offset_array[i];

#if MXD_COORDINATED_ANGLE_DEBUG
		MX_DEBUG(-2,
		("%s: child '%s', child_scale = %f, child_offset = %f",
			fname, child_motor_record->name,
			child_scale, child_offset));
#endif

		child_destination =
			child_scale * transverse_distance + child_offset;

#if MXD_COORDINATED_ANGLE_DEBUG
		MX_DEBUG(-2,("%s: child_destination = %f",
			fname, child_destination));
#endif

		mx_status = mx_motor_move_absolute( child_motor_record,
						child_destination, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_coordinated_angle_get_position()";

	MX_COORDINATED_ANGLE *coordinated_angle;
	MX_RECORD *child_record;
	double longitudinal_distance, child_position;
	double motor_scale, motor_offset;
	double transverse_distance, raw_angle;
	double tangent_of_angle, sine_of_angle;
	double raw_angle_error;
	double sum, sum_of_squares;
	double mean, sample_variance;
	unsigned long i;
	mx_status_type mx_status;

	mx_status = mxd_coordinated_angle_get_pointers( motor,
						&coordinated_angle, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We get the angles of the child motors, convert them to radians,
	 * and add them up to get the mean angle.
	 */

	sum = 0.0;

	for ( i = 0; i < coordinated_angle->num_motors; i++ ) {

		child_record = coordinated_angle->motor_record_array[i];

		longitudinal_distance =
			coordinated_angle->longitudinal_distance_array[i];

		mx_status = mx_motor_get_position( child_record,
						&child_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor_scale  = coordinated_angle->motor_scale_array[i];
		motor_offset = coordinated_angle->motor_offset_array[i];

		transverse_distance = mx_divide_safely(
					child_position - motor_offset,
						motor_scale );

		switch( coordinated_angle->angle_type ) {
		case MXT_COORDINATED_ANGLE_ARC:
			raw_angle = transverse_distance;
			break;

		case MXT_COORDINATED_ANGLE_TANGENT_ARM:
			tangent_of_angle = mx_divide_safely(transverse_distance,
							longitudinal_distance );

			raw_angle = atan( tangent_of_angle );
			break;

		case MXT_COORDINATED_ANGLE_SINE_ARM:
			sine_of_angle = mx_divide_safely( transverse_distance,
							longitudinal_distance );

			raw_angle = asin( sine_of_angle );
			break;
		
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal angle type %lu requested for motor '%s'.",
				coordinated_angle->angle_type,
				motor->record->name );
			break;
		}

		coordinated_angle->raw_angle_array[i] = raw_angle;

		sum += raw_angle;

		/* Save a copy of the individual motor angle
		 * expressed in user units.
		 */

		coordinated_angle->motor_angle_array[i] =
			raw_angle * motor->scale + motor->offset;
	}

	/* The raw position of the pseudomotor is the average of the
	 * raw angles of the child motors in radians.
	 */

	mean = mx_divide_safely( sum, coordinated_angle->num_motors );

	motor->raw_position.analog = mean;

	/* Now let us go back and compute the standard deviation
	 * and the motor angle errors.
	 */

	sum_of_squares = 0.0;

	for ( i = 0; i < coordinated_angle->num_motors; i++ ) {

		child_record = coordinated_angle->motor_record_array[i];

		raw_angle = coordinated_angle->raw_angle_array[i];

		raw_angle_error = raw_angle - mean;

		sum_of_squares += ( raw_angle_error * raw_angle_error );

		/* Save a copy of the raw angle error expressed
		 * in user units.
		 */

		coordinated_angle->motor_error_array[i] =
			raw_angle_error * motor->scale + motor->offset;
	}

	/* Compute the sample standard deviation. */

	sample_variance = mx_divide_safely( sum_of_squares,
			coordinated_angle->num_motors - 1 );

	if ( sample_variance < 0.0 ) {

		/* EEK! The sky is falling! */

		mx_warning(
		"The angle sample variance detected for record '%s' "
		"is %f.  This should never happen.",
			motor->record->name, sample_variance );

		sample_variance = 0.0;
	}

	coordinated_angle->standard_deviation = sqrt( sample_variance );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_coordinated_angle_soft_abort()";

	MX_COORDINATED_ANGLE *coordinated_angle;
	MX_RECORD *motor_record;
	long i, num_motors;
	mx_status_type mx_status;

	mx_status = mxd_coordinated_angle_get_pointers( motor,
						&coordinated_angle, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_COORDINATED_ANGLE_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s'", fname, motor->record->name ));
#endif

	num_motors = coordinated_angle->num_motors;

	for ( i = 0; i < num_motors; i++ ) {
		motor_record = coordinated_angle->motor_record_array[i];

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor record %ld for pseudomotor '%s' is NULL.",
				i, motor->record->name );
		}

		mx_status = mx_motor_soft_abort( motor_record );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_coordinated_angle_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_coordinated_angle_get_status()";

	MX_COORDINATED_ANGLE *coordinated_angle;
	MX_RECORD *motor_record;
	long i, num_motors;
	unsigned long motor_status, pseudomotor_status;
	mx_status_type mx_status;

	mx_status = mxd_coordinated_angle_get_pointers( motor,
						&coordinated_angle, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pseudomotor_status = 0;

	num_motors = coordinated_angle->num_motors;

	for ( i = 0; i < num_motors; i++ ) {
		motor_record = coordinated_angle->motor_record_array[i];

		if ( motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor record %ld for pseudomotor '%s' is NULL.",
				i, motor->record->name );
		}

		mx_status = mx_motor_get_status( motor_record, &motor_status );

		if ( motor_status & MXSF_MTR_IS_BUSY ) {
			pseudomotor_status |= MXSF_MTR_IS_BUSY;

			/* Break out of the while() loop. */
			break;
		}
	}

#if MXD_COORDINATED_ANGLE_DEBUG
	MX_DEBUG(-2,("%s: '%s' status = %lx",
		fname, motor->record->name, pseudomotor_status));
#endif

	motor->status = pseudomotor_status;

	return MX_SUCCESSFUL_RESULT;
}

