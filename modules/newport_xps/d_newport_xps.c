/*
 * Name:    d_newport_xps.c
 *
 * Purpose: MX driver for Galil-controlled motors via the gclib library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014-2015, 2017, 2019, 2021-2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NEWPORT_XPS_MOTOR_DEBUG				FALSE

#define MXD_NEWPORT_XPS_MOTOR_DEBUG_MOTOR_GROUPS		FALSE

#define MXD_NEWPORT_XPS_MOTOR_POSITION_DEBUG			FALSE

#define MXD_NEWPORT_XPS_MOTOR_MOVE_THREAD_DEBUG			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "i_newport_xps.h"
#include "d_newport_xps.h"

/* Vendor include files. */

#include "XPS_C8_drivers.h"
#include "XPS_C8_errors.h"

/* ============ Motor channels ============ */

MX_RECORD_FUNCTION_LIST mxd_newport_xps_record_function_list = {
	NULL,
	mxd_newport_xps_create_record_structures,
	mx_motor_finish_record_initialization,
	NULL,
	NULL,
	mxd_newport_xps_open,
	NULL,
	NULL,
	mxd_newport_xps_resynchronize,
	mxd_newport_xps_special_processing_setup
};

MX_MOTOR_FUNCTION_LIST mxd_newport_xps_motor_function_list = {
	NULL,
	mxd_newport_xps_move_absolute,
	mxd_newport_xps_get_position,
	mxd_newport_xps_set_position,
	mxd_newport_xps_soft_abort,
	mxd_newport_xps_immediate_abort,
	NULL,
	NULL,
	mxd_newport_xps_raw_home_command,
	NULL,
	mxd_newport_xps_get_parameter,
	mxd_newport_xps_set_parameter,
	NULL,
	mxd_newport_xps_get_status
};

/* NEWPORT_XPS_MOTOR motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_newport_xps_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_NEWPORT_XPS_MOTOR_STANDARD_FIELDS
};

long mxd_newport_xps_num_record_fields
		= sizeof( mxd_newport_xps_record_field_defaults )
			/ sizeof( mxd_newport_xps_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_newport_xps_rfield_def_ptr
			= &mxd_newport_xps_record_field_defaults[0];

/*---*/

static mx_status_type mxd_newport_xps_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );
/*---*/

static mx_status_type
mxd_newport_xps_get_pointers( MX_MOTOR *motor,
			MX_NEWPORT_XPS_MOTOR **newport_xps_motor,
			MX_NEWPORT_XPS **newport_xps,
			const char *calling_fname )
{
	static const char fname[] = "mxd_newport_xps_get_pointers()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor_ptr;
	MX_RECORD *newport_xps_record;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	newport_xps_motor_ptr = (MX_NEWPORT_XPS_MOTOR *)
				motor->record->record_type_struct;

	if ( newport_xps_motor_ptr == (MX_NEWPORT_XPS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT_XPS_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( newport_xps_motor != (MX_NEWPORT_XPS_MOTOR **) NULL ) {
		*newport_xps_motor = newport_xps_motor_ptr;
	}

	if ( newport_xps != (MX_NEWPORT_XPS **) NULL ) {
		newport_xps_record = newport_xps_motor_ptr->newport_xps_record;

		if ( newport_xps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The newport_xps_record pointer for record '%s' "
			"is NULL.", motor->record->name );
		}

		*newport_xps = (MX_NEWPORT_XPS *)
				newport_xps_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/* This is a utility function used by mxd_newport_xps_move_thread().
 *
 * It runs in the context of the 'move' thread and must use the socket
 * newport_xps_motor->move_thread_socket_id to communicate with the
 * Newport controller.
 */

static mx_status_type
mxd_newport_xps_home_or_set_position( MX_MOTOR *motor,
				MX_NEWPORT_XPS_MOTOR *newport_xps_motor,
				MX_NEWPORT_XPS *newport_xps )
{
	static const char fname[] = "mxd_newport_xps_home_or_set_position()";

	int xps_status, group_status;
	unsigned long command_type;

	command_type = newport_xps_motor->command_type;

	/* Verify that this is an allowed command type for this function. */

	switch( command_type ) {
	case MXT_NEWPORT_XPS_GROUP_HOME_SEARCH:
	case MXT_NEWPORT_XPS_GROUP_SET_POSITION:
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Command type %lu for motor '%s' is not "
			"permitted by this function.",
			command_type, motor->record->name );
		break;
	}

	/*------*/

	if ( ( command_type == MXT_NEWPORT_XPS_GROUP_HOME_SEARCH )
	  || ( newport_xps_motor->num_motors_in_group == 1 ) )
	{
		/*------------------------------------------------------+
		 | This code path handles two cases:                    |
		 |   1. Home search (for any size group)                |
		 |   2. Set position (if only 1 motor is in this group) |
		 +------------------------------------------------------*/

		/* The first step is to put the group into the NOT REFERENCED
		 * state by killing and then initializing the positioner.
		 */

		xps_status = GroupKill(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupKill()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		xps_status = GroupInitialize(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupInitialize()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		/* The motor may move a bit after initialization, so wait
		 * a while to give that time to happen.
		 */

		mx_msleep( newport_xps_motor->set_position_sleep_ms );

		/*------------------------------------------------+
		 | Home searches and 'set position' commands take |
		 | different paths back to the READY state.       |
		 +------------------------------------------------*/

		switch( newport_xps_motor->command_type ) {
		case MXT_NEWPORT_XPS_GROUP_HOME_SEARCH:

			/* Start the home search. */

			MX_DEBUG(-2,
			("Home search '%s': Calling GroupHomeSearch()",
				motor->record->name ));

			xps_status = GroupHomeSearch(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

			/* When we get here, the home search has completed
			 * and/or aborted.  Check to see what happened.
			 */

			MX_DEBUG(-2,
			("Home search '%s': GroupHomeSearch() status = %d",
				motor->record->name, xps_status));

			if ( xps_status != SUCCESS ) {
				motor->latched_status
					|= MXSF_MTR_DEVICE_ACTION_FAILED;

				newport_xps_motor->home_search_succeeded
							= FALSE;

				return mxi_newport_xps_error(
					"GroupHomeSearch()",
					newport_xps_motor->record->name,
				    newport_xps_motor->move_thread_socket_id,
					xps_status );
			}
			break;

		case MXT_NEWPORT_XPS_GROUP_SET_POSITION:

			/* Redefine the current position. */

			MX_DEBUG(-2,
			("Set position '%s': Calling GroupReferencingStart()",
				motor->record->name ));

			xps_status = GroupReferencingStart(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				motor->latched_status
					|= MXSF_MTR_DEVICE_ACTION_FAILED;

				return mxi_newport_xps_error(
					"GroupReferencingStart()",
					newport_xps_motor->record->name,
				    newport_xps_motor->move_thread_socket_id,
					xps_status );
			}

			MX_DEBUG(-2,
		("Set position '%s': Calling GroupReferencingActionExecute()",
				motor->record->name ));

			xps_status = GroupReferencingActionExecute(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->positioner_name,
				"SetPosition", "None",
				motor->raw_set_position.analog );

			if ( xps_status != SUCCESS ) {
				motor->latched_status
					|= MXSF_MTR_DEVICE_ACTION_FAILED;

				return mxi_newport_xps_error(
					"GroupReferencingActionExecute()",
					newport_xps_motor->record->name,
				    newport_xps_motor->move_thread_socket_id,
					xps_status );
			}

			xps_status = GroupReferencingStop(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				motor->latched_status
					|= MXSF_MTR_DEVICE_ACTION_FAILED;

				return mxi_newport_xps_error(
					"GroupReferencingStop()",
					newport_xps_motor->record->name,
				    newport_xps_motor->move_thread_socket_id,
					xps_status );
			}
			break;
		}

		/* If the sequence of operations we just performed succeeded,
		 * then the motor's group should be in group state 11.  If
		 * we are _not_ in group state 11, then something went wrong
		 * and we must report this to the user.
		 */

		xps_status = GroupStatusGet(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name,
				&group_status );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupStatusGet() after Homing or Referencing",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		MX_DEBUG(-2,
		("After homing or referencing '%s': group_status = %d",
			motor->record->name, group_status ));

		switch( newport_xps_motor->command_type ) {
		case MXT_NEWPORT_XPS_GROUP_HOME_SEARCH:

			if ( group_status == 11 ) {
			    newport_xps_motor->home_search_succeeded = TRUE;
			} else {
			    newport_xps_motor->home_search_succeeded = FALSE;

			    mx_warning( "Home search failed for motor '%s'.  "
				"Newport group status = %d",
					motor->record->name,
					group_status );
			}
			break;

		case MXT_NEWPORT_XPS_GROUP_SET_POSITION:
			if ( group_status != 11 ) {
			    mx_warning( "Set position failed for motor '%s'.  "
				"Newport group status = %d",
					motor->record->name,
					group_status );
			}
			break;
		}

	} else {
		/**** 'Set position' with multiple motors in the group ****/

		MX_RECORD *other_newport_xps_motor_record = NULL;
		MX_NEWPORT_XPS_MOTOR *other_newport_xps_motor = NULL;
		long i, index_of_self;

		mx_warning("%s: motor '%s', group '%s', has %ld motors in "
			"its group. 'homing' and 'set position' for more "
			"than one motor has not yet been tested "
			"(as of 2015/04/14).",
			fname, motor->record->name,
			newport_xps_motor->group_name,
			newport_xps_motor->num_motors_in_group);

		/* First, save the positions of all of the motors in
		 * the group.
		 */

		index_of_self = -1;

		for ( i = 0; i < newport_xps_motor->num_motors_in_group; i++ ) {
		    other_newport_xps_motor_record =
			newport_xps_motor->motor_records_in_group[i];

		    if ( other_newport_xps_motor_record == (MX_RECORD *) NULL )
		    {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer at "
			"newport_xps_motor->motor_records_in_group[%ld] "
			"for Newport XPS motor '%s' is NULL.",
				i, motor->record->name );
		    }

		    other_newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *)
			other_newport_xps_motor_record->record_type_struct;

		    if ( other_newport_xps_motor ==
			(MX_NEWPORT_XPS_MOTOR *) NULL ) 
		    {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NEWPORT_XPS_MOTOR pointer for Newport XPS "
			"motor '%s' is NULL.",
				other_newport_xps_motor_record->name );
		    }

		    /* Along the way, we figure out which motor in this array
		     * is the same as the caller's motor.
		     */

		    if ( other_newport_xps_motor == newport_xps_motor ) {
			index_of_self = i;
		    }

		    xps_status = GroupPositionCurrentGet(
			newport_xps_motor->move_thread_socket_id,
			other_newport_xps_motor->positioner_name,
			1,
			&(newport_xps_motor->old_motor_positions_in_group[i]) );

		    if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupReferencingStop()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		    }
#if MXD_NEWPORT_XPS_MOTOR_DEBUG_MOTOR_GROUPS
		    MX_DEBUG(-2,("%s: motor '%s' saved position = %g",
			fname, other_newport_xps_motor_record->name,
			newport_xps_motor->old_motor_positions_in_group[i] ));
#endif
		}

		/* Did we find ourself in the motor_records_in_group array? */

		if ( index_of_self < 0 ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Motor '%s' was not found in its internal array "
			"newport_xps_motor->motor_records_in_group.",
				motor->record->name );
		}

		/* Overwrite old_motor_positions_in_group[ index_of_self ]
		 * with the requested new position of this motor.
		 */

		newport_xps_motor->old_motor_positions_in_group[ index_of_self ]
			= motor->raw_set_position.analog;

		/* Now put the group into the NOT REFERENCED state
		 * by killing and then reinitializing the group.
		 */

		xps_status = GroupKill(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupKill()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		xps_status = GroupInitialize(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupInitialize()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		/* The motor may move a bit after initialization, so wait
		 * a while to give that time to happen.
		 */

		mx_msleep( newport_xps_motor->set_position_sleep_ms );

		/* Set positions for all of the motors using the cached values
		 * in the old_motor_positions_in_group array.  Remember that
		 * we overwrote the value for the caller's motor using the
		 * new 'set_position' value requested by the user.
		 */

		/* Put us into the REFERENCING state. */

		xps_status = GroupReferencingStart(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupReferencingStart()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		/* Note that we skip some pointer checking this time around
		 * since we did that already in the first for() loop.
		 */

		for ( i = 0; i < newport_xps_motor->num_motors_in_group; i++ ) {
		    other_newport_xps_motor_record =
			newport_xps_motor->motor_records_in_group[i];

		    other_newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *)
			other_newport_xps_motor_record->record_type_struct;

		    xps_status = GroupReferencingActionExecute(
			newport_xps_motor->move_thread_socket_id,
			other_newport_xps_motor->positioner_name,
			"SetPosition",
			"None",
			newport_xps_motor->old_motor_positions_in_group[i] );

		    if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupReferencingStop()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		    }
		}

#if MXD_NEWPORT_XPS_MOTOR_DEBUG_MOTOR_GROUPS
		MX_DEBUG(-2,("%s: Set motor '%s' position to %g",
			fname, motor->record->name,
	    newport_xps_motor->old_motor_positions_in_group[index_of_self] ));
#endif
		/* We have finished doing REFERENCING state actions,
		 * so leave that state.
		 */

		xps_status = GroupReferencingStop(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

		if ( xps_status != SUCCESS ) {
			motor->latched_status |= MXSF_MTR_DEVICE_ACTION_FAILED;

			return mxi_newport_xps_error(
				"GroupReferencingStop()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
		}

		/* We are now finished with 'set position' for the
		 * case of multiple motors in a group.
		 */
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

/*
 * mxd_newport_xps_move_thread() is a persistent MX thread function that
 * handles 'move', 'home search', and 'set position' commands.  It must
 * do all of its communication with the Newport controller via the socket
 * newport_xps_motor->move_thread_socket_id to avoid deadlocks.
 */

static mx_status_type
mxd_newport_xps_move_thread( MX_THREAD *thread, void *thread_argument )
{
	static const char fname[] = "mxd_newport_xps_move_thread()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_MOTOR *motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	double starting_position, destination;
	int xps_status;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( thread_argument == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The thread_argument pointer for this thread is NULL." );
	}

	motor = (MX_MOTOR *) thread_argument;

	/*---*/

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
						&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( newport_xps_motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_NEWPORT_XPS_MOTOR ptr %p is NULL",
			newport_xps_motor );
	}

	/* Lock the mutex in preparation for entering the thread's
	 * event handler loop.
	 */

	mx_status_code = mx_mutex_lock( newport_xps_motor->move_thread_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the move_thread_mutex for "
		"Newport XPS motor '%s' failed.",
			newport_xps_motor->record->name );
	}

	while (TRUE) {

		/* Wait on the condition variable. */

		if ( newport_xps_motor->move_in_progress == FALSE ) {
			mx_status = mx_condition_variable_wait(
					newport_xps_motor->move_thread_cv,
					newport_xps_motor->move_thread_mutex );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if MXD_NEWPORT_XPS_MOTOR_MOVE_THREAD_DEBUG
		MX_DEBUG(-2,("%s: '%s' command = %lu, destination = %f",
			fname, newport_xps_motor->record->name,
			newport_xps_motor->command_type,
			newport_xps_motor->command_destination));
#endif
		switch( newport_xps_motor->command_type ) {
		case MXT_NEWPORT_XPS_GROUP_MOVE_ABSOLUTE:
			/* Get the current position of the motor, just in
			 * case we get ERR_PARAMETER_OUT_OF_RANGE.  If that
			 * happens, then that will let us figure out which
			 * controller limit we were trying to exceed.
			 */

			mx_status = mxd_newport_xps_get_position( motor );

			if ( mx_status.code != MXE_SUCCESS ) {
				motor->latched_status
					|= MXSF_MTR_DEVICE_ACTION_FAILED;

				/* If we cannot get the motor's current
				 * position, then we will not attempt
				 * to make the move.
				 */
				break;
			}

			starting_position = motor->position;

			destination = newport_xps_motor->command_destination;

			xps_status = GroupMoveAbsolute(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->positioner_name,
				1,
				&(newport_xps_motor->command_destination) );

			switch( xps_status ) {
			case SUCCESS:
			case ERR_GROUP_ABORT_MOTION:   /* Move was aborted. */
				break;
			default:
				switch( xps_status ) {
				case ERR_PARAMETER_OUT_OF_RANGE:
					if ( destination >= starting_position ){
						motor->latched_status
					    |= MXSF_MTR_SOFT_POSITIVE_LIMIT_HIT;
					} else {
						motor->latched_status
					    |= MXSF_MTR_SOFT_NEGATIVE_LIMIT_HIT;
					}
					break;
				case ERR_FOLLOWING_ERROR:
					motor->latched_status
						|= MXSF_MTR_FOLLOWING_ERROR;
					break;
				case ERR_EMERGENCY_SIGNAL:
				case ERR_FATAL_INIT:
				case ERR_IN_INITIALIZATION:
				case ERR_GROUP_MOTION_DONE_TIMEOUT:
					motor->latched_status
					    |= MXSF_MTR_DEVICE_ACTION_FAILED;
					break;
				case ERR_NOT_ALLOWED_ACTION:
					motor->latched_status
							|= MXSF_MTR_NOT_READY;
					break;
				case ERR_GROUP_NAME:
				case ERR_POSITIONER_NAME:
					motor->latched_status
					    |= MXSF_MTR_CONFIGURATION_ERROR;
					break;
				case ERR_WRONG_FORMAT:
				case ERR_WRONG_OBJECT_TYPE:
				case ERR_WRONG_PARAMETERS_NUMBER:
				default:
					motor->latched_status |= MXSF_MTR_ERROR;
					break;
				}

				(void) mxi_newport_xps_error(
					"GroupMoveAbsolute()",
					newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
					xps_status );

				break;
			}
			break;

		case MXT_NEWPORT_XPS_GROUP_HOME_SEARCH:

			newport_xps_motor->home_search_succeeded = FALSE;

			/* Fall through to the 'set position' case. */

		case MXT_NEWPORT_XPS_GROUP_SET_POSITION:

			mx_status = mxd_newport_xps_home_or_set_position(
					motor, newport_xps_motor, newport_xps );
			break;

		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized command type %lu for motor '%s'.",
				newport_xps_motor->command_type,
				newport_xps_motor->record->name );
			break;
		}

		newport_xps_motor->move_in_progress = FALSE;

#if MXD_NEWPORT_XPS_MOTOR_MOVE_THREAD_DEBUG
		MX_DEBUG(-2,("%s: '%s' command complete",
			fname, newport_xps_motor->record->name));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_newport_xps_send_command_to_move_thread(
			MX_NEWPORT_XPS_MOTOR *newport_xps_motor,
			unsigned long command_type,
			double destination )
{
	static const char fname[] =
		"mxd_newport_xps_send_command_to_move_thread()";

	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( newport_xps_motor == (MX_NEWPORT_XPS_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT_XPS_MOTOR pointer passed was NULL." );
	}

	/* Prepare to tell the "move" thread to start a command. */

	mx_status_code = mx_mutex_lock( newport_xps_motor->move_thread_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the move thread mutex for "
		"XPS motor '%s' failed.",
			newport_xps_motor->record->name );
	}

	if ( newport_xps_motor->move_in_progress ) {
		(void) mx_mutex_unlock( newport_xps_motor->move_thread_mutex );

		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"A move was requested for Newport XPS motor '%s', but "
		"the motor is already moving.  You should either wait for "
		"the current move to complete or abort the current move.",
			newport_xps_motor->record->name );
	}

	newport_xps_motor->command_type = command_type;

	newport_xps_motor->command_destination = destination;

	newport_xps_motor->move_in_progress = TRUE;

	mx_status_code = mx_mutex_unlock( newport_xps_motor->move_thread_mutex);

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock the move thread mutex for "
		"XPS motor '%s' failed.",
			newport_xps_motor->record->name );
	}

	/* Tell the "move" thread to start the command. */

	mx_status = mx_condition_variable_signal(
				newport_xps_motor->move_thread_cv );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_newport_xps_motor_pco_set_config_value( MX_MOTOR *motor,
					MX_NEWPORT_XPS_MOTOR *newport_xps_motor,
					MX_NEWPORT_XPS *newport_xps,
					char *config_name,
					char *config_value )
{
	static const char fname[] =
		"mxd_newport_xps_motor_pco_set_config_value()";

	char *config_value_copy;
	int argc;
	char **argv;
	int xps_status;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

#if 0
	MX_DEBUG(-2,("%s: motor '%s', config_name = '%s', config_value = '%s'",
		fname, motor->record->name, config_name, config_value ));
#endif

	config_value_copy = strdup( config_value );

	if ( config_value_copy == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt by record '%s' to make a copy "
		"of string '%s' ran out of memory.",
			motor->record->name,
			config_value );
	}

	mx_string_split( config_value_copy, " ,", &argc, &argv );

	config_name = newport_xps_motor->pco_config_name;

	/* Disable any previously existing PCO configuration. */

	xps_status = PositionerPositionCompareDisable(
			newport_xps->socket_id,
			newport_xps_motor->positioner_name );

	if ( xps_status != SUCCESS ) {
		mx_free( argv ); mx_free( config_value_copy );

		return mxi_newport_xps_error(
			"PositionerPositionCompareDisable()",
			newport_xps_motor->record->name,
			newport_xps->socket_id,
			xps_status );

		motor->window_is_available = FALSE;
		motor->use_window = FALSE;
		motor->window[0] = 0.0;
		motor->window[1] = 0.0;
	}

	/* Now setup the new PCO configuration. */

	if ( mx_strcasecmp( "disable", config_name ) == 0 ) {
		/* We just disabled the PCO a few lines ago, so there
		 * is no need to do it a second time.
		 */

		motor->window_is_available = FALSE;
		motor->use_window = FALSE;
		motor->window[0] = 0.0;
		motor->window[1] = 0.0;
	} else
	if ( mx_strcasecmp( "distance_spaced_pulses", config_name ) == 0 ) {
		double minimum_position, maximum_position, step_size;

		if ( argc < 3 ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"distance_spaced_pulses mode for variable '%s' has "
			"fewer than 3 arguments in its configuration value "
			"string '%s'.",
				motor->record->name, config_value );
		}

		minimum_position = atof( argv[0] );
		maximum_position = atof( argv[1] );
		step_size        = atof( argv[2] );

		xps_status = PositionerPositionCompareSet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					minimum_position,
					maximum_position,
					step_size );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerPositionCompareSet()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}

		xps_status = PositionerPositionCompareEnable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerPositionCompareEnable()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}
	} else
	if ( mx_strcasecmp( "time_spaced_pulses", config_name ) == 0 ) {
		double minimum_position, maximum_position, time_interval;

		if ( argc < 3 ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"time_spaced_pulses mode for variable '%s' has "
			"fewer than 3 arguments in its configuration value "
			"string '%s'.",
				motor->record->name, config_value );
		}

		minimum_position = atof( argv[0] );
		maximum_position = atof( argv[1] );
		time_interval    = atof( argv[2] );

		xps_status = PositionerTimeFlasherSet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					minimum_position,
					maximum_position,
					time_interval );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerTimeFlasherSet()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}

		xps_status = PositionerTimeFlasherEnable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerTimeFlasherEnable()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}
	} else
	if ( mx_strcasecmp( "aquadb_always_enable", config_name ) == 0 ) {

		motor->window_is_available = FALSE;
		motor->use_window = FALSE;
		motor->window[0] = 0.0;
		motor->window[1] = 0.0;

		xps_status = PositionerPositionCompareAquadBAlwaysEnable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerPositionCompareAquadBAlwaysEnable()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}
	} else
	if ( mx_strcasecmp( "aquadb_windowed", config_name ) == 0 ) {
		double window_low, window_high;

		if ( argc < 2 ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"aquadb_windowed mode for variable '%s' has fewer "
			"than 3 arguments in its configuration value "
			"string '%s'.",
				motor->record->name, config_value );
		}

		window_low  = atof( argv[0] );
		window_high = atof( argv[1] );

#if 0
		MX_DEBUG(-2,("%s: '%s' aquadb_windowed = %f, %f",
		    fname, motor->record->name, window_low, window_high));
#endif

		/* Set the window. */

		xps_status = PositionerPositionCompareAquadBWindowedSet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					window_low, window_high );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerPositionCompareAquadBWindowedSet()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}

		/* Turn position compare on. */

		xps_status = PositionerPositionCompareEnable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				"PositionerPositionCompareEnable()",
				newport_xps_motor->record->name,
				newport_xps->socket_id,
				xps_status );
		}

#if 0
		{
		  int enabled;

		  /* Verify that the window was set correctly. */

		  window_low = window_high = 0;

		  xps_status = PositionerPositionCompareAquadBWindowedGet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&window_low, &window_high,
					&enabled );

		  if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( config_value_copy );

			return mxi_newport_xps_error(
				newport_xps->socket_id,
				"PositionerPositionCompareAquadBWindowedSet()",
				xps_status );
		  }

		  MX_DEBUG(-2,("%s: '%s' aquadb_windowed values read = %f, %f",
		    fname, motor->record->name, window_low, window_high));
		}
#endif

		motor->window_is_available = TRUE;
		motor->use_window = TRUE;

		motor->window[0] = window_low;
		motor->window[1] = window_high;
	} else {
		mx_free( argv ); mx_free( config_value_copy );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Motor '%s' has a config name '%s' that is not recognized.",
			motor->record->name, config_name );
	}

	return mx_status;
}


/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_newport_xps_create_record_structures()";

	MX_MOTOR *motor;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_MOTOR structure." );
	}

	newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *)
			malloc( sizeof(MX_NEWPORT_XPS_MOTOR) );

	if ( newport_xps_motor == (MX_NEWPORT_XPS_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_NEWPORT_XPS_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = newport_xps_motor;
	record->class_specific_function_list =
				&mxd_newport_xps_motor_function_list;

	motor->record = record;
	newport_xps_motor->record = record;

	newport_xps_motor->group_status = 0;
	newport_xps_motor->group_status_string[0] = '\0';

	newport_xps_motor->array_index = -1;

	newport_xps_motor->set_position_sleep_ms = 500;   /* in milliseconds */

	/* A NEWPORT_XPS_MOTOR is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	/* The XPS motor reports acceleration in units/sec**2 */

	motor->acceleration_type = MXF_MTR_ACCEL_RATE;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_newport_xps_open()";

	MX_MOTOR *motor = NULL;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	char *ptr = NULL;
	long i, j, num_motors_in_group;
	const char *our_group_name = NULL;
	MX_NEWPORT_XPS_MOTOR *other_newport_xps_motor = NULL;
	int xps_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The group name for the motor consists of everything before
	 * the period character '.' in the name.
	 */

	strlcpy( newport_xps_motor->group_name,
		newport_xps_motor->positioner_name,
		sizeof(newport_xps_motor->group_name) );

	ptr = strrchr( newport_xps_motor->group_name, '.' );

	if ( ptr != NULL ) {
		*ptr = '\0';
	}

	/* Verify that the motor is present by asking for its status. */

	mx_status = mxd_newport_xps_get_status( motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We need a list of the motors that are in the same group as 
	 * this motor.  We will lists the current motor as part of that
	 * list for convenience sake.
	 */

	/* First find out how many motors are in our group. */

	our_group_name = newport_xps_motor->group_name;

	num_motors_in_group = 0;

	for ( i = 0; i < newport_xps->num_motors; i++ ) {
		other_newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *)
			newport_xps->motor_record_array[i]->record_type_struct;

		if ( strcmp( our_group_name,
		    other_newport_xps_motor->group_name ) == 0 )
		{
			num_motors_in_group++;
		}
	}

#if MXD_NEWPORT_XPS_MOTOR_DEBUG_MOTOR_GROUPS
	MX_DEBUG(-2,("%s: motor '%s', group '%s', num_motors_in_group = %ld",
		fname, record->name,
		newport_xps_motor->group_name,
		num_motors_in_group));
#endif

	newport_xps_motor->num_motors_in_group = num_motors_in_group;

	/* Now go back and make an array of these motors for future
	 * uses in places like 'set_position'.
	 */

	newport_xps_motor->motor_records_in_group = (MX_RECORD **)
		calloc( num_motors_in_group, sizeof(MX_RECORD *) );

	if ( newport_xps_motor->motor_records_in_group == (MX_RECORD **) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of group motor record pointers for motor '%s'.",
			newport_xps_motor->num_motors_in_group,
			record->name );
	}

	newport_xps_motor->old_motor_positions_in_group = (double *)
		calloc( num_motors_in_group, sizeof(MX_RECORD *) );

	if ( newport_xps_motor->old_motor_positions_in_group == (double *)NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of group old motor positions for motor '%s'.",
			newport_xps_motor->num_motors_in_group,
			record->name );
	}

	/*---*/

	j = 0;

	for ( i = 0; i < newport_xps->num_motors; i++ ) {
		other_newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *)
			newport_xps->motor_record_array[i]->record_type_struct;

		if ( strcmp( our_group_name,
		    other_newport_xps_motor->group_name ) == 0 )
		{
			newport_xps_motor->motor_records_in_group[j]
				= newport_xps->motor_record_array[i];

#if MXD_NEWPORT_XPS_MOTOR_DEBUG_MOTOR_GROUPS
			MX_DEBUG(-2,
			("%s: motor '%s', motor_records_in_group[%ld] = '%s'",
			   fname, record->name, j,
			   newport_xps_motor->motor_records_in_group[j]->name));
#endif
			j++;
		}
	}

	/*** Create move thread ***/

	/* Move commands _block_ until the move is complete, so we need to 
	 * create a second thread with its own socket connection to the
	 * XPS controller.  This allows us to put blocking "move" commands
	 * into this separate "move thread", so that other communication
	 * with the XPS controller does not block until the move is complete.
	 */

	/* First, we create the additional socket connection to the XPS.
	 * This is the operation that is probably most likely to fail,
	 * so we do it first.
	 */

	newport_xps_motor->move_thread_socket_id
				= TCP_ConnectToServer(
					newport_xps->hostname,
					newport_xps->port_number,
					newport_xps->timeout );

#if MXD_NEWPORT_XPS_MOTOR_MOVE_THREAD_DEBUG
	MX_DEBUG(-2,("%s: newport_xps_motor->move_thread_socket_id = %d",
			fname, newport_xps_motor->move_thread_socket_id));
#endif

	if ( newport_xps_motor->move_thread_socket_id < 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to make another connection to Newport XPS "
		"controller '%s' for move commands failed.",
			record->name );
	}

	/* Set XPS socket timeouts. */

	mx_status = mxd_newport_xps_process_function( record,
			mx_get_record_field( record, "socket_send_timeout" ),
			NULL, MX_PROCESS_PUT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_newport_xps_process_function( record,
			mx_get_record_field( record, "socket_receive_timeout" ),
			NULL, MX_PROCESS_PUT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* Login to the Newport XPS controller (on the additional socket). */

	xps_status = Login( newport_xps_motor->move_thread_socket_id,
				newport_xps->username,
				newport_xps->password );
#else
	xps_status = 0;
#endif

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error(
				"Login() (for move thread)",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
	}

	/* Set 'move_in_progress' to FALSE so that the thread will not
	 * prematurely try to send a move command.
	 */

	newport_xps_motor->move_in_progress = FALSE;

	newport_xps_motor->command_type = 0;

	/* Create the mutex and condition variable that the thread will
	 * use to synchronize with the main thread.
	 */

	mx_status = mx_mutex_create( &(newport_xps_motor->move_thread_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(newport_xps_motor->move_thread_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* Set the Position Compare Output (PCO) configuration. */

	mx_status = mxd_newport_xps_motor_pco_set_config_value(
			motor, newport_xps_motor, newport_xps,
			newport_xps_motor->pco_config_name,
			newport_xps_motor->pco_config_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Start the move thread. */

	mx_status = mx_thread_create( &(newport_xps_motor->move_thread),
					mxd_newport_xps_move_thread, motor );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_newport_xps_resynchronize()";

	MX_MOTOR *motor = NULL;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	motor = (MX_MOTOR *) record->record_class_struct;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xps_status = GroupInitialize( newport_xps->socket_id,
					newport_xps_motor->group_name );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error(
				"GroupInitialize()",
				newport_xps_motor->record->name,
				newport_xps_motor->move_thread_socket_id,
				xps_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_NEWPORT_XPS_DRIVER_STATUS:
		case MXLV_NEWPORT_XPS_GROUP_STATUS:
		case MXLV_NEWPORT_XPS_GROUP_STATUS_STRING:
		case MXLV_NEWPORT_XPS_HARDWARE_STATUS:
		case MXLV_NEWPORT_XPS_MOTOR_SOCKET_RECEIVE_TIMEOUT:
		case MXLV_NEWPORT_XPS_MOTOR_SOCKET_SEND_TIMEOUT:
		case MXLV_NEWPORT_XPS_PCO_CONFIG_NAME:
		case MXLV_NEWPORT_XPS_PCO_CONFIG_VALUE:
		case MXLV_NEWPORT_XPS_POSITIONER_ERROR:
			record_field->process_function
					= mxd_newport_xps_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxd_newport_xps_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_newport_xps_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MOTOR *motor;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor;
	MX_RECORD *newport_xps_record;
	MX_NEWPORT_XPS *newport_xps;
	struct timeval timeout;
	socklen_t timeval_size = sizeof(struct timeval);
	int os_status, saved_errno;
	char error_message[200];
	int driver_status, group_status, hardware_status, positioner_error;
	int xps_status;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	motor = (MX_MOTOR *) record->record_class_struct;
	newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *) record->record_type_struct;
	newport_xps_record = newport_xps_motor->newport_xps_record;
	newport_xps = (MX_NEWPORT_XPS *) newport_xps_record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_NEWPORT_XPS_DRIVER_STATUS:
			xps_status = PositionerDriverStatusGet(
				newport_xps->socket_id,
				newport_xps_motor->positioner_name,
				&driver_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"PositionerDriverStatusGet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps_motor->driver_status = driver_status;
			break;
		case MXLV_NEWPORT_XPS_GROUP_STATUS:
			xps_status = GroupStatusGet( newport_xps->socket_id,
						newport_xps_motor->group_name,
						&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"GroupStatusGet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps_motor->group_status = group_status;
			break;
		case MXLV_NEWPORT_XPS_GROUP_STATUS_STRING:
			xps_status = GroupStatusGet( newport_xps->socket_id,
						newport_xps_motor->group_name,
						&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"GroupStatusGet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps_motor->group_status = group_status;

			xps_status = GroupStatusStringGet(
				newport_xps->socket_id,
				group_status,
				newport_xps_motor->group_status_string );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"GroupStatusStringGet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_HARDWARE_STATUS:
			xps_status = PositionerHardwareStatusGet(
				newport_xps->socket_id,
				newport_xps_motor->positioner_name,
				&hardware_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"PositionerHardwareStatusGet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps_motor->hardware_status = hardware_status;
			break;
		case MXLV_NEWPORT_XPS_POSITIONER_ERROR:
			/* A Read() does not clear the error. */

			xps_status = PositionerErrorRead(
				newport_xps->socket_id,
				newport_xps_motor->positioner_name,
				&positioner_error );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"PositionerErrorRead()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps_motor->positioner_error = positioner_error;
			break;
		case MXLV_NEWPORT_XPS_MOTOR_SOCKET_RECEIVE_TIMEOUT:
			os_status = getsockopt(
				newport_xps_motor->move_thread_socket_id,
					SOL_SOCKET, SO_RCVTIMEO, 
					&timeout, &timeval_size );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to get SO_SNDTIMEO for the "
				"move thread socket %d of Newport XPS motor "
				"'%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps_motor->move_thread_socket_id,
				    newport_xps_motor->record->name,
				    saved_errno, mx_strerror( saved_errno,
				    error_message, sizeof(error_message) ) );

			}

			newport_xps->socket_receive_timeout = 
				timeout.tv_sec + 1.0e-6 * timeout.tv_usec;
			break;
		case MXLV_NEWPORT_XPS_MOTOR_SOCKET_SEND_TIMEOUT:
			os_status = getsockopt(
				newport_xps_motor->move_thread_socket_id,
					SOL_SOCKET, SO_SNDTIMEO, 
					&timeout, &timeval_size );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to get SO_SNDTIMEO for the "
				"move thread socket %d of Newport XPS motor "
				"'%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps_motor->move_thread_socket_id,
				    newport_xps_motor->record->name,
				    saved_errno, mx_strerror( saved_errno,
				    error_message, sizeof(error_message) ) );

			}

			newport_xps->socket_send_timeout = 
				timeout.tv_sec + 1.0e-6 * timeout.tv_usec;
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_NEWPORT_XPS_POSITIONER_ERROR:
			/* Clear the error by Get()-ing it. */

			xps_status = PositionerErrorGet(
				newport_xps->socket_id,
				newport_xps_motor->positioner_name,
				&positioner_error );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"PositionerErrorRead()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps_motor->positioner_error = 0;
			break;

		case MXLV_NEWPORT_XPS_PCO_CONFIG_NAME:
			/* If 'pco_config_name' is changed, we must overwrite
			 * the existing contents of 'pco_config_value' so that
		`	 * we do not attempt to use a config value string with
		`	 * a configuration type that is not compatible.
			 */

			strlcpy( newport_xps_motor->pco_config_value, "",
				sizeof(newport_xps_motor->pco_config_value) );
			break;

		case MXLV_NEWPORT_XPS_PCO_CONFIG_VALUE:
			mx_status = mxd_newport_xps_motor_pco_set_config_value(
					motor, newport_xps_motor, newport_xps,
					newport_xps_motor->pco_config_name,
					newport_xps_motor->pco_config_value );
			break;

		case MXLV_NEWPORT_XPS_SOCKET_RECEIVE_TIMEOUT:
			timeout.tv_sec =
				newport_xps_motor->socket_receive_timeout;
			timeout.tv_usec = 1000000.0 *
				( newport_xps_motor->socket_receive_timeout
				- timeout.tv_sec );

			os_status = setsockopt(
				newport_xps_motor->move_thread_socket_id,
					SOL_SOCKET, SO_RCVTIMEO, 
					&timeout, sizeof(timeout) );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to set SO_SNDTIMEO for the "
				"move thread socket %d of Newport XPS motor "
				"'%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps_motor->move_thread_socket_id,
				    newport_xps_motor->record->name,
				    saved_errno, mx_strerror( saved_errno,
				    error_message, sizeof(error_message) ) );

			}
			break;
		case MXLV_NEWPORT_XPS_SOCKET_SEND_TIMEOUT:
			timeout.tv_sec =
				newport_xps_motor->socket_send_timeout;
			timeout.tv_usec = 1000000.0 *
				( newport_xps_motor->socket_send_timeout
				- timeout.tv_sec );

			os_status = setsockopt(
				newport_xps_motor->move_thread_socket_id,
					SOL_SOCKET, SO_SNDTIMEO, 
					&timeout, sizeof(timeout) );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to set SO_SNDTIMEO for the "
				"move thread socket %d of Newport XPS motor "
				"'%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps_motor->move_thread_socket_id,
				    newport_xps_motor->record->name,
				    saved_errno, mx_strerror( saved_errno,
				    error_message, sizeof(error_message) ) );

			}
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

/* ============ Motor specific functions ============ */

MX_EXPORT mx_status_type
mxd_newport_xps_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_move_absolute()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_newport_xps_send_command_to_move_thread(
			newport_xps_motor,
			MXT_NEWPORT_XPS_GROUP_MOVE_ABSOLUTE,
			motor->raw_destination.analog );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_get_position()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	double raw_encoder_position;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xps_status = GroupPositionCurrentGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					1, &raw_encoder_position );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( "GroupPositionCurrentGet()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
	}

#if MXD_NEWPORT_XPS_MOTOR_POSITION_DEBUG
	{
	    double raw_setpoint_position, raw_following_error;

	    xps_status = GroupPositionSetpointGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					1, &raw_setpoint_position );

	    if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupPositionSetpointGet()",
						xps_status );
	    }

	    xps_status = GroupCurrentFollowingErrorGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					1, &raw_following_error );

	    if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
					"GroupCurrentFollowingErrorGet()",
					xps_status );
	    }

	    MX_DEBUG(-2,("%s: encoder = %g, setpoint = %g, error = %g",
		fname, raw_encoder_position, raw_setpoint_position,
		raw_following_error));
	}
#endif

	motor->raw_position.analog = raw_encoder_position;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_set_position()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_newport_xps_send_command_to_move_thread(
			newport_xps_motor,
			MXT_NEWPORT_XPS_GROUP_SET_POSITION,
			motor->raw_set_position.analog );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_soft_abort()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xps_status = GroupMoveAbort( newport_xps->socket_id,
				newport_xps_motor->group_name );

	switch( xps_status ) {
	case SUCCESS:
	case ERR_NOT_ALLOWED_ACTION:   /* Abort while not moving. */
		break;
	default:
		return mxi_newport_xps_error( "GroupMoveAbort()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_immediate_abort()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xps_status = GroupKill( newport_xps->socket_id,
				newport_xps_motor->group_name );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( "GroupKill()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_raw_home_command( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_raw_home_command()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_newport_xps_send_command_to_move_thread(
			newport_xps_motor,
			MXT_NEWPORT_XPS_GROUP_HOME_SEARCH,
			0.0 );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_get_parameter()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_ACCELERATION_TYPE:
		motor->acceleration_type = MXF_MTR_ACCEL_RATE;
		break;
	case MXLV_MTR_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* Acceleration parameter 0 = acceleration (units/sec**2)
		 * Acceleration parameter 1 = minimum jerk time (sec)
		 * Acceleration parameter 2 = maximum jerk time (sec)
		 */

		xps_status = PositionerSGammaParametersGet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&(motor->raw_speed),
				&(motor->raw_acceleration_parameters[0]),
				&(motor->raw_acceleration_parameters[1]),
				&(motor->raw_acceleration_parameters[2]) );

		if ( xps_status != SUCCESS ) {
			return mxi_newport_xps_error(
					"PositionerSGammaParametersGet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
		}
		break;

	default:
		return mx_motor_default_get_parameter_handler( motor );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_set_parameter()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for motor '%s' for parameter type '%s' (%ld).",
		fname, motor->record->name,
		mx_get_field_label_string( motor->record,
			motor->parameter_type ),
		motor->parameter_type));
#endif

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
	case MXLV_MTR_RAW_ACCELERATION_PARAMETERS:
		/* Acceleration parameter 0 = acceleration (units/sec**2)
		 * Acceleration parameter 1 = minimum jerk time (sec)
		 * Acceleration parameter 2 = maximum jerk time (sec)
		 */

		xps_status = PositionerSGammaParametersSet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					motor->raw_speed,
					motor->raw_acceleration_parameters[0],
					motor->raw_acceleration_parameters[1],
					motor->raw_acceleration_parameters[2] );

		if ( xps_status != SUCCESS ) {
			return mxi_newport_xps_error(
					"PositionerSGammaParametersSet()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
		}
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			xps_status = GroupMotionEnable( newport_xps->socket_id,
						newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"GroupMotionEnable()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}
		} else {
			xps_status = GroupMotionDisable( newport_xps->socket_id,
						newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"GroupMotionDisable()",
					newport_xps_motor->record->name,
					newport_xps->socket_id,
					xps_status );
			}
		}
		break;

	case MXLV_MTR_WINDOW:
		snprintf( newport_xps_motor->pco_config_value,
			sizeof(newport_xps_motor->pco_config_value),
			"%g %g", motor->window[0], motor->window[1] );

		mx_status = mxd_newport_xps_motor_pco_set_config_value(
				motor, newport_xps_motor, newport_xps,
				newport_xps_motor->pco_config_name,
				newport_xps_motor->pco_config_value );
		break;

	default:
		return mx_motor_default_set_parameter_handler( motor );
	}

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_get_status()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status, int_value;
	unsigned long hw_status, gs;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->status = 0;

	/* FIXME: Reading move_in_progress from the main thread
	 * without a mutex may have concurrency issues.
	 */

	if ( newport_xps_motor->move_in_progress ) {
		motor->status |= MXSF_MTR_IS_BUSY;
	}

	/*---*/

	xps_status = PositionerErrorRead( newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&int_value );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( "PositionerErrorRead()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
	}

	newport_xps_motor->positioner_error = int_value;

	/*---*/

	xps_status = PositionerHardwareStatusGet( newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&int_value );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( "PositionerHardwareStatusGet()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
	}

	newport_xps_motor->hardware_status = int_value;

	/* Translate the hardware status bits into MX status bits. */

	hw_status = newport_xps_motor->hardware_status;

	if ( hw_status & 0x100 ) {
		motor->status |= MXSF_MTR_NEGATIVE_LIMIT_HIT;
	} else
	if ( hw_status & 0x200 ) {
		motor->status |= MXSF_MTR_POSITIVE_LIMIT_HIT;
	} else
	if ( hw_status & 0x400 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;
	} else
	if ( hw_status & 0x800 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;
	} else
	if ( hw_status & 0x1000 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;
	} else
	if ( hw_status & 0x2000 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;
	} else
	if ( hw_status & 0x10000 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;
	} else
	if ( hw_status & 0x20000 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;
	} else
	if ( hw_status & 0x100000 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	} else
	if ( hw_status & 0x200000 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;
	}

	/*---*/

	xps_status = PositionerDriverStatusGet( newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&int_value );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( "PositionerDriverStatusGet()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
	}

	newport_xps_motor->hardware_status = int_value;

	/*---*/

	xps_status = GroupStatusGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					&int_value );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( "GroupStatusGet()",
						newport_xps_motor->record->name,
						newport_xps->socket_id,
						xps_status );
	}

	newport_xps_motor->group_status = int_value;

	/* Interpret the group status.
	 *
	 * It is really annoying to have to read tea leaves in this fashion,
	 * rather than having a simple, clear "home search succeeded" bit.
	 *
	 * If we end up needing to handle more than just home_search_succeeded,
	 * then we should probably change to a switch() statement.
	 */

	gs = newport_xps_motor->group_status;

	if ( gs <= 9 ) {	/* Various not-initialized states */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs <= 39 ) {	/* Various ready or disabled states */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else
	if ( gs == 40 ) {	/* Emergency Braking */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 41 ) {	/* Motor Initialization state */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 42 ) {	/* Not Referenced state */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 43 ) {	/* Homing state (not yet finished) */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs <= 48 ) {	/* Various motion states */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else
	if ( gs == 49 ) {    /* Analog Interpolated Encoder Calibrating state */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 50 ) {	/* Another Not Initialized state */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 51 ) {	/* Spinning state */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else
	if ( gs <= 62 ) {	/* (undefined states) */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs <= 67 ) {   /* Various referenced and not initializing states */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 68 ) {	/* Auto-tuning state */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else
	if ( gs == 69 ) {	/* Scaling calibration state */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 70 ) {	/* Ready state from auto-tuning */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else
	if ( gs <= 72 ) {	/* More Not Initialized states */
		newport_xps_motor->home_search_succeeded = FALSE;
	} else
	if ( gs == 73 ) {	/* Excitation signal generation state */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else
	if ( gs <= 77 ) {	/* Various disabled and ready states */
		newport_xps_motor->home_search_succeeded = TRUE;
	} else {
		/* Anything above 77 */
		newport_xps_motor->home_search_succeeded = FALSE;
	}

	/*---*/

	if ( newport_xps_motor->home_search_succeeded ) {
		motor->status |= MXSF_MTR_HOME_SEARCH_SUCCEEDED;
	}

	/*---*/

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s', status = %#lx",
		fname, motor->record->name, motor->status));
	MX_DEBUG(-2,("%s:     positioner_error = %#lx, hardware_status = %#lx",
		fname, newport_xps_motor->positioner_error,
		newport_xps_motor->hardware_status));
	MX_DEBUG(-2,("%s:     driver_status = %#lx, group_status = %lu",
		fname, newport_xps_motor->driver_status,
		newport_xps_motor->group_status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

