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
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NEWPORT_XPS_MOTOR_DEBUG			FALSE

#define MXD_NEWPORT_XPS_MOTOR_MOVE_THREAD_DEBUG		TRUE

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
	NULL,
	mxd_newport_xps_soft_abort,
	NULL,
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

	MX_NEWPORT_XPS_MOTOR *newport_xps_motor;
	MX_MOTOR *motor;
	int xps_status;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( thread_argument == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The thread_argument pointer for this thread is NULL." );
	}

	newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *) thread_argument;

	if ( newport_xps_motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_NEWPORT_XPS_MOTOR ptr %p is NULL",
			newport_xps_motor );
	}

	motor = (MX_MOTOR *) newport_xps_motor->record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for motor '%s' is NULL.",
			newport_xps_motor->record->name );
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

			if ( xps_status != SUCCESS ) {
				(void) mxi_newport_xps_error(
				newport_xps_motor->move_thread_socket_id,
					"GroupMoveAbsolute()",
					xps_status );
			}
			break;

		case MXT_NEWPORT_XPS_GROUP_HOME_SEARCH:

			xps_status = GroupHomeSearch(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name );

			if ( xps_status != SUCCESS ) {
				(void) mxi_newport_xps_error(
				newport_xps_motor->move_thread_socket_id,
					"GroupHomeSearch()",
					xps_status );
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

	/* Login to the Newport XPS controller (on the additional socket). */

	xps_status = Login( newport_xps_motor->move_thread_socket_id,
				newport_xps->username,
				newport_xps->password );

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
					mxd_newport_xps_move_thread,
					newport_xps_motor );

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
		case MXLV_NEWPORT_XPS_GROUP_STATUS:
		case MXLV_NEWPORT_XPS_GROUP_STATUS_STRING:
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
	int xps_status, group_status;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	newport_xps_motor = (MX_NEWPORT_XPS_MOTOR *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_NEWPORT_XPS_GROUP_STATUS:
			xps_status = GroupStatusGet(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name,
				&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
				    newport_xps_motor->move_thread_socket_id,
					"GroupStatusGet()",
					xps_status );
			}

			newport_xps_motor->group_status = group_status;
			break;
		case MXLV_NEWPORT_XPS_GROUP_STATUS_STRING:
			xps_status = GroupStatusGet(
				newport_xps_motor->move_thread_socket_id,
				newport_xps_motor->group_name,
				&group_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
				    newport_xps_motor->move_thread_socket_id,
					"GroupStatusGet()",
					xps_status );
			}

			newport_xps_motor->group_status = group_status;

			xps_status = GroupStatusStringGet(
				newport_xps_motor->move_thread_socket_id,
				group_status,
				newport_xps_motor->group_status_string );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
				    newport_xps_motor->move_thread_socket_id,
					"GroupStatusStringGet()",
					xps_status );
			}
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
	double raw_position;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxd_newport_xps_get_pointers( motor, &newport_xps_motor,
							&newport_xps, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xps_status = GroupPositionCurrentGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					1, &raw_position );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupPositionCurrentGet()",
						xps_status );
	}

	motor->raw_position.analog = raw_position;

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

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupMoveAbort()",
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
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		break;

	case MXLV_MTR_EXTRA_GAIN:
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
		break;

	case MXLV_MTR_AXIS_ENABLE:
		break;

	case MXLV_MTR_PROPORTIONAL_GAIN:
		break;

	case MXLV_MTR_INTEGRAL_GAIN:
		break;

	case MXLV_MTR_DERIVATIVE_GAIN:
		break;

	case MXLV_MTR_EXTRA_GAIN:
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
	int group_status, xps_status;
	char group_status_string[300];
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

	xps_status = GroupStatusGet( newport_xps->socket_id,
					newport_xps_motor->group_name,
					&group_status );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupStatusGet()",
						xps_status );
	}

	xps_status = GroupStatusStringGet( newport_xps->socket_id,
					group_status,
					group_status_string );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"GroupStatusStringGet()",
						xps_status );
	}

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s', group_status = %d, '%s'",
		fname, motor->record->name,
		group_status, group_status_string ))
#endif

#if MXD_NEWPORT_XPS_MOTOR_DEBUG
	MX_DEBUG(-2,("%s: Motor '%s', status = %#lx",
		fname, motor->record->name, motor->status));
#endif

	return MX_SUCCESSFUL_RESULT;
}

