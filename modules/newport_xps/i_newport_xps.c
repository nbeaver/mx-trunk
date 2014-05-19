/*
 * Name:    i_newport_xps.c
 *
 * Purpose: MX driver for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NEWPORT_XPS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_newport_xps.h"

/* Vendor include files. */

#include "XPS_C8_drivers.h"
#include "XPS_C8_errors.h"

MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list = {
	NULL,
	mxi_newport_xps_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_newport_xps_open
};

MX_RECORD_FIELD_DEFAULTS mxi_newport_xps_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NEWPORT_XPS_STANDARD_FIELDS
};

long mxi_newport_xps_num_record_fields
		= sizeof( mxi_newport_xps_record_field_defaults )
			/ sizeof( mxi_newport_xps_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_newport_xps_rfield_def_ptr
			= &mxi_newport_xps_record_field_defaults[0];

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_newport_xps_create_record_structures()";

	MX_NEWPORT_XPS *newport_xps = NULL;

	/* Allocate memory for the necessary structures. */

	newport_xps = (MX_NEWPORT_XPS *) malloc( sizeof(MX_NEWPORT_XPS) );

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NEWPORT_XPS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = newport_xps;

	record->record_function_list = &mxi_newport_xps_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	newport_xps->record = record;

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxi_newport_xps_move_thread( MX_THREAD *thread, void *thread_argument )
{
	static const char fname[] = "mxi_newport_xps_move_thread()";

	MX_NEWPORT_XPS *newport_xps;
	char *type;
	int xps_status;
	unsigned long mx_status_code;
	mx_status_type mx_status;

	if ( thread_argument == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The thread_argument pointer for this thread is NULL." );
	}

	newport_xps = (MX_NEWPORT_XPS *) thread_argument;

	/* Lock the mutex in preparation for entering the thread's
	 * event handler loop.
	 */

	mx_status_code = mx_mutex_lock( newport_xps->move_thread_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock the move_thread_mutex for "
		"Newport XPS controller '%s' failed.",
			newport_xps->record->name );
	}

	while (TRUE) {

		/* Wait on the condition variable. */

		if ( newport_xps->move_in_progress == FALSE ) {
			mx_status = mx_condition_variable_wait(
					newport_xps->move_thread_cv,
					newport_xps->move_thread_mutex );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		type = newport_xps->command_type;

		MX_DEBUG(-2,("%s: executing command '%s'", fname, type ));

		if ( strcmp( type, "GroupMoveAbsolute" ) == 0 ) {
			xps_status = GroupMoveAbsolute(
					newport_xps->move_thread_socket_id,
					newport_xps->commanded_object_name,
					1,
					&(newport_xps->commanded_destination) );

			if ( xps_status != SUCCESS ) {
				(void) mxi_newport_xps_error( newport_xps,
						"GroupMoveAbsolute()",
						xps_status );
			}
		} else {
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unrecognized command type '%s' for controller '%s'.",
				type, newport_xps->record->name );
		}

		newport_xps->move_in_progress = FALSE;

		MX_DEBUG(-2,("%s: command complete", fname));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_newport_xps_open()";

	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;
	int controller_status_code;
	char controller_status_string[200];
	char firmware_version[200];
	char hardware_date_and_time[200];
	double elapsed_seconds_since_power_on;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	newport_xps = (MX_NEWPORT_XPS *) record->record_type_struct;

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NEWPORT_XPS pointer for record '%s' is NULL.",
			record->name );
	}

	MX_DEBUG(-2,("%s: '%s'", fname, GetLibraryVersion() ));

	/*** Connect to the Newport XPS controller. ***/

	newport_xps->socket_id = TCP_ConnectToServer(
					newport_xps->hostname,
					newport_xps->port_number,
					5.0 );

	MX_DEBUG(-2,("%s: newport_xps->socket_id = %d",
			fname, newport_xps->socket_id));

	if ( newport_xps->socket_id < 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to connect to Newport XPS controller '%s' failed.",
			record->name );
	}

	/* Login to the Newport XPS controller. */

	xps_status = Login( newport_xps->socket_id,
				newport_xps->username,
				newport_xps->password );

	MX_DEBUG(-2,("%s: Login() status = %d", fname, xps_status));

	if ( xps_status == SUCCESS ) {
		MX_DEBUG(-2,("%s: Login() successfully completed.", fname));
	} else {
		return mxi_newport_xps_error( newport_xps,
						"Login()",
						xps_status );
	}

	/*** Display some information about the controller's configuration. ***/

	xps_status = FirmwareVersionGet( newport_xps->socket_id,
					firmware_version );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps,
						"FirmwareVersionGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: firmware version = '%s'", fname, firmware_version));

	/*---*/

	xps_status = ElapsedTimeGet( newport_xps->socket_id,
					&elapsed_seconds_since_power_on );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps,
						"ElapsedTimeGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: %f seconds since power on.",
		fname, elapsed_seconds_since_power_on));

	/*---*/

	xps_status = HardwareDateAndTimeGet( newport_xps->socket_id,
					hardware_date_and_time );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps,
						"HardwareDateAndTimeGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: hardware date and time = '%s'",
		fname, hardware_date_and_time ));

	/*---*/

	xps_status = ControllerStatusGet( newport_xps->socket_id,
					&controller_status_code );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps,
						"ControllerStatusGet()",
						xps_status );
	}

	xps_status = ControllerStatusStringGet( newport_xps->socket_id,
						controller_status_code,
						controller_status_string );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps,
						"ControllerStatusStringGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: controller status = %d, '%s'",
		fname, controller_status_code, controller_status_string));

	/*******************************************************************/

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

	newport_xps->move_thread_socket_id = TCP_ConnectToServer(
					newport_xps->hostname,
					newport_xps->port_number,
					5.0 );

	MX_DEBUG(-2,("%s: newport_xps->move_thread_socket_id = %d",
			fname, newport_xps->move_thread_socket_id));

	if ( newport_xps->move_thread_socket_id < 0 ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to make a second connection to Newport XPS "
		"controller '%s' for move commands failed.",
			record->name );
	}

	/* Login to the Newport XPS controller (on the additional socket). */

	xps_status = Login( newport_xps->move_thread_socket_id,
				newport_xps->username,
				newport_xps->password );

	MX_DEBUG(-2,("%s: Login() status = %d", fname, xps_status));

	if ( xps_status == SUCCESS ) {
		MX_DEBUG(-2,
		("%s: Login() successfully completed for move thread.", fname));
	} else {
		return mxi_newport_xps_error( newport_xps,
						"Login() (for move thread)",
						xps_status );
	}

	/* Set 'move_in_progress' to FALSE so that the thread will not
	 * prematurely try to send a move command.
	 */

	newport_xps->move_in_progress = FALSE;

	newport_xps->command_type[0] = '\0';
	newport_xps->commanded_object_name[0] = '\0';
	newport_xps->commanded_destination = 0.0;

	/* Create the mutex and condition variable that the thread will
	 * use to synchronize with the main thread.
	 */

	mx_status = mx_mutex_create( &(newport_xps->move_thread_mutex) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_condition_variable_create(
				&(newport_xps->move_thread_cv) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the move thread. */

	mx_status = mx_thread_create( &(newport_xps->move_thread),
					mxi_newport_xps_move_thread,
					newport_xps );

	return mx_status;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_error( MX_NEWPORT_XPS *newport_xps,
			char *api_name,
			int error_code )
{
	static const char fname[] = "mxi_newport_xps_error()";

	int status_of_error_string_get;
	char error_message[300];
	mx_status_type mx_status;

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NEWPORT_XPS pointer passed was NULL." );
	}

	if ( error_code == SUCCESS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	status_of_error_string_get = ErrorStringGet( newport_xps->socket_id,
						error_code, error_message );

	if ( status_of_error_string_get != SUCCESS ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to get the error string for XPS error code %d "
		"failed for controller '%s'",
			error_code, newport_xps->record->name );
	}

	mx_status = mx_error( MXE_DEVICE_ACTION_FAILED, api_name,
			"The XPS function call for controller '%s' "
			"failed.  XPS error code = %d, error message = '%s'",
				newport_xps->record->name,
				error_code,
				error_message );

	return mx_status;
}

