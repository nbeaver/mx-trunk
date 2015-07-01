/*
 * Name:    d_sercat_als_robot.c
 *
 * Purpose: MX driver for SER-CAT's version of the ALS sample changing robot.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2007, 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SERCAT_ALS_ROBOT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_motor.h"
#include "mx_relay.h"
#include "mx_sample_changer.h"

#include "d_als_dewar_positioner.h"
#include "d_sercat_als_robot.h"

MX_RECORD_FUNCTION_LIST mxd_sercat_als_robot_record_function_list = {
	NULL,
	mxd_sercat_als_robot_create_record_structures
};

MX_SAMPLE_CHANGER_FUNCTION_LIST
			mxd_sercat_als_robot_sample_changer_function_list
  = {
	mxd_sercat_als_robot_initialize,
	mxd_sercat_als_robot_shutdown,
	mxd_sercat_als_robot_mount_sample,
	mxd_sercat_als_robot_unmount_sample,
	mxd_sercat_als_robot_grab_sample,
	mxd_sercat_als_robot_ungrab_sample,
	mxd_sercat_als_robot_select_sample_holder,
	mxd_sercat_als_robot_unselect_sample_holder,
	mxd_sercat_als_robot_soft_abort,
	mxd_sercat_als_robot_immediate_abort,
	mxd_sercat_als_robot_idle,
	mxd_sercat_als_robot_reset,
	mxd_sercat_als_robot_get_status,
	mx_sample_changer_default_get_parameter_handler,
	mxd_sercat_als_robot_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_sercat_als_robot_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SAMPLE_CHANGER_STANDARD_FIELDS,
	MXD_SERCAT_ALS_ROBOT_STANDARD_FIELDS
};

long mxd_sercat_als_robot_num_record_fields
		= sizeof( mxd_sercat_als_robot_record_field_defaults )
		/ sizeof( mxd_sercat_als_robot_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sercat_als_robot_rfield_def_ptr
			= &mxd_sercat_als_robot_record_field_defaults[0];

#define MXD_SERCAT_ALS_ROBOT_SET_FAULT( status ) \
		do { \
			(status) &= ( ~ MXSF_CHG_IS_BUSY ); \
			(status) |= ( MXSF_CHG_FAULT | MXSF_CHG_ERROR ); \
		} while (0)

#define MXD_SERCAT_ALS_ROBOT_SET_USER_ABORT( status ) \
		do { \
			(status) &= ( ~ MXSF_CHG_IS_BUSY ); \
			(status) |= ( MXSF_CHG_USER_ABORT | MXSF_CHG_ERROR ); \
		} while (0)

/* --- */

static mx_status_type
mxd_sercat_als_robot_get_pointers( MX_SAMPLE_CHANGER *changer,
			MX_SERCAT_ALS_ROBOT **sercat_als_robot,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sercat_als_robot_get_pointers()";

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAMPLE_CHANGER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sercat_als_robot == (MX_SERCAT_ALS_ROBOT **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SERCAT_ALS_ROBOT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sercat_als_robot = (MX_SERCAT_ALS_ROBOT *)
			changer->record->record_type_struct;

	if ( *sercat_als_robot == (MX_SERCAT_ALS_ROBOT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SERCAT_ALS_ROBOT pointer for record '%s' is NULL.",
			changer->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sercat_als_robot_relay_command( MX_SAMPLE_CHANGER *changer,
				MX_SERCAT_ALS_ROBOT *sercat_als_robot,
				MX_RECORD *relay_record,
				int relay_command,
				unsigned long delay_ms )
{
	static const char fname[] = "mxd_sercat_als_robot_relay_command()";

	unsigned long i, max_attempts, wait_ms;
	double timeout;
	long relay_status;
	unsigned long total_delay;
	mx_status_type mx_status;

#if MXD_SERCAT_ALS_ROBOT_DEBUG
	MX_DEBUG(-2,("%s: relay '%s', command = %d",
		fname, relay_record->name, relay_command));
#endif

	mx_status = mx_get_relay_status( relay_record, &relay_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( relay_status == relay_command ) {
		/* If the actuator is already at the right position,
		 * we need not send it any commands.
		 */

#if MXD_SERCAT_ALS_ROBOT_DEBUG
		MX_DEBUG(-2,("%s: relay is already at the right place.",fname));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Tell the actuator to move. */

	mx_status = mx_relay_command( relay_record, relay_command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	total_delay = delay_ms + sercat_als_robot->relay_delay_ms;

#if 1 /* FIXME: Status readback for the vert actuator doesn't work. */

	if ( strcmp( relay_record->name, "vert" ) == 0 ) {
		total_delay += 2000;
	}
#endif

	mx_msleep( total_delay );

	max_attempts = 50;

	wait_ms = 100;

	for ( i = 0; i < max_attempts; i++ ) {
		mx_status = mx_get_relay_status( relay_record, &relay_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXD_SERCAT_ALS_ROBOT_DEBUG
		MX_DEBUG(-2,("%s: attempt %ld, relay_status = %d",
			fname, i, relay_status));
#endif

		if ( relay_status == relay_command ) {
			break;		/* Exit the for() loop. */
		}

		mx_msleep( 100 );
	}

	if ( i >= max_attempts ) {
		timeout = 0.001 * (double) (max_attempts * wait_ms);

		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for relay '%s' to change "
		"to position %d", timeout, relay_record->name, relay_command );
	}

	return mx_status;
}

static mx_status_type
mxd_sercat_als_robot_go_to_idle( MX_SAMPLE_CHANGER *changer,
			MX_SERCAT_ALS_ROBOT *sercat_als_robot )
{
	mx_status_type mx_status;

	/* Retract the actuators.  It is done in the following order to
	 * minimize the chance of things running into each other.
	 */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->vertical_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->sample_rotation_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->horizontal_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}


static mx_status_type
mxd_sercat_als_robot_go_to_goniostat( MX_SAMPLE_CHANGER *changer,
			MX_SERCAT_ALS_ROBOT *sercat_als_robot )
{
	mx_status_type mx_status;

	/* Before going to the goniostat, we must retract the small slide,
	 * sample rotation, and the vertical slide.
	 */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->vertical_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->sample_rotation_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Move the horizontal slide to the correct position at the goniostat.*/

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->horizontal_slide_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Extend the small slide. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait a second for the position of the gripper to settle down. */

	mx_msleep(1000);

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sercat_als_robot_go_to_dewar_cooldown_position( MX_SAMPLE_CHANGER *changer,
			MX_SERCAT_ALS_ROBOT *sercat_als_robot )
{
	double dewar_destination;
	mx_status_type mx_status;

	/* Before going to the dewar, we must retract the small slide,
	 * sample rotation, and the vertical slide.
	 */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->vertical_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->sample_rotation_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Move the dewar to the correct position. */

	dewar_destination = 1000.0 * atof( changer->current_sample_holder )
				+ (double) changer->current_sample_id;

	mx_status = mx_motor_move_absolute(
				sercat_als_robot->dewar_positioner_record,
				dewar_destination, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_stop( 
				sercat_als_robot->dewar_positioner_record, 0 );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Move the horizontal slide to the correct position over the dewar. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->horizontal_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Rotate the sample holder. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->sample_rotation_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Extend the vertical slide. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->vertical_slide_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sercat_als_robot_go_to_deice( MX_SAMPLE_CHANGER *changer,
			MX_SERCAT_ALS_ROBOT *sercat_als_robot )
{
	static const char fname[] = "mxd_sercat_als_robot_go_to_deice()";

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sercat_als_robot_get_x_motor( MX_SERCAT_ALS_ROBOT *sercat_als_robot,
					MX_RECORD **x_motor_record )
{
	static const char fname[] = "mxd_sercat_als_robot_get_x_motor()";

	MX_RECORD *dewar_positioner_record;
	MX_ALS_DEWAR_POSITIONER *als_dewar_positioner;

	if ( sercat_als_robot == (MX_SERCAT_ALS_ROBOT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SERCAT_ALS_ROBOT pointer passed was NULL." );
	}
	if ( x_motor_record == (MX_RECORD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dewar_positioner_record = sercat_als_robot->dewar_positioner_record;

	if ( dewar_positioner_record == ( MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'dewar_positioner_record' pointer for the "
		"MX_SERCAT_ALS_ROBOT pointer passed is NULL." );
	}

	als_dewar_positioner = (MX_ALS_DEWAR_POSITIONER *)
				dewar_positioner_record->record_type_struct;

	if ( als_dewar_positioner == (MX_ALS_DEWAR_POSITIONER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ALS_DEWAR_POSITIONER pointer for record '%s' is NULL.",
			dewar_positioner_record->name );
	}

	*x_motor_record = als_dewar_positioner->dewar_x_record;

	if ( *x_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'dewar_x_record' pointer for record '%s' is NULL.",
			dewar_positioner_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_sercat_als_robot_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sercat_als_robot_create_record_structures()";

	MX_SAMPLE_CHANGER *changer;
	MX_SERCAT_ALS_ROBOT *sercat_als_robot;

	changer = (MX_SAMPLE_CHANGER *)
				malloc( sizeof(MX_SAMPLE_CHANGER) );

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SAMPLE_CHANGER structure." );
	}

	sercat_als_robot = (MX_SERCAT_ALS_ROBOT *)
				malloc( sizeof(MX_SERCAT_ALS_ROBOT) );

	if ( sercat_als_robot == (MX_SERCAT_ALS_ROBOT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SERCAT_ALS_ROBOT structure." );
	}

	record->record_class_struct = changer;
	record->record_type_struct = sercat_als_robot;
	record->class_specific_function_list = 
		&mxd_sercat_als_robot_sample_changer_function_list;

	changer->record = record;
	sercat_als_robot->record = record;

	strlcpy( changer->current_sample_holder, MX_CHG_NO_SAMPLE_HOLDER,
			sizeof(changer->current_sample_holder) );

	changer->current_sample_id = MX_CHG_NO_SAMPLE_ID;
	changer->sample_is_mounted = FALSE;

	strlcpy( changer->requested_sample_holder, MX_CHG_NO_SAMPLE_HOLDER,
			sizeof(changer->requested_sample_holder) );

	changer->requested_sample_id = MX_CHG_NO_SAMPLE_ID;

	changer->status = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_initialize( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_initialize()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	MX_RECORD *x_motor_record;
	unsigned long dewar_positioner_status;
	mx_status_type mx_status;

	/* Suppress bogus GCC 4 uninitialized variable warnings. */

	sercat_als_robot = NULL;
	x_motor_record = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->status |= MXSF_CHG_INITIALIZATION_IN_PROGRESS;

	/* Retract all of the actuators. */

	mx_status = mxd_sercat_als_robot_go_to_idle( changer,
						sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Perform a home search on the dewar.
	 *
	 * The Animatics SmartMotors currently used by the robot use their
	 * negative limit switches as home switches, so we must home search
	 * in the negative direction.
	 */

	mx_status = mx_motor_raw_home_command(
			sercat_als_robot->dewar_positioner_record, -1 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Wait for the home search to complete.
	 *
	 * The Animatics SmartMotors set several error bits when they hit
	 * the limit switches.  The home search only completes successfully
	 * if we ignore these errors.
	 */

	mx_status = mx_wait_for_motor_stop(
			sercat_als_robot->dewar_positioner_record,
			MXF_MTR_IGNORE_LIMIT_SWITCHES | MXF_MTR_IGNORE_ERRORS );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Did the home search succeed? */

	mx_status = mx_motor_get_status( 
			sercat_als_robot->dewar_positioner_record,
			&dewar_positioner_status );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	if ( ( dewar_positioner_status & MXSF_MTR_HOME_SEARCH_SUCCEEDED ) == 0 )
	{
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"The dewar home search for sample changer '%s' failed.  "
		"The motor status for dewar positioner '%s' is %#lx",
			changer->record->name,
			sercat_als_robot->dewar_positioner_record->name,
			dewar_positioner_status );
	}

	/* If the home search succeeded, define the current positions of
	 * the motors as zero.
	 */

	mx_status = mx_motor_set_position(
			sercat_als_robot->dewar_positioner_record, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* The robot rotation stage disables its limit switch after the
	 * home switch completes, but the X translation stage does not.
	 * We can avoid future limit switch aborts by moving the X stage
	 * off the limit right now.
	 */

	mx_status = mxd_sercat_als_robot_get_x_motor( sercat_als_robot,
							&x_motor_record );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	mx_status = mx_motor_move_absolute( x_motor_record,
						5.0, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	mx_status = mx_wait_for_motor_stop( x_motor_record,
			MXF_MTR_IGNORE_LIMIT_SWITCHES | MXF_MTR_IGNORE_ERRORS );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	changer->status &= ( ~ MXSF_CHG_INITIALIZATION_IN_PROGRESS );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_shutdown( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_shutdown()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->status |= MXSF_CHG_SHUTDOWN_IN_PROGRESS;

	mx_status = mxd_sercat_als_robot_go_to_idle( changer,
						sercat_als_robot );

	changer->status &= ( ~ MXSF_CHG_SHUTDOWN_IN_PROGRESS );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_mount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_mount_sample()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( changer->sample_is_mounted ) {
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
		"Sample changer '%s' already has sample %ld from puck %s "
		"mounted on the goniostat.", changer->record->name,
			changer->current_sample_id,
			changer->current_sample_holder );
	}

	changer->status |= MXSF_CHG_OPERATION_IN_PROGRESS;

	/* Move to the correct position at the goniostat. */

	mx_status = mxd_sercat_als_robot_go_to_goniostat( changer,
						sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Mount the sample. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->gripper_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Retract the small stage. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	changer->sample_is_mounted = TRUE;

	changer->status &= ( ~ MXSF_CHG_OPERATION_IN_PROGRESS );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_unmount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_unmount_sample()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( changer->sample_is_mounted == FALSE ) {
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
		"Sample changer '%s' does not currently have a sample "
		"mounted on the goniostat.", changer->record->name );
	}

	changer->status |= MXSF_CHG_OPERATION_IN_PROGRESS;

	/* Move to the dewar cooldown position. */

	mx_status = mxd_sercat_als_robot_go_to_dewar_cooldown_position(
						changer, sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Wait for the gripper to cool down. */

	mx_msleep( sercat_als_robot->cooldown_delay_ms );

	/* Move to the correct position at the goniostat. */

	mx_status = mxd_sercat_als_robot_go_to_goniostat( changer,
						sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Mount the sample. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->gripper_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Retract the small stage. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	changer->sample_is_mounted = FALSE;

	changer->status &= ( ~ MXSF_CHG_OPERATION_IN_PROGRESS );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_grab_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_grab_sample()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( changer->current_sample_id != MX_CHG_NO_SAMPLE_ID ) {
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
		"Sample changer '%s' already has grabbed sample %ld "
		"from puck %s.", changer->record->name,
				changer->current_sample_id,
				changer->current_sample_holder );
	}
	if ( strcmp( changer->current_sample_holder,
			MX_CHG_NO_SAMPLE_HOLDER ) == 0 )
	{
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
		"No puck has been selected for sample changer '%s'.",
				changer->record->name );
	}

	changer->current_sample_id = changer->requested_sample_id;

	changer->status |= MXSF_CHG_OPERATION_IN_PROGRESS;

	/* Move to the dewar cooldown position. */

	mx_status = mxd_sercat_als_robot_go_to_dewar_cooldown_position(
						changer, sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Wait for the gripper to cool down. */

	mx_msleep( sercat_als_robot->cooldown_delay_ms );

	/* Extend the small stage. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	mx_msleep(1000);

	/* Grab the sample with the gripper. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->gripper_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	mx_msleep(1000);

	/* Retract the small stage. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	changer->status &= ( ~ MXSF_CHG_OPERATION_IN_PROGRESS );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_ungrab_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_ungrab_sample()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( changer->current_sample_id == MX_CHG_NO_SAMPLE_ID ) {
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
			"Sample changer '%s' has not grabbed a sample.",
			changer->record->name );
	}
	if ( strcmp( changer->current_sample_holder,
			MX_CHG_NO_SAMPLE_HOLDER ) == 0 )
	{
		return mx_error( MXE_CLIENT_REQUEST_DENIED, fname,
		"No puck has been selected for sample changer '%s'.",
				changer->record->name );
	}

	changer->status |= MXSF_CHG_OPERATION_IN_PROGRESS;

	/* Move to the dewar cooldown position. */

	mx_status = mxd_sercat_als_robot_go_to_dewar_cooldown_position(
						changer, sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	/* Extend the small stage. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_CLOSE_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	mx_msleep(1000);

	/* Release the sample from the gripper. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->gripper_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	mx_msleep(1000);

	/* Retract the small stage. */

	mx_status = mxd_sercat_als_robot_relay_command(
				changer, sercat_als_robot,
				sercat_als_robot->small_slide_record,
				MXF_OPEN_RELAY, 0 );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	changer->current_sample_id = MX_CHG_NO_SAMPLE_ID;

	changer->status &= ( ~ MXSF_CHG_OPERATION_IN_PROGRESS );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_select_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_sercat_als_robot_select_sample_holder()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( changer->current_sample_holder,
			changer->requested_sample_holder,
			MXU_SAMPLE_HOLDER_NAME_LENGTH );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_unselect_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_sercat_als_robot_unselect_sample_holder()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( changer->current_sample_holder,
			MX_CHG_NO_SAMPLE_HOLDER,
			MXU_SAMPLE_HOLDER_NAME_LENGTH );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_soft_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_soft_abort()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_soft_abort(
				sercat_als_robot->dewar_positioner_record );

	MXD_SERCAT_ALS_ROBOT_SET_USER_ABORT( changer->status );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_immediate_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_immediate_abort()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_immediate_abort(
				sercat_als_robot->dewar_positioner_record );

	MXD_SERCAT_ALS_ROBOT_SET_USER_ABORT( changer->status );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_idle( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_idle()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->status &= ( ~ MXSF_CHG_OPERATION_IN_PROGRESS );

	changer->status |= MXSF_CHG_IDLE_IN_PROGRESS;

	mx_status = mxd_sercat_als_robot_go_to_idle( changer,
						sercat_als_robot );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXD_SERCAT_ALS_ROBOT_SET_FAULT( changer->status );

		return mx_status;
	}

	changer->status &= ( ~ MXSF_CHG_IDLE_IN_PROGRESS );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_reset( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_reset()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->status &= ( ~ MXSF_CHG_ERROR_BITMASK );

	strlcpy( changer->current_sample_holder, MX_CHG_NO_SAMPLE_HOLDER,
			sizeof(changer->current_sample_holder) );

	changer->current_sample_id = MX_CHG_NO_SAMPLE_ID;

	changer->sample_is_mounted = FALSE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_get_status( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_status()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( changer->current_sample_holder,
			MX_CHG_NO_SAMPLE_HOLDER ) == 0 )
	{
		changer->status &= ( ~ MXSF_CHG_SAMPLE_HOLDER_SELECTED );
	} else {
		changer->status |= MXSF_CHG_SAMPLE_HOLDER_SELECTED;
	}

	if ( changer->current_sample_id == MX_CHG_NO_SAMPLE_ID ) {
		changer->status &= ( ~ MXSF_CHG_SAMPLE_GRABBED );
	} else {
		changer->status |= MXSF_CHG_SAMPLE_GRABBED;
	}

	if ( changer->sample_is_mounted == FALSE ) {
		changer->status &= ( ~ MXSF_CHG_SAMPLE_MOUNTED );
	} else {
		changer->status |= MXSF_CHG_SAMPLE_MOUNTED;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sercat_als_robot_set_parameter( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_sercat_als_robot_set_parameter()";

	MX_SERCAT_ALS_ROBOT *sercat_als_robot;
	mx_status_type mx_status;

	sercat_als_robot = NULL;

	mx_status = mxd_sercat_als_robot_get_pointers( changer,
						&sercat_als_robot, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( changer->parameter_type ) {
	case MXLV_CHG_COOLDOWN:
		mx_status = mxd_sercat_als_robot_go_to_dewar_cooldown_position(
						changer, sercat_als_robot );
		break;
	case MXLV_CHG_DEICE:
		mx_status = mxd_sercat_als_robot_go_to_deice( changer,
							sercat_als_robot );
		break;
	default:
		mx_status =  mx_sample_changer_default_set_parameter_handler(
								changer );
		break;
	}

	return mx_status;
}

