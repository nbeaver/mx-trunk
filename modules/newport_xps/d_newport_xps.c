/*
 * Name:    d_newport_xps.c
 *
 * Purpose: MX driver for the Stanford Research Systems NEWPORT_XPS_MOTOR
 *          Analog PID Controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NEWPORT_XPS_MOTOR_DEBUG			FALSE

#define MXD_NEWPORT_XPS_MOTOR_POSITION_DEBUG		FALSE

#define MXD_NEWPORT_XPS_MOTOR_MOVE_THREAD_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

static mx_status_type
mxd_newport_xps_move_thread( MX_THREAD *thread, void *thread_argument )
{
	static const char fname[] = "mxd_newport_xps_move_thread()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_MOTOR *motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status, group_status;
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
				(void) mxi_newport_xps_error(
				newport_xps_motor->move_thread_socket_id,
					"GroupMoveAbsolute()",
					xps_status );
			}
			break;

		case MXT_NEWPORT_XPS_GROUP_HOME_SEARCH:

			newport_xps_motor->home_search_succeeded = FALSE;

			/* Check to see if the motor is in the Ready state. */

			xps_status = GroupStatusGet( newport_xps->socket_id,
						newport_xps_motor->group_name,
						&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"GroupStatusGet() (Home #1)",
					xps_status );
			}

			MX_DEBUG(-2,("Home search '%s': group_status #1 = %d",
				motor->record->name, group_status ));

			if ( (group_status >= 10) && (group_status <= 18) )
			{
				/* We cannot start a home search if the group
				 * is in the ready state.  If so, then we
				 * must kill the group to put it into the
				 * "NOT INITIALIZED" state.
				 */

				MX_DEBUG(-2,
				("Home search '%s': Calling GroupKill()",
					motor->record->name ));

				xps_status = GroupKill( newport_xps->socket_id,
						newport_xps_motor->group_name );

				if ( xps_status != SUCCESS ) {
					return mxi_newport_xps_error(
						newport_xps->socket_id,
						"GroupKill() (Home)",
						xps_status );
				}
			}

			/* We must make sure that the axis is now initialized.
			 * It is possible for an Initialized axis to get here,
			 * so we must check to see if it is already initialized.
			 */

			xps_status = GroupStatusGet( newport_xps->socket_id,
						newport_xps_motor->group_name,
						&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"GroupStatusGet() (Home #2)",
					xps_status );
			}

			MX_DEBUG(-2,("Home search '%s': group_status #2 = %d",
				motor->record->name, group_status ));

			if ( ((group_status >= 0) && (group_status <= 9))
			  || (group_status == 50) || (group_status == 63) )
			{
				/* The axis is not initialized, so we must
				 * initialize it.
				 */

				MX_DEBUG(-2,
				("Home search '%s': Calling GroupInitialize()",
					motor->record->name ));

				xps_status = GroupInitialize(
					newport_xps->socket_id,
					newport_xps_motor->group_name );

				if ( xps_status != SUCCESS ) {
					return mxi_newport_xps_error(
						newport_xps->socket_id,
						"GroupInitialize() (Home)",
						xps_status );
				}
			}

			/* Now start the actual home search.  This thread
			 * will block until the home search completes.
			 */

			MX_DEBUG(-2,
			("Home search '%s': Calling GroupHomeSearch()",
				motor->record->name ));

			xps_status = GroupHomeSearch(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

			/* When we get here, the home search has
			 * completed and/or aborted.  Check to see
			 * what happened.
			 */

			MX_DEBUG(-2,
			("Home search '%s': GroupHomeSearch() status = %d",
				motor->record->name, xps_status ));

			if ( xps_status != SUCCESS ) {
				newport_xps_motor->home_search_succeeded
							= FALSE;

				(void) mxi_newport_xps_error(
				newport_xps_motor->move_thread_socket_id,
					"GroupHomeSearch()",
					xps_status );
			}

			/* If all went well, we should be in group state 11
			 * (Ready state from homing).  If we are _not_ in
			 * group state 11, then something went wrong and
			 * we mark the home search as having failed.
			 */

			xps_status = GroupStatusGet( newport_xps->socket_id,
						newport_xps_motor->group_name,
						&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"GroupStatusGet() (Home #3)",
					xps_status );
			}

			MX_DEBUG(-2,("Home search '%s': group_status #3 = %d",
				motor->record->name, group_status ));

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
	char *ptr;
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
				newport_xps_motor->move_thread_socket_id,
				"Login() (for move thread)",
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
			newport_xps_motor->move_thread_socket_id,
				"GroupInitialize()",
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
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxd_newport_xps_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor;
	MX_RECORD *newport_xps_record;
	MX_NEWPORT_XPS *newport_xps;
	int driver_status, group_status, hardware_status, positioner_error;
	int xps_status;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
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
					newport_xps->socket_id,
					"PositionerDriverStatusGet()",
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
					newport_xps->socket_id,
					"GroupStatusGet()",
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
					newport_xps->socket_id,
					"GroupStatusGet()",
					xps_status );
			}

			newport_xps_motor->group_status = group_status;

			xps_status = GroupStatusStringGet(
				newport_xps->socket_id,
				group_status,
				newport_xps_motor->group_status_string );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"GroupStatusStringGet()",
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
					newport_xps->socket_id,
					"PositionerHardwareStatusGet()",
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
					newport_xps->socket_id,
					"PositionerErrorRead()",
					xps_status );
			}

			newport_xps_motor->positioner_error = positioner_error;
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
					newport_xps->socket_id,
					"PositionerErrorRead()",
					xps_status );
			}

			newport_xps_motor->positioner_error = 0;
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
	double new_controller_destination;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Newport XPS does not provide a way to redefine the position
	 * inside the controller itself, so we must emulate this in the
	 * MX driver itself.
	 */

	new_controller_destination = motor->raw_destination.analog
			- newport_xps_motor->internal_position_offset;

	mx_status = mxd_newport_xps_send_command_to_move_thread(
				newport_xps_motor,
				MXT_NEWPORT_XPS_GROUP_MOVE_ABSOLUTE,
				new_controller_destination );

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
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupPositionCurrentGet()",
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

	/* The Newport XPS does not provide a way to redefine the position
	 * inside the controller itself, so we must emulate this in the
	 * MX driver itself.
	 */

	motor->raw_position.analog =
	    raw_encoder_position + newport_xps_motor->internal_position_offset;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_newport_xps_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_newport_xps_set_position()";

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_NEWPORT_XPS *newport_xps = NULL;
	double old_user_position, new_user_position, raw_difference;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Newport XPS does not provide a way to redefine the position
	 * inside the controller itself, so we must emulate this in the
	 * MX driver itself.
	 */

	new_user_position = motor->set_position;

	mx_status = mx_motor_get_position( motor->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	old_user_position = motor->position;

	raw_difference = mx_divide_safely(
				new_user_position - old_user_position,
				motor->scale );

	newport_xps_motor->internal_position_offset += raw_difference;

	return MX_SUCCESSFUL_RESULT;
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
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupMoveAbort()",
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
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupKill()",
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
			return mxi_newport_xps_error( newport_xps->socket_id,
					"PositionerSGammaParametersGet()",
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
			return mxi_newport_xps_error( newport_xps->socket_id,
					"PositionerSGammaParametersGet()",
					xps_status );
		}
		break;

	case MXLV_MTR_AXIS_ENABLE:
		if ( motor->axis_enable ) {
			xps_status = GroupMotionEnable( newport_xps->socket_id,
						newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"GroupMotionEnable()",
					xps_status );
			}
		} else {
			xps_status = GroupMotionDisable( newport_xps->socket_id,
						newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"GroupMotionDisable()",
					xps_status );
			}
		}
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
		return mxi_newport_xps_error( newport_xps->socket_id,
						"PositionerErrorRead()",
						xps_status );
	}

	newport_xps_motor->positioner_error = int_value;

	/*---*/

	xps_status = PositionerHardwareStatusGet( newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&int_value );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"PositionerHardwareStatusGet()",
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
		return mxi_newport_xps_error( newport_xps->socket_id,
						"PositionerDriverStatusGet()",
						xps_status );
	}

	newport_xps_motor->hardware_status = int_value;

	/*---*/

	xps_status = GroupStatusGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					&int_value );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupStatusGet()",
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

