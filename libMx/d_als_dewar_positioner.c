/*
 * Name:    d_als_dewar_positioner.c 
 *
 * Purpose: MX motor driver to control the rotation and X-translation stages
 *          of an ALS dewar positioner used as part of the ALS sample changing
 *          robot design.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ALS_DEWAR_POSITIONER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_constants.h"
#include "mx_rs232.h"
#include "mx_relay.h"
#include "mx_motor.h"
#include "d_smartmotor.h"
#include "d_als_dewar_positioner.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_als_dewar_positioner_record_function_list = {
	NULL,
	mxd_als_dewar_positioner_create_record_structures,
	mxd_als_dewar_positioner_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_als_dewar_positioner_open
};

MX_MOTOR_FUNCTION_LIST mxd_als_dewar_positioner_motor_function_list = {
	NULL,
	mxd_als_dewar_positioner_move_absolute,
	mxd_als_dewar_positioner_get_position,
	mxd_als_dewar_positioner_set_position,
	mxd_als_dewar_positioner_soft_abort,
	mxd_als_dewar_positioner_immediate_abort,
	NULL,
	NULL,
	mxd_als_dewar_positioner_find_home_position,
	NULL,
	mxd_als_dewar_positioner_get_parameter,
	mxd_als_dewar_positioner_set_parameter,
	NULL,
	mxd_als_dewar_positioner_get_status
};

/* ALS dewar positioner data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_als_dewar_positioner_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_STEPPER_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_ALS_DEWAR_POSITIONER_STANDARD_FIELDS
};

long mxd_als_dewar_positioner_num_record_fields
		= sizeof( mxd_als_dewar_positioner_rf_defaults )
			/ sizeof( mxd_als_dewar_positioner_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_als_dewar_positioner_rfield_def_ptr
			= &mxd_als_dewar_positioner_rf_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_als_dewar_positioner_get_pointers( MX_MOTOR *motor,
			MX_ALS_DEWAR_POSITIONER **als_dewar_positioner,
			const char *calling_fname )
{
	static const char fname[] = "mxd_als_dewar_positioner_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( als_dewar_positioner == (MX_ALS_DEWAR_POSITIONER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ALS_DEWAR_POSITIONER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*als_dewar_positioner = (MX_ALS_DEWAR_POSITIONER *)
				(motor->record->record_type_struct);

	if ( *als_dewar_positioner == (MX_ALS_DEWAR_POSITIONER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ALS_DEWAR_POSITIONER pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_als_dewar_positioner_create_record_structures()";

	MX_MOTOR *motor;
	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	als_dewar_positioner = (MX_ALS_DEWAR_POSITIONER *)
				malloc( sizeof(MX_ALS_DEWAR_POSITIONER) );

	if ( als_dewar_positioner == (MX_ALS_DEWAR_POSITIONER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Can't allocate memory for MX_ALS_DEWAR_POSITIONER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = als_dewar_positioner;
	record->class_specific_function_list
				= &mxd_als_dewar_positioner_motor_function_list;

	motor->record = record;
	als_dewar_positioner->record = record;

	/* An ALS dewar positioner is treated as a stepper motor. */

	motor->subclass = MXC_MTR_STEPPER;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_als_dewar_positioner_finish_record_initialization()";

	MX_MOTOR *motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;

	motor->home_search = 0;

	if ( fabs( motor->scale - 1.0 ) > 1.0e-6 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The motor scale factor for pseudomotor '%s' MUST be set to 1 for "
	"this pseudomotor to function correctly.  Instead, you have it "
	"set to %g", record->name, motor->scale );
	}

	if ( fabs( motor->offset ) > 1.0e-6 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The motor offset for pseudomotor '%s' MUST be set to 0 for "
	"this pseudomotor to function correctly.  Instead, you have it "
	"set to %g", record->name, motor->offset );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_als_dewar_positioner_open()";

	MX_MOTOR *motor;
	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the starting speeds of the dewar motors. */

	mx_status = mx_motor_get_speed( als_dewar_positioner->dewar_rot_record,
			&(als_dewar_positioner->dewar_rot_normal_speed) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_speed( als_dewar_positioner->dewar_x_record,
			&(als_dewar_positioner->dewar_x_normal_speed) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the home speeds to 10 times slower. */

	als_dewar_positioner->dewar_rot_home_speed =
		1.0 * als_dewar_positioner->dewar_rot_normal_speed;

	als_dewar_positioner->dewar_x_home_speed =
		1.0 * als_dewar_positioner->dewar_x_normal_speed;

	return MX_SUCCESSFUL_RESULT;
}

#define PUCK_7_ORIENTATION_IN_DEGREES	5.0	/* FIXME: check this! */

#define NUM_OUTER_SAMPLES		11
#define NUM_INNER_SAMPLES		5

static mx_status_type
mxd_als_dewar_positioner_compute_7puck_location(
			MX_ALS_DEWAR_POSITIONER *als_dewar_positioner,
			long dewar_puck_number,
			long dewar_sample_number,
			double *dewar_rot_destination,
			double *dewar_x_destination )
{
	static const char fname[] =
		"mxd_als_dewar_positioner_compute_7puck_location()";

	double puck_center_x, puck_center_y, puck_orientation_angle;
	double puck_center_angle_in_degrees, puck_center_angle;
	double sample_relative_center_angle_in_degrees;
	double sample_relative_center_angle, sample_absolute_center_angle;
	double sample_relative_x, sample_relative_y;
	double sample_radius, sample_absolute_x, sample_absolute_y;
	double sample_angle, sample_angle_in_degrees;

	/* Angles in this function are expressed in radians, except
	 * where explicitly stated to be in degrees.  Linear distances
	 * are always expressed in millimeters.
	 */

	MX_DEBUG( 1,
	("%s: dewar_puck_number = %ld, dewar_sample_number = %ld",
		fname, dewar_puck_number, dewar_sample_number));

	/* Compute the X-Y position and orientation angle of the puck center. */

	if ( dewar_puck_number == 7 ) {
		puck_center_angle = 0.0;
		puck_center_x = 0.0;
		puck_center_y = 0.0;

		puck_orientation_angle = PUCK_7_ORIENTATION_IN_DEGREES
						* MX_RADIANS_PER_DEGREE;
	} else
	if ( ( dewar_puck_number >= 1 ) && ( dewar_puck_number <= 6 ) ) {
		puck_center_angle_in_degrees =
			60.0 * ( (double) dewar_puck_number - 0.5 );

		puck_center_angle = puck_center_angle_in_degrees
						* MX_RADIANS_PER_DEGREE;

		puck_center_x = als_dewar_positioner->puck_center_radius
					* cos( puck_center_angle );

		puck_center_y = als_dewar_positioner->puck_center_radius
					* sin( puck_center_angle );

		puck_orientation_angle = puck_center_angle
    + als_dewar_positioner->puck_orientation_offset * MX_RADIANS_PER_DEGREE;

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal puck number %ld requested for dewar positioner '%s'.",
		dewar_puck_number, als_dewar_positioner->record->name );
	}

	MX_DEBUG( 1,("%s: puck_center_angle = %g degrees",
		fname, puck_center_angle / MX_RADIANS_PER_DEGREE ));

	MX_DEBUG( 1,("%s: puck_center_x = %g, puck_center_y = %g",
		fname, puck_center_x, puck_center_y));

	MX_DEBUG( 1,("%s: puck_orientation_angle = %g degrees",
		fname, puck_orientation_angle / MX_RADIANS_PER_DEGREE ));

	/* Compute the position of the sample relative to the center
	 * of its puck.
	 */

	if ( ( dewar_sample_number < 0 ) || ( dewar_sample_number > 16 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	    "Illegal sample number %ld requested for dewar positioner '%s'.",
		dewar_sample_number, als_dewar_positioner->record->name );
	} else
	if ( dewar_sample_number == 0 ) {

		/* Sample 0 is the center of the puck and is only used
		 * for alignment.
		 */

		sample_relative_center_angle = 0.0;
		sample_absolute_center_angle = 0.0;
		sample_relative_x = 0.0;
		sample_relative_y = 0.0;
		
	} else
	if ( dewar_sample_number <= 5 ) {

		/* Samples 1 to 5 */
			
		/* Compute the angle of the sample relative to an X axis
		 * pointing toward the retaining pin.
		 */

		sample_relative_center_angle_in_degrees =
			( 360.0 / (double) NUM_INNER_SAMPLES )
				* ( (double) dewar_sample_number - 1.0 );

		sample_relative_center_angle = 
			sample_relative_center_angle_in_degrees
				* MX_RADIANS_PER_DEGREE;

		/* Convert to the absolute angles. */

		sample_absolute_center_angle = sample_relative_center_angle
					+ puck_orientation_angle;

		/* Compute the X-Y position relative to the puck center. */

		sample_relative_x = als_dewar_positioner->inner_sample_radius
					* cos( sample_absolute_center_angle );

		sample_relative_y = als_dewar_positioner->inner_sample_radius
					* sin( sample_absolute_center_angle );
	} else {
		/* Samples 6 to 16 */

		/* Compute the angle of the sample relative to an X axis
		 * pointing toward the retaining pin.
		 */

		sample_relative_center_angle_in_degrees =
			( 360.0 / (double) NUM_OUTER_SAMPLES )
	* ((double) dewar_sample_number - (double) NUM_INNER_SAMPLES - 1.0 );

		sample_relative_center_angle = 
			sample_relative_center_angle_in_degrees
				* MX_RADIANS_PER_DEGREE;

		/* Convert to the absolute angles. */

		sample_absolute_center_angle = sample_relative_center_angle
					+ puck_orientation_angle;

		/* Compute the X-Y position relative to the puck center. */

		sample_relative_x = als_dewar_positioner->outer_sample_radius
					* cos( sample_absolute_center_angle );

		sample_relative_y = als_dewar_positioner->outer_sample_radius
					* sin( sample_absolute_center_angle );
	}

	MX_DEBUG( 1,("%s: sample_relative_center_angle = %g degrees",
		fname, sample_relative_center_angle / MX_RADIANS_PER_DEGREE ));

	MX_DEBUG( 1,("%s: sample_absolute_center_angle = %g degrees",
		fname, sample_absolute_center_angle / MX_RADIANS_PER_DEGREE ));

	MX_DEBUG( 1,("%s: sample_relative_x = %g mm, sample_relative_y = %g mm",
		fname, sample_relative_x, sample_relative_y));

	/* Compute the sample absolute x and y positions. */

	sample_absolute_x = puck_center_x + sample_relative_x;

	sample_absolute_y = puck_center_y + sample_relative_y;

	MX_DEBUG( 1,("%s: sample_absolute_x = %g mm, sample_absolute_y = %g mm",
		fname, sample_absolute_x, sample_absolute_y));

	/* Compute the sample radius and angle. */

	sample_radius = sqrt( sample_absolute_x * sample_absolute_x
				+ sample_absolute_y * sample_absolute_y );

	sample_angle = atan2( sample_absolute_y, sample_absolute_x );

	sample_angle_in_degrees = sample_angle / MX_RADIANS_PER_DEGREE;

	MX_DEBUG( 1,("%s: sample_radius = %g, sample_angle = %g degrees",
		fname, sample_radius, sample_angle_in_degrees ));

	/*===*/

	*dewar_rot_destination = als_dewar_positioner->dewar_rot_offset
					+ sample_angle_in_degrees;

	*dewar_x_destination = als_dewar_positioner->dewar_x_offset
					- sample_radius;

	MX_DEBUG( 1,("%s: dewar_rot_destination = %g, dewar_x_destination = %g",
		fname, *dewar_rot_destination, *dewar_x_destination));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_move_absolute()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	long raw_destination, dewar_puck_number, dewar_sample_number;
	double dewar_rot_destination, dewar_x_destination;
	long vertical_slide_status, small_slide_status;
	mx_bool_type the_move_can_be_performed;
	unsigned long flags;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warning. */

	dewar_rot_destination = dewar_x_destination = 0.0;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Are either of the vertical slides not in the retracted position?
	 * If so, we cannot allow the move.
	 */

	mx_status = mx_get_relay_status(
			als_dewar_positioner->small_slide_record,
			&small_slide_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_relay_status(
			als_dewar_positioner->vertical_slide_record,
			&vertical_slide_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( small_slide_status == MXF_RELAY_IS_OPEN )
	  && ( vertical_slide_status == MXF_RELAY_IS_OPEN ) )
	{
		the_move_can_be_performed = TRUE;
	} else {
		the_move_can_be_performed = FALSE;
	}

	/* The flags field can override this check. */

	flags = als_dewar_positioner->als_dewar_positioner_flags;

	if ( flags & MXF_ALS_DEWAR_POSITIONER_IGNORE_SLIDE_STATUS ) {
		the_move_can_be_performed = TRUE;
	}

	/* If the move cannot be performed, tell the user why. */

	if ( the_move_can_be_performed == FALSE ) {

		if ( ( small_slide_status == MXF_RELAY_IS_CLOSED )
	  	  && ( vertical_slide_status == MXF_RELAY_IS_CLOSED ) )
		{
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The dewar positioner '%s' cannot be moved, since both "
			"the large vertical slide '%s' and the small vertical "
			"slide '%s' are extended.",
			motor->record->name, 
			als_dewar_positioner->vertical_slide_record->name,
			als_dewar_positioner->small_slide_record->name );
		}
		if ( small_slide_status == MXF_RELAY_IS_CLOSED ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The dewar positioner '%s' cannot be moved, since "
			"the small vertical slide '%s' is extended.",
			motor->record->name, 
			als_dewar_positioner->small_slide_record->name );
		}
		if ( vertical_slide_status == MXF_RELAY_IS_CLOSED ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The dewar positioner '%s' cannot be moved, since "
			"the large vertical slide '%s' is extended.",
			motor->record->name, 
			als_dewar_positioner->vertical_slide_record->name );
		}
		if ( small_slide_status != MXF_RELAY_IS_OPEN ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The dewar positioner '%s' cannot be moved, since "
			"the small vertical slide '%s' reports an "
			"illegal relay status %d.",
			motor->record->name, 
			als_dewar_positioner->small_slide_record->name,
			small_slide_status );
		}
		if ( vertical_slide_status != MXF_RELAY_IS_OPEN ) {
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"The dewar positioner '%s' cannot be moved, since "
			"the large vertical slide '%s' reports an "
			"illegal relay status %d.",
			motor->record->name, 
			als_dewar_positioner->vertical_slide_record->name,
			vertical_slide_status );
		}

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"This function decided that the requested move was not "
		"allowed, but it is not clear why.  You should never "
		"see this message, since it indicates there is a "
		"programming bug of some sort." );
	}

	/**** If we get here, we are allowed to perform the move. ****/

	/* Get the puck and sample number. */

	raw_destination = motor->raw_destination.stepper;

	dewar_puck_number = raw_destination / 1000;

	dewar_sample_number = raw_destination % 1000;

	MX_DEBUG( 2,("%s: raw_destination = %ld, ", fname, raw_destination));

	/* Compute the X and rotation positions required for
	 * the current sample.
	 */

	mx_status = mxd_als_dewar_positioner_compute_7puck_location(
				als_dewar_positioner,
				dewar_puck_number,
				dewar_sample_number,
				&dewar_rot_destination,
				&dewar_x_destination );

	/* Move the motors to the requested positions. */

	mx_status = mx_motor_move_absolute(
			als_dewar_positioner->dewar_rot_record,
			dewar_rot_destination, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute(
			als_dewar_positioner->dewar_x_record,
			dewar_x_destination, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.stepper = motor->raw_destination.stepper;

	motor->position = motor->offset + motor->scale
				* (double) motor->raw_position.stepper;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_get_position()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Just report back the last commanded move destination. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_set_position()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->raw_set_position.stepper != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"It is not possible to set the position of pseudomotor '%s' to any value "
"other than zero.  This pseudomotor is intended for use only via the "
"robotic sample changer record, so you should probably not be trying "
"to redefine its position directly anyway.", motor->record->name );
	}

	/* Zero the positions of the motors. */

	mx_status = mx_motor_set_position(
			als_dewar_positioner->dewar_rot_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position(
			als_dewar_positioner->dewar_x_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_soft_abort()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status, mx_status2;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort( 
			als_dewar_positioner->dewar_rot_record );

	/* Perform the soft abort for dewar_x even if the abort for
	 * dewar_rot failed.
	 */

	mx_status2 = mx_motor_soft_abort(
			als_dewar_positioner->dewar_x_record );

	if ( mx_status2.code != MXE_SUCCESS )
		return mx_status2;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] =
			"mxd_als_dewar_positioner_immediate_abort()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status, mx_status2;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort( 
			als_dewar_positioner->dewar_rot_record );

	/* Perform the immediate abort for dewar_x even if the abort for 
	 * dewar_rot failed.
	 */

	mx_status2 = mx_motor_immediate_abort(
			als_dewar_positioner->dewar_x_record );

	if ( mx_status2.code != MXE_SUCCESS )
		return mx_status2;

	return mx_status;
}

#define RESTORE_AFTER_HOME_SEARCH \
		mxd_als_dewar_positioner_restore_after_home_search( motor, \
							als_dewar_positioner )

static void
mxd_als_dewar_positioner_restore_after_home_search( MX_MOTOR *motor,
				MX_ALS_DEWAR_POSITIONER *als_dewar_positioner )
{
	/* Restore the motor speeds. */

	(void) mx_motor_set_speed( als_dewar_positioner->dewar_rot_record,
				als_dewar_positioner->dewar_rot_normal_speed );

	(void) mx_motor_set_speed( als_dewar_positioner->dewar_x_record,
				als_dewar_positioner->dewar_x_normal_speed );
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] =
		"mxd_als_dewar_positioner_find_home_position()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( motor->home_search == 0 ) {
		motor->home_search = 1;
	}

	/* Change the motor speeds for the home search. */

	mx_status = mx_motor_set_speed( als_dewar_positioner->dewar_rot_record,
				als_dewar_positioner->dewar_rot_home_speed );

	if ( mx_status.code != MXE_SUCCESS ) {
		RESTORE_AFTER_HOME_SEARCH;
		return mx_status;
	}

	mx_status = mx_motor_set_speed( als_dewar_positioner->dewar_x_record,
				als_dewar_positioner->dewar_x_home_speed );

	if ( mx_status.code != MXE_SUCCESS ) {
		RESTORE_AFTER_HOME_SEARCH;
		return mx_status;
	}

	/* Start the home search on the dewar rotation stage first. */

	mx_status = mx_motor_find_home_position(
			als_dewar_positioner->dewar_rot_record, -1 );

	if ( mx_status.code != MXE_SUCCESS ) {
		RESTORE_AFTER_HOME_SEARCH;
		return mx_status;
	}

	/* Then start the home search on the dewar translation stage. */

	mx_status = mx_motor_find_home_position(
			als_dewar_positioner->dewar_x_record, -1 );

	if ( mx_status.code != MXE_SUCCESS ) {
		RESTORE_AFTER_HOME_SEARCH;
		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxd_als_dewar_positioner_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_get_parameter()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	switch( motor->parameter_type ) {
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
		break;
	}
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_set_parameter()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	mx_status_type mx_status;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	switch( motor->parameter_type ) {
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			motor->parameter_type );
		break;
	}
#else
	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxd_als_dewar_positioner_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_als_dewar_positioner_get_status()";

	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;
	unsigned long dewar_rot_status, dewar_x_status, total_status;
	unsigned long home_search_succeeded;
	mx_status_type mx_status;

	mx_status = mxd_als_dewar_positioner_get_pointers( motor,
						&als_dewar_positioner, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the status flags for the individual motors. */

	mx_status = mx_motor_get_status( als_dewar_positioner->dewar_rot_record,
						&dewar_rot_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_get_status( als_dewar_positioner->dewar_x_record,
						&dewar_x_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: dewar_rot_status = %#lx, dewar_x_status = %#lx",
		fname, dewar_rot_status, dewar_x_status));

	/* For the most part, just or-ing together the individual status
	 * words does the right thing.
	 */

	total_status = dewar_rot_status | dewar_x_status;

	/* However, MXSF_MTR_HOME_SEARCH_SUCCEEDED should only be set if
	 * it is set for both of the motors.
	 */

	if ( ( dewar_rot_status & MXSF_MTR_HOME_SEARCH_SUCCEEDED )
	  && ( dewar_x_status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ) )
	{
		home_search_succeeded = MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	} else {
		home_search_succeeded = 0;
	}

	/* Now set the home search succeed flag in 'total_status' to match. */

	total_status &= ( ~ MXSF_MTR_HOME_SEARCH_SUCCEEDED );

	total_status |= home_search_succeeded;

	motor->status = total_status;

	MX_DEBUG( 2,("%s: motor->status = %#lx", fname, motor->status));

	/* See if it is time to restore the motor speeds. */

	if ( motor->home_search ) {
		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {
			/* If the motor is not moving,
			 * the home search must be over,
			 * so restore the motor parameters.
			 */

			motor->home_search = 0;

			RESTORE_AFTER_HOME_SEARCH;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

