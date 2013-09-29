/*
 * Name:    d_trans_motor.c 
 *
 * Purpose: MX motor driver to move 1 or more motors in the same direction.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006-2007, 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "d_trans_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_trans_motor_record_function_list = {
	mxd_trans_motor_initialize_driver,
	mxd_trans_motor_create_record_structures,
	mxd_trans_motor_finish_record_initialization,
	NULL,
	mxd_trans_motor_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_trans_motor_motor_function_list = {
	mxd_trans_motor_motor_is_busy,
	mxd_trans_motor_move_absolute,
	mxd_trans_motor_get_position,
	mxd_trans_motor_set_position,
	mxd_trans_motor_soft_abort,
	mxd_trans_motor_immediate_abort,
	mxd_trans_motor_positive_limit_hit,
	mxd_trans_motor_negative_limit_hit,
	NULL,
	mxd_trans_motor_constant_velocity_move,
	mxd_trans_motor_get_parameter,
	mxd_trans_motor_set_parameter
};

/* Translation motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_trans_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_TRANS_MOTOR_STANDARD_FIELDS
};

long mxd_trans_motor_num_record_fields
		= sizeof( mxd_trans_motor_record_field_defaults )
			/ sizeof( mxd_trans_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_trans_motor_rfield_def_ptr
			= &mxd_trans_motor_record_field_defaults[0];

/* === */

#define NUM_TRANS_MOTOR_FIELDS	3

/* A private function for the use of the driver. */

static mx_status_type
mxd_trans_motor_get_pointers( MX_MOTOR *motor,
			MX_TRANSLATION_MOTOR **translation_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_trans_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( translation_motor == (MX_TRANSLATION_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_TRANSLATION_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*translation_motor = (MX_TRANSLATION_MOTOR *)
					motor->record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_trans_motor_initialize_driver( MX_DRIVER *driver )
{
        static const char fname[] = "mxs_trans_motor_initialize_driver()";

        const char field_name[NUM_TRANS_MOTOR_FIELDS][MXU_FIELD_NAME_LENGTH+1]
            = {
                "motor_record_array",
		"position_array",
	        "saved_start_position_array"};

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

        MX_DEBUG( 2,("%s: num_motors varargs cookie = %ld",
                        fname, num_motors_varargs_cookie));

	for ( i = 0; i < NUM_TRANS_MOTOR_FIELDS; i++ ) {
		mx_status = mx_find_record_field_defaults( driver,
						field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_motors_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_trans_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_TRANSLATION_MOTOR *trans_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	trans_motor = (MX_TRANSLATION_MOTOR *)
				malloc( sizeof(MX_TRANSLATION_MOTOR) );

	if ( trans_motor == (MX_TRANSLATION_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TRANSLATION_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = trans_motor;
	record->class_specific_function_list
				= &mxd_trans_motor_motor_function_list;

	motor->record = record;

	/* A translation motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_finish_record_initialization( MX_RECORD *record )
{
	MX_MOTOR *motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_trans_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_TRANSLATION_MOTOR *translation_motor;
	MX_RECORD **motor_record_array;
	long i, num_motors;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	translation_motor = (MX_TRANSLATION_MOTOR *)
					(record->record_type_struct);

	if ( translation_motor == (MX_TRANSLATION_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_TRANSLATION_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;


	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type           = TRANSLATION_MOTOR.\n\n");

	fprintf(file, "  name                 = %s\n", record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position             = %g %s  (%g).\n",
		motor->position, motor->units, motor->raw_position.analog );
	fprintf(file, "  scale                = %g %s per unscaled "
							"translation unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  offset               = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash             = %g %s  (%g).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog);
	fprintf(file, "  negative limit       = %g %s  (%g).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog);
	fprintf(file, "  positive limit       = %g %s  (%g).\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband        = %g %s  (%g).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	fprintf(file, "  num motors           = %ld\n", num_motors);
	fprintf(file, "  motor record array   = (");

	if ( motor_record_array == NULL ) {
		fprintf(file, "NULL");
	} else {
		for ( i = 0; i < num_motors; i++ ) {
			fprintf(file, "%s", motor_record_array[i]->name);

			if ( i != num_motors-1 ) {
				fprintf(file, ", ");
			}
		}
	}
	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_motor_is_busy()";

	MX_TRANSLATION_MOTOR *translation_motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	mx_bool_type busy;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	for ( i = 0; i < num_motors; i++ ) {
	
		child_motor_record = motor_record_array[i];

		if ( child_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Motor record pointer for child motor '%s' for translation motor '%s' is NULL.",
			motor_record_array[i]->name,
			motor->record->name );
		}

		mx_status = mx_motor_is_busy( child_motor_record, &busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the child motor is busy, we are done. */

		if ( busy == TRUE ) {
			motor->busy = TRUE;

			return MX_SUCCESSFUL_RESULT;
		}
	}

	motor->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_move_absolute()";

	MX_TRANSLATION_MOTOR *translation_motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	double *position_array;
	long i, num_motors, trans_flags;
	int move_flags;
	double new_translation_position;
	double old_translation_position;
	double translation_position_difference;
	double position_sum;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;
	position_array = translation_motor->position_array;

	new_translation_position = motor->raw_destination.analog;

	/* Check the limits on this request. */

	if ( new_translation_position > motor->raw_positive_limit.analog
	  || new_translation_position < motor->raw_negative_limit.analog )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
		new_translation_position, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the old positions. */

	position_sum = 0.0;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_get_position(
			child_motor_record, &position_array[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_sum += position_array[i];
	}

	old_translation_position = position_sum / (double) num_motors;

	/* Compute the new positions. */

	if ( motor->use_start_positions ) {
		/* If we are in a pseudomotor step scan, this method will
		 * be used to compute the real motor destinations.
		 */

		translation_position_difference = new_translation_position
			- translation_motor->pseudomotor_start_position;

		for ( i = 0; i < num_motors; i++ ) {
			position_array[i] =
				translation_motor->saved_start_position_array[i]
				+ translation_position_difference;

			MX_DEBUG( 2,("%s: motor '%s' destination = %g", fname,
				translation_motor->motor_record_array[i]->name,
				position_array[i]));
		}
	} else {
		/* For all other cases, we just the current real motor
		 * positions, to compute the new destinations.
		 */

		translation_position_difference = new_translation_position
			- old_translation_position;

		for ( i = 0; i < num_motors; i++ ) {
			position_array[i] += translation_position_difference;
		}
	}

	/* Perform the move. */

	trans_flags = translation_motor->translation_flags;

	if ( trans_flags & MXF_TRANSLATION_MOTOR_SIMULTANEOUS_START ) {
		move_flags = MXF_MTR_NOWAIT | MXF_MTR_SIMULTANEOUS_START;
	} else {
		move_flags = MXF_MTR_NOWAIT;
	} 

	mx_status = mx_motor_array_move_absolute(
			num_motors, motor_record_array, position_array,
			move_flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trans_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_get_position()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	double *position_array;
	long i, num_motors;
	MX_TRANSLATION_MOTOR *translation_motor;
	double translation_position;
	double position_sum;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;
	position_array = translation_motor->position_array;

	/* Get the current motor positions. */

	position_sum = 0.0;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_get_position(
			child_motor_record, &position_array[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_sum += position_array[i];
	}

	translation_position = position_sum / (double) num_motors;

	motor->raw_position.analog = translation_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_set_position()";

	MX_TRANSLATION_MOTOR *translation_motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	double *position_array;
	long i, num_motors;
	double new_translation_position;
	double old_translation_position;
	double translation_position_difference;
	double position_sum;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;
	position_array = translation_motor->position_array;

	new_translation_position = motor->raw_set_position.analog;

	/* Check the limits on this request. */

	if ( new_translation_position > motor->raw_positive_limit.analog
	  || new_translation_position < motor->raw_negative_limit.analog )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
		new_translation_position, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the old positions. */

	position_sum = 0.0;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_get_position(
			child_motor_record, &position_array[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		position_sum += position_array[i];
	}

	old_translation_position = position_sum / (double) num_motors;

	translation_position_difference = new_translation_position
		- old_translation_position;

	/* Compute the new positions and see if they are within
	 * the software limits.
	 */

	for ( i = 0; i < num_motors; i++ ) {
		position_array[i] += translation_position_difference;

		mx_status = mx_motor_check_position_limits( motor_record_array[i],
							position_array[i], 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now set the new positions. */

	for ( i = 0; i < num_motors; i++ ) {
		mx_status = mx_motor_set_position( motor_record_array[i],
						position_array[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_soft_abort()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	MX_TRANSLATION_MOTOR *translation_motor;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_soft_abort( child_motor_record );
	}


	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trans_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_immediate_abort()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	long i, num_motors;
	MX_TRANSLATION_MOTOR *translation_motor;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_immediate_abort( child_motor_record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trans_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_positive_limit_hit()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_TRANSLATION_MOTOR *translation_motor;
	long i, num_motors;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	limit_hit = FALSE;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_positive_limit_hit(
					child_motor_record, &limit_hit );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( limit_hit == TRUE ) {
			(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Positive limit hit for motor '%s'",
				child_motor_record->name );

			break;             /* exit the for() loop */
		}
	}

	motor->positive_limit_hit = limit_hit;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_negative_limit_hit()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_TRANSLATION_MOTOR *translation_motor;
	long i, num_motors;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	limit_hit = FALSE;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_negative_limit_hit(
					child_motor_record, &limit_hit );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( limit_hit == TRUE ) {
			(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"Negative limit hit for motor '%s'",
				child_motor_record->name );

			break;             /* exit the for() loop */
		}
	}

	motor->negative_limit_hit = limit_hit;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_constant_velocity_move()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_TRANSLATION_MOTOR *translation_motor;
	long i, num_motors;
	long direction;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	direction = motor->constant_velocity_move;

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	for ( i = 0; i < num_motors; i++ ) {
		child_motor_record = motor_record_array[i];

		mx_status = mx_motor_constant_velocity_move(
					child_motor_record, direction );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_trans_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_get_parameter()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_TRANSLATION_MOTOR *translation_motor;
	long i, num_motors;
	double sum, max_value, double_value;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	switch( motor->parameter_type ) {
	case MXLV_MTR_MAXIMUM_SPEED:
		break;

	case MXLV_MTR_SPEED:
		sum = 0.0;

		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_get_speed( child_motor_record,
								&double_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			sum += double_value;
		}

		motor->raw_speed = mx_divide_safely( sum, (double) num_motors );
		break;

	case MXLV_MTR_BASE_SPEED:
		sum = 0.0;

		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_get_base_speed( child_motor_record,
								&double_value );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			sum += double_value;
		}

		motor->raw_base_speed = mx_divide_safely( sum,
							(double) num_motors );
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the value of synchronous_motion_mode is "
			"unsupported since the pseudomotor '%s' depends on "
			"multiple motors and there is no unique answer.",
				motor->record->name );

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the value of the acceleration parameters is "
			"unsupported since the pseudomotor '%s' depends on "
			"multiple motors and there is no unique answer.",
				motor->record->name );

	case MXLV_MTR_ACCELERATION_DISTANCE:
		max_value = 0.0;

		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_get_acceleration_distance(
					child_motor_record, &double_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( double_value > max_value )
				max_value = double_value;
		}

		motor->acceleration_distance = max_value;
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		max_value = 0.0;

		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_get_acceleration_time(
					child_motor_record, &double_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( double_value > max_value )
				max_value = double_value;
		}

		motor->acceleration_time = max_value;
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		return mx_motor_default_get_parameter_handler( motor );

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		motor->compute_pseudomotor_position[1]
				= motor->compute_pseudomotor_position[0];
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		motor->compute_real_position[1]
				= motor->compute_real_position[0];
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_trans_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_trans_motor_set_parameter()";

	MX_RECORD **motor_record_array;
	MX_RECORD *child_motor_record;
	MX_TRANSLATION_MOTOR *translation_motor;
	long i, num_motors;
	double start_position, end_position, time_for_move, speed;
	double pseudomotor_difference, position_sum;
	double *position_array, *start_position_array;
	mx_status_type mx_status;

	translation_motor = NULL;

	mx_status = mxd_trans_motor_get_pointers( motor,
						&translation_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	num_motors = translation_motor->num_motors;
	motor_record_array = translation_motor->motor_record_array;

	switch ( motor->parameter_type ) {
	case MXLV_MTR_MAXIMUM_SPEED:
		break;

	case MXLV_MTR_SPEED:
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_set_speed( child_motor_record,
							motor->raw_speed );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_BASE_SPEED:
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_set_base_speed( child_motor_record,
							motor->raw_base_speed );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_set_raw_acceleration_parameters(
					child_motor_record,
					motor->raw_acceleration_parameters );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		start_position = motor->raw_speed_choice_parameters[0];
		end_position = motor->raw_speed_choice_parameters[1];
		time_for_move = motor->raw_speed_choice_parameters[2];

		speed = fabs( mx_divide_safely( end_position - start_position,
							time_for_move ) );
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_set_speed(child_motor_record, speed);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_SAVE_SPEED:
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_save_speed( child_motor_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_RESTORE_SPEED:
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_restore_speed( child_motor_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_SYNCHRONOUS_MOTION_MODE:
		for ( i = 0; i < num_motors; i++ ) {
			child_motor_record = motor_record_array[i];

			mx_status = mx_motor_set_synchronous_motion_mode(
						child_motor_record,
						motor->synchronous_motion_mode);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
		break;

	case MXLV_MTR_SAVE_START_POSITIONS:
		MX_DEBUG( 2,("%s: motor '%s' invoked for %g %s",
			fname, motor->record->name,
			motor->save_start_positions,
			motor->units));

		/* Get the current position of the pseudomotor.
		 *
		 * This will, as a side effect, cause the current
		 * positions of the real motors to be put into
		 * the array 'translation_motor->position_array'.
		 * Also the raw pseudomotor position will be in
		 * 'motor->raw_position.analog'.
		 *
		 * Next, we compute the difference between the
		 * current pseudomotor position and the 
		 * 'motor->raw_saved_start_position' value.
		 *
		 * Finally, we subtract that difference from each
		 * of the three real motor current positions and
		 * save that in 
		 * 'translation_motor->saved_start_position_array'.
		 */

		mx_status = mxd_trans_motor_get_position( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

                MX_DEBUG( 2,
        ("%s: motor '%s' raw_saved_start_position = %g, raw_position = %g",
                        fname, motor->record->name,
                        motor->raw_saved_start_position,
                        motor->raw_position.analog ));

		pseudomotor_difference = motor->raw_saved_start_position
						- motor->raw_position.analog;

		MX_DEBUG( 2,("%s: pseudomotor_difference = %g",
			fname, pseudomotor_difference));

		start_position_array
			= translation_motor->saved_start_position_array;

		position_array = translation_motor->position_array;

		position_sum = 0.0;

		for ( i = 0; i < translation_motor->num_motors; i++ ) {
			MX_DEBUG( 2,("%s: position_array[%ld] = %g",
				fname, i, position_array[i]));

			start_position_array[i]
				= position_array[i] + pseudomotor_difference;

			MX_DEBUG( 2,("%s: start_position_array[%ld] = %g",
				fname, i, start_position_array[i]));

			position_sum += start_position_array[i];
		}

		translation_motor->pseudomotor_start_position
			= mx_divide_safely( position_sum,
				(double) translation_motor->num_motors );

		MX_DEBUG( 2,("%s: pseudomotor_start_position = %g",
			fname, translation_motor->pseudomotor_start_position));
		break;

	case MXLV_MTR_USE_START_POSITIONS:
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

