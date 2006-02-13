/*
 * Name:    d_slit_motor.c 
 *
 * Purpose: MX motor driver to use a pair of MX motors as the sides
 *          of a slit.  Two records are needed to completely control
 *          a set of slit motors.  One record is for the slit center
 *          and one record is for the slit width.
 *
 * Note:    This driver may be used both for slit blade pairs for which
 *          positive moves are in the same direction and for slit blade
 *          pairs for which positive moves are in opposite directions.
 *          See 'd_slit_motor.h' for more information.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006 Illinois Institute of Technology
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
#include "d_slit_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_slit_motor_record_function_list = {
	NULL,
	mxd_slit_motor_create_record_structures,
	mxd_slit_motor_finish_record_initialization,
	NULL,
	mxd_slit_motor_print_motor_structure
};

MX_MOTOR_FUNCTION_LIST mxd_slit_motor_motor_function_list = {
	NULL,
	mxd_slit_motor_move_absolute,
	mxd_slit_motor_get_position,
	NULL,
	mxd_slit_motor_soft_abort,
	mxd_slit_motor_immediate_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_slit_motor_set_parameter,
	NULL,
	mxd_slit_motor_get_status
};

/* Slit motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_slit_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_SLIT_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_slit_motor_num_record_fields
		= sizeof( mxd_slit_motor_record_field_defaults )
			/ sizeof( mxd_slit_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_slit_motor_rfield_def_ptr
			= &mxd_slit_motor_record_field_defaults[0];

/* === */

static mx_status_type
mxd_slit_motor_get_pointers( MX_MOTOR *motor,
			MX_SLIT_MOTOR **slit_motor,
			MX_RECORD **positive_motor_record,
			MX_RECORD **negative_motor_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_slit_motor_get_pointers()";

	MX_SLIT_MOTOR *slit_motor_ptr;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	slit_motor_ptr = (MX_SLIT_MOTOR *) motor->record->record_type_struct;

	if ( slit_motor_ptr == (MX_SLIT_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SLIT_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( slit_motor != (MX_SLIT_MOTOR **) NULL ) {
		*slit_motor = slit_motor_ptr;
	}

	if ( positive_motor_record != (MX_RECORD **) NULL ) {
		*positive_motor_record = slit_motor_ptr->positive_motor_record;

		if ( *positive_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The positive_motor_record pointer for slit motor '%s' is NULL.",
				motor->record->name );
		}
	}

	if ( negative_motor_record != (MX_RECORD **) NULL ) {
		*negative_motor_record = slit_motor_ptr->negative_motor_record;

		if ( *negative_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The negative_motor_record pointer for slit motor '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_slit_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_slit_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_SLIT_MOTOR *slit_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	slit_motor = (MX_SLIT_MOTOR *) malloc( sizeof(MX_SLIT_MOTOR) );

	if ( slit_motor == (MX_SLIT_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SLIT_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = slit_motor;
	record->class_specific_function_list
				= &mxd_slit_motor_motor_function_list;

	motor->record = record;

	/* A slit motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_slit_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_slit_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *positive_motor_record;
	MX_RECORD *negative_motor_record;
	mx_status_type mx_status;

	/* Verify that the necessary pointers are set up correctly. */

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Perform standard motor record initialization. */

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;
	motor->motor_flags |= MXF_MTR_CANNOT_QUICK_SCAN;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_slit_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_slit_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	MX_SLIT_MOTOR *slit_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type      = SLIT_MOTOR.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  slit type       = %ld\n",
					(long) slit_motor->slit_type);
	fprintf(file, "  negative motor  = %s\n",
					negative_motor_record->name);
	fprintf(file, "  positive motor  = %s\n",
					positive_motor_record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position        = %g %s\n",
		position, motor->units);
	fprintf(file, "  slit scale    = %g %s per unscaled slit unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  slit offset   = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash        = %g %s.\n",
		motor->raw_backlash_correction.analog, motor->units);
	fprintf(file, "  negative limit  = %g %s.\n",
		motor->raw_negative_limit.analog, motor->units);
	fprintf(file, "  positive limit  = %g %s.\n",
		motor->raw_positive_limit.analog, motor->units);

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g %s.\n\n",
		move_deadband, motor->units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_slit_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_slit_motor_move_absolute()";

	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	MX_RECORD *motor_record_array[2];
	double new_slit_position, old_slit_position;
	double new_negative_motor_position, old_negative_motor_position;
	double new_positive_motor_position, old_positive_motor_position;
	double new_position_array[2];
	double slit_position_difference;
	long slit_flags, move_flags;
	mx_status_type mx_status;

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_slit_position = motor->raw_destination.analog;

	/* Check the limits on this request. */

	if ( new_slit_position > motor->raw_positive_limit.analog
	  || new_slit_position < motor->raw_negative_limit.analog )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"Requested position %g is outside the allowed range of %g to %g",
			new_slit_position, motor->raw_negative_limit.analog,
			motor->raw_positive_limit.analog );
	}

	/* Get the old motor and slit positions. */

	if ( motor->use_start_positions ) {

		/* This will be used for a pseudomotor step scan. */

		old_negative_motor_position =
			slit_motor->saved_negative_motor_start_position;

		old_positive_motor_position =
			slit_motor->saved_positive_motor_start_position;
	} else {
		/* This will be used for all other cases. */

		mx_status = mx_motor_get_position( negative_motor_record,
						&old_negative_motor_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_get_position( positive_motor_record,
						&old_positive_motor_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Compute the new motor positions. */

	switch( slit_motor->slit_type ) {
	case MXF_SLIT_CENTER_SAME:
		old_slit_position = 0.5 *
		  (old_positive_motor_position + old_negative_motor_position);

		slit_position_difference
			= new_slit_position - old_slit_position;

		new_negative_motor_position = old_negative_motor_position
					+ slit_position_difference;

		new_positive_motor_position = old_positive_motor_position
					+ slit_position_difference;
		break;
	case MXF_SLIT_WIDTH_SAME:
		old_slit_position = old_positive_motor_position
					- old_negative_motor_position;

		slit_position_difference
			= new_slit_position - old_slit_position;

		new_negative_motor_position = old_negative_motor_position
					- 0.5 * slit_position_difference;

		new_positive_motor_position = old_positive_motor_position
					+ 0.5 * slit_position_difference;
		break;
	case MXF_SLIT_CENTER_OPPOSITE:
		old_slit_position = 0.5 *
		  (old_positive_motor_position - old_negative_motor_position);

		slit_position_difference
			= new_slit_position - old_slit_position;

		new_negative_motor_position = old_negative_motor_position
					- slit_position_difference;

		new_positive_motor_position = old_positive_motor_position
					+ slit_position_difference;
		break;
	case MXF_SLIT_WIDTH_OPPOSITE:
		old_slit_position = old_positive_motor_position
					+ old_negative_motor_position;

		slit_position_difference
			= new_slit_position - old_slit_position;

		new_negative_motor_position = old_negative_motor_position
					+ 0.5 * slit_position_difference;

		new_positive_motor_position = old_positive_motor_position
					+ 0.5 * slit_position_difference;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown slit motor type %ld for slit motor '%s'",
			(long) slit_motor->slit_type, motor->record->name );
	}

	/* Now perform the move. */

	motor_record_array[0] = negative_motor_record;
	motor_record_array[1] = positive_motor_record;

	new_position_array[0] = new_negative_motor_position;
	new_position_array[1] = new_positive_motor_position;

	slit_flags = slit_motor->slit_flags;

	if ( slit_flags & MXF_SLIT_MOTOR_SIMULTANEOUS_START ) {
		move_flags = MXF_MTR_NOWAIT | MXF_MTR_SIMULTANEOUS_START;
	} else {
		move_flags = MXF_MTR_NOWAIT;
	}

	mx_status = mx_motor_array_move_absolute( 2, motor_record_array,
			new_position_array, move_flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_slit_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_slit_motor_get_position()";

	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	double slit_position;
	double negative_motor_position, positive_motor_position;
	mx_status_type mx_status;

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the current motor positions. */

	mx_status = mx_motor_get_position( negative_motor_record,
						&negative_motor_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	slit_motor->negative_motor_position = negative_motor_position;

	mx_status = mx_motor_get_position( positive_motor_record,
						&positive_motor_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	slit_motor->positive_motor_position = positive_motor_position;

	/* Compute the new motor positions. */

	switch( slit_motor->slit_type ) {
	case MXF_SLIT_CENTER_SAME:
		slit_position = 0.5 *
			(positive_motor_position + negative_motor_position);
		break;
	case MXF_SLIT_WIDTH_SAME:
		slit_position = positive_motor_position
					- negative_motor_position;
		break;
	case MXF_SLIT_CENTER_OPPOSITE:
		slit_position = 0.5 *
			(positive_motor_position - negative_motor_position);
		break;
	case MXF_SLIT_WIDTH_OPPOSITE:
		slit_position = positive_motor_position
					+ negative_motor_position;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown slit motor type %ld for slit motor '%s'",
			(long) slit_motor->slit_type, motor->record->name );
	}

	motor->raw_position.analog = slit_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_slit_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_slit_motor_soft_abort()";

	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	mx_status_type mx_status, mx_status_negative, mx_status_positive;

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_negative = mx_motor_soft_abort( negative_motor_record );

	mx_status_positive = mx_motor_soft_abort( positive_motor_record );

	if ( mx_status_positive.code != MXE_SUCCESS ) {
		return mx_status_positive;
	} else {
		return mx_status_negative;
	}
}

MX_EXPORT mx_status_type
mxd_slit_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_slit_motor_immediate_abort()";

	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	mx_status_type mx_status, mx_status_negative, mx_status_positive;

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_negative = mx_motor_immediate_abort( negative_motor_record );

	mx_status_positive = mx_motor_immediate_abort( positive_motor_record );

	if ( mx_status_positive.code != MXE_SUCCESS ) {
		return mx_status_positive;
	} else {
		return mx_status_negative;
	}
}

MX_EXPORT mx_status_type
mxd_slit_motor_set_parameter( MX_MOTOR *motor )
{
	const char fname[] = "mxd_slit_motor_set_parameter()";

	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	double pseudomotor_start, pseudomotor_difference;
	double negative_position, positive_position;
	double negative_start, positive_start;
	mx_status_type mx_status;

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for motor '%s' for parameter type '%s' (%d).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));

	switch ( motor->parameter_type ) {
	case MXLV_MTR_SAVE_START_POSITIONS:
		MX_DEBUG( 2,("%s: motor '%s' invoked for %g %s",
			fname, motor->record->name,
			motor->save_start_positions,
			motor->units));

		/* Get the current position of the pseudomotor.
		 *
		 * This will, as a side effect, cause the current
		 * positions of the real motors to be put into
		 * 'slit_motor->negative_motor_position' and
		 * 'slit_motor->positive_motor_position'. Also,
		 * the raw pseudomotor position will be in
		 * 'motor->raw_position.analog'.
		 *
		 * Next, we compute the difference between the
		 * current pseudomotor position and the 
		 * 'motor->raw_saved_start_position' value.
		 *
		 * Finally, we subtract that difference from the
		 * two real motor current positions and save that
		 * in 'slit_motor->saved_start_position_array'.
		 */

		mx_status = mxd_slit_motor_get_position( motor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		pseudomotor_start = motor->raw_saved_start_position;

		negative_position = slit_motor->negative_motor_position;

		positive_position = slit_motor->positive_motor_position;

		/*==*/

		slit_motor->pseudomotor_start_position = pseudomotor_start;

		pseudomotor_difference = pseudomotor_start
						- motor->raw_position.analog;

		MX_DEBUG( 2,
	("%s: motor '%s' pseudomotor_start = %g, raw_position = %g",
			fname, motor->record->name,
			pseudomotor_start,
			motor->raw_position.analog ));

		MX_DEBUG( 2,("%s: pseudomotor_difference = %g",
			fname, pseudomotor_difference));

		switch( slit_motor->slit_type ) {
		case MXF_SLIT_CENTER_SAME:
			negative_start = negative_position
						+ pseudomotor_difference;
			positive_start = positive_position
						+ pseudomotor_difference;
			break;
		case MXF_SLIT_WIDTH_SAME:
			negative_start = negative_position
						- 0.5 * pseudomotor_difference;
			positive_start = positive_position
						+ 0.5 * pseudomotor_difference;
			break;
		case MXF_SLIT_CENTER_OPPOSITE:
			negative_start = negative_position
						- pseudomotor_difference;
			positive_start = positive_position
						+ pseudomotor_difference;
			break;
		case MXF_SLIT_WIDTH_OPPOSITE:
			negative_start = negative_position
						+ 0.5 * pseudomotor_difference;
			positive_start = positive_position
						+ 0.5 * pseudomotor_difference;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			    "Unknown slit motor type %ld for slit motor '%s'",
				(long) slit_motor->slit_type,
				motor->record->name );
		}

		slit_motor->saved_positive_motor_start_position
						= positive_start;

		slit_motor->saved_negative_motor_start_position
						= negative_start;

		MX_DEBUG( 2,
		("%s: positive_position = %g, positive_start = %g",
			fname, positive_position, positive_start));

		MX_DEBUG( 2,
		("%s: negative_motor_position = %g, negative_start = %g",
			fname, negative_position, negative_start));

                MX_DEBUG( 2,("%s: pseudomotor_start_position = %g",
                        fname, slit_motor->pseudomotor_start_position));
		break;

	case MXLV_MTR_USE_START_POSITIONS:
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_slit_motor_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_slit_motor_get_status()";

	MX_SLIT_MOTOR *slit_motor;
	MX_RECORD *negative_motor_record, *positive_motor_record;
	mx_hex_type positive_motor_status, negative_motor_status;
	mx_status_type mx_status;

	mx_status = mxd_slit_motor_get_pointers( motor, &slit_motor,
						&positive_motor_record,
						&negative_motor_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( negative_motor_record,
						&negative_motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( positive_motor_record,
						&positive_motor_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	/* Busy status */

	if ( negative_motor_status & MXSF_MTR_IS_BUSY ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}
	if ( positive_motor_status & MXSF_MTR_IS_BUSY ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/* Home search status */

	if ( ( negative_motor_status & MXSF_MTR_HOME_SEARCH_SUCCEEDED )
	  && ( positive_motor_status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ) )
	{
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/* Following error status */

	if ( negative_motor_status & MXSF_MTR_FOLLOWING_ERROR ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}
	if ( positive_motor_status & MXSF_MTR_FOLLOWING_ERROR ) {
		motor->status |= MXSF_MTR_FOLLOWING_ERROR;
	}

	/* Drive fault status */

	if ( negative_motor_status & MXSF_MTR_DRIVE_FAULT ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}
	if ( positive_motor_status & MXSF_MTR_DRIVE_FAULT ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/* Axis enable status */

	if ( negative_motor_status & MXSF_MTR_AXIS_DISABLED ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}
	if ( positive_motor_status & MXSF_MTR_AXIS_DISABLED ) {
		motor->status |= MXSF_MTR_AXIS_DISABLED;
	}

	/* Determining the limit status is a little more complicated. */

	switch( slit_motor->slit_type ) {
	case MXF_SLIT_CENTER_SAME:
	case MXF_SLIT_WIDTH_OPPOSITE:

		if ( ( positive_motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT )
		  || ( negative_motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) )
		{
			if ( motor->scale >= 0.0 ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			} else {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}
		}

		if ( ( positive_motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT )
		  || ( negative_motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) )
		{
			if ( motor->scale >= 0.0 ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			} else {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			}
		}

		break;
	case MXF_SLIT_WIDTH_SAME:
	case MXF_SLIT_CENTER_OPPOSITE:

		if ( ( positive_motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT )
		  || ( negative_motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT ) )
		{
			if ( motor->scale >= 0.0 ) {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			} else {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			}
		}

		if ( ( positive_motor_status & MXSF_MTR_NEGATIVE_LIMIT_HIT )
		  || ( negative_motor_status & MXSF_MTR_POSITIVE_LIMIT_HIT ) )
		{
			if ( motor->scale >= 0.0 ) {
				motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
			} else {
				motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
			}
		}

		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal slit type %ld for slit motor '%s'",
			(long) slit_motor->slit_type, motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

