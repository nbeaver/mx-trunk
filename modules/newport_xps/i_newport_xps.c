/*
 * Name:    i_newport_xps.c
 *
 * Purpose: MX driver for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015, 2017, 2019, 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NEWPORT_XPS_DEBUG			FALSE

#define MXI_NEWPORT_XPS_FIND_MOTOR_RECORDS	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "i_newport_xps.h"
#include "d_newport_xps.h"

/* Vendor include files. */

#include "Socket.h"
#include "XPS_C8_drivers.h"
#include "XPS_C8_errors.h"

MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list = {
	NULL,
	mxi_newport_xps_create_record_structures,
	mxi_newport_xps_finish_record_initialization,
	NULL,
	NULL,
	mxi_newport_xps_open,
	NULL,
	NULL,
	NULL,
	mxi_newport_xps_special_processing_setup
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

/*---*/

static mx_status_type mxi_newport_xps_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

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

	newport_xps->controller_status = 0;
	newport_xps->controller_status_string[0] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_newport_xps_finish_record_initialization()";

	MX_NEWPORT_XPS *newport_xps = NULL;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor = NULL;
	MX_RECORD *current_record = NULL;
	MX_RECORD *list_head_record = NULL;
	long i, num_motors;
	unsigned long flags;
	const char *driver_name = NULL;

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

	/* Check to see if we need to turn on XPS socket debugging. */

	flags = newport_xps->newport_xps_flags;

	if ( flags & MXF_NEWPORT_XPS_DEBUG_XPS_SOCKET ) {
		mxp_newport_xps_set_comm_debug_flag( TRUE );
	}

	/* Figure out how many Newport motors are attached to this
	 * controller in the MX database.
	 */

	num_motors = 0;

	list_head_record = record->list_head;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {
		driver_name = mx_get_driver_name( current_record );

		if ( strcmp( driver_name, "newport_xps_motor" ) == 0 ) {
			/* If there are more than one Newport XPS motor
			 * controllers in use, then we only want to count
			 * motor axes attached to this record instance's
			 * MX record.
			 */

			newport_xps_motor = current_record->record_type_struct;

			if ( record == newport_xps_motor->newport_xps_record ) {
				num_motors++;
			}
		}

		current_record = current_record->next_record;
	}

#if MXI_NEWPORT_XPS_FIND_MOTOR_RECORDS
	MX_DEBUG(-2,("%s: num_motors = %ld", fname, num_motors));
#endif

	newport_xps->num_motors = num_motors;

	/* Now that we know how many child motor instances are attached to this
	 * controller's MX record, then we need to create some data structures
	 * that save information about the individual motor axes.
	 */

	newport_xps->motor_record_array = (MX_RECORD **)
		calloc( num_motors, sizeof(MX_RECORD *) );

	if ( newport_xps->motor_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of motor record pointers for controller '%s'.",
			num_motors, record->name );
	}

	/* Loop over the motor records for this controller. */

	i = 0;

	current_record = list_head_record->next_record;

	while ( current_record != list_head_record ) {
	    driver_name = mx_get_driver_name( current_record );

	    if ( strcmp( driver_name, "newport_xps_motor" ) == 0 ) {
		newport_xps_motor = current_record->record_type_struct;

		if ( record == newport_xps_motor->newport_xps_record ) {
		    newport_xps->motor_record_array[i] = current_record;

#if MXI_NEWPORT_XPS_FIND_MOTOR_RECORDS
		    MX_DEBUG(-2,("%s: motor_record_array[%ld] = '%s'",
			fname, i, newport_xps->motor_record_array[i]->name));
#endif
		    /* Save a copy of the array index 'i' into the motor
		     * record's data structures.
		     */

		    newport_xps_motor->array_index = i;

		    i++;
		}
	    }
	    current_record = current_record->next_record;
	}
	

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_newport_xps_open()";

	MX_NEWPORT_XPS *newport_xps = NULL;
	mx_status_type mx_status;

#if MXI_NEWPORT_XPS_DEBUG
	int controller_status_code;
	char controller_status_string[200];
	char firmware_version[200];
	char hardware_date_and_time[200];
	double elapsed_seconds_since_power_on;
#endif

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

#if MXI_NEWPORT_XPS_DEBUG
	MX_DEBUG(-2,("%s: '%s'", fname, GetLibraryVersion() ));
#endif

	/*** Connect to the Newport XPS controller. ***/

	newport_xps->socket_id = TCP_ConnectToServer(
					newport_xps->hostname,
					newport_xps->port_number,
					newport_xps->timeout );

#if MXI_NEWPORT_XPS_DEBUG
	MX_DEBUG(-2,("%s: newport_xps->socket_id = %ld",
			fname, newport_xps->socket_id));
#endif

	if ( newport_xps->socket_id < 0 ) {
		newport_xps->connected_to_controller = FALSE;

		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to connect to Newport XPS controller '%s' failed.",
			record->name );
	} else {
		newport_xps->connected_to_controller = TRUE;
	}

	/* Set XPS socket timeouts. */

	mx_status = mxi_newport_xps_process_function( record,
			mx_get_record_field( record, "socket_send_timeout" ),
			NULL, MX_PROCESS_PUT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_newport_xps_process_function( record,
			mx_get_record_field( record, "socket_receive_timeout" ),
			NULL, MX_PROCESS_PUT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NEWPORT_XPS_DEBUG

	/*** Display some information about the controller's configuration. ***/

	xps_status = FirmwareVersionGet( newport_xps->socket_id,
					firmware_version );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"FirmwareVersionGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: firmware version = '%s'", fname, firmware_version));

	/*---*/

	xps_status = ElapsedTimeGet( newport_xps->socket_id,
					&elapsed_seconds_since_power_on );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"ElapsedTimeGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: %f seconds since power on.",
		fname, elapsed_seconds_since_power_on));

	/*---*/

	xps_status = HardwareDateAndTimeGet( newport_xps->socket_id,
					hardware_date_and_time );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"HardwareDateAndTimeGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: hardware date and time = '%s'",
		fname, hardware_date_and_time ));

	/*---*/

	xps_status = ControllerStatusGet( newport_xps->socket_id,
					&controller_status_code );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"ControllerStatusGet()",
						xps_status );
	}

	xps_status = ControllerStatusStringGet( newport_xps->socket_id,
						controller_status_code,
						controller_status_string );

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"ControllerStatusStringGet()",
						xps_status );
	}

	MX_DEBUG(-2,("%s: controller status = %d, '%s'",
		fname, controller_status_code, controller_status_string));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_NEWPORT_XPS_CONTROLLER_STATUS:
		case MXLV_NEWPORT_XPS_CONTROLLER_STATUS_STRING:
		case MXLV_NEWPORT_XPS_ELAPSED_TIME:
		case MXLV_NEWPORT_XPS_FIRMWARE_VERSION:
		case MXLV_NEWPORT_XPS_HARDWARE_TIME:
		case MXLV_NEWPORT_XPS_LIBRARY_VERSION:
		case MXLV_NEWPORT_XPS_SOCKET_RECEIVE_TIMEOUT:
		case MXLV_NEWPORT_XPS_SOCKET_SEND_TIMEOUT:
			record_field->process_function
					= mxi_newport_xps_process_function;
			break;
		default:
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*--------------------------------------------------------------------------*/

static mx_status_type
mxi_newport_xps_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxi_newport_xps_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_NEWPORT_XPS *newport_xps;
	struct timeval timeout;
	socklen_t timeval_size = sizeof(struct timeval);
	int os_status, saved_errno;
	char error_message[200];
	int xps_status, controller_status;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	newport_xps = (MX_NEWPORT_XPS *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_NEWPORT_XPS_CONTROLLER_STATUS:
			xps_status = ControllerStatusGet(
						newport_xps->socket_id,
						&controller_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"ControllerStatusGet()",
					newport_xps->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps->controller_status = controller_status;
			break;
		case MXLV_NEWPORT_XPS_CONTROLLER_STATUS_STRING:
			xps_status = ControllerStatusGet(
						newport_xps->socket_id,
						&controller_status );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"ControllerStatusGet()",
					newport_xps->record->name,
					newport_xps->socket_id,
					xps_status );
			}

			newport_xps->controller_status = controller_status;

			xps_status = ControllerStatusStringGet(
						newport_xps->socket_id,
						controller_status,
					newport_xps->controller_status_string );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"ControllerStatusStringGet()",
					newport_xps->record->name,
					newport_xps->socket_id,
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_ELAPSED_TIME:
			xps_status = ElapsedTimeGet( newport_xps->socket_id,
						&(newport_xps->elapsed_time) );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"ElapsedTimeGet()",
					newport_xps->record->name,
					newport_xps->socket_id,
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_FIRMWARE_VERSION:
			xps_status = FirmwareVersionGet(
						newport_xps->socket_id,
						newport_xps->firmware_version );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"FirmwareVersionGet()",
					newport_xps->record->name,
					newport_xps->socket_id,
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_HARDWARE_TIME:
			xps_status = HardwareDateAndTimeGet(
						newport_xps->socket_id,
						newport_xps->hardware_time );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					"HardwareDateAndTimeGet()",
					newport_xps->record->name,
					newport_xps->socket_id,
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_LIBRARY_VERSION:
			strlcpy( newport_xps->library_version,
				GetLibraryVersion(),
				sizeof(newport_xps->library_version) );
			break;
		case MXLV_NEWPORT_XPS_SOCKET_RECEIVE_TIMEOUT:
			os_status = getsockopt( newport_xps->socket_id,
					SOL_SOCKET, SO_RCVTIMEO, 
					&timeout, &timeval_size );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to get SO_SNDTIMEO for the "
				"primary socket %ld of Newport XPS motor "
				"controller '%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps->socket_id,
				    newport_xps->record->name,
				    saved_errno, mx_strerror( saved_errno,
				    error_message, sizeof(error_message) ) );

			}

			newport_xps->socket_receive_timeout = 
				timeout.tv_sec + 1.0e-6 * timeout.tv_usec;
			break;
		case MXLV_NEWPORT_XPS_SOCKET_SEND_TIMEOUT:
			os_status = getsockopt( newport_xps->socket_id,
					SOL_SOCKET, SO_SNDTIMEO, 
					&timeout, &timeval_size );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to get SO_SNDTIMEO for the "
				"primary socket %ld of Newport XPS motor "
				"controller '%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps->socket_id,
				    newport_xps->record->name,
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
		case MXLV_NEWPORT_XPS_SOCKET_RECEIVE_TIMEOUT:
			timeout.tv_sec = newport_xps->socket_receive_timeout;
			timeout.tv_usec = 1000000.0 *
				( newport_xps->socket_receive_timeout
				- timeout.tv_sec );

			os_status = setsockopt( newport_xps->socket_id,
					SOL_SOCKET, SO_RCVTIMEO, 
					&timeout, sizeof(timeout) );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to set SO_SNDTIMEO for the "
				"primary socket %ld of Newport XPS motor "
				"controller '%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps->socket_id,
				    newport_xps->record->name,
				    saved_errno, mx_strerror( saved_errno,
				    error_message, sizeof(error_message) ) );

			}
			break;
		case MXLV_NEWPORT_XPS_SOCKET_SEND_TIMEOUT:
			timeout.tv_sec = newport_xps->socket_send_timeout;
			timeout.tv_usec = 1000000.0 *
				( newport_xps->socket_send_timeout
				- timeout.tv_sec );

			os_status = setsockopt( newport_xps->socket_id,
					SOL_SOCKET, SO_SNDTIMEO, 
					&timeout, sizeof(timeout) );

			if ( os_status != 0 ) {
				saved_errno = errno;

				return mx_error( MXE_NETWORK_IO_ERROR, fname,
				"The attempt to set SO_SNDTIMEO for the "
				"primary socket %ld of Newport XPS motor "
				"controller '%s' failed.  "
				"errno = %d, error message = '%s'",
				    newport_xps->socket_id,
				    newport_xps->record->name,
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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_error( char *api_name,
			char *error_location,
			int socket_id,
			int xps_error_code )
{
	static const char fname[] = "mxi_newport_xps_error()";

	int status_of_error_string_get;
	char xps_error_message[300];
	mx_status_type mx_status;

	if ( xps_error_code == SUCCESS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	status_of_error_string_get = ErrorStringGet( socket_id,
					xps_error_code, xps_error_message );

	if ( status_of_error_string_get != SUCCESS ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to get the error string for XPS error code %d "
		"failed for socket ID %d",
			xps_error_code, socket_id );
	}

	switch( xps_error_code ) {
	case SUCCESS:
		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	case ERR_TCP_TIMEOUT:
		mx_status = mx_error( MXE_TIMED_OUT, fname,
			"Function '%s' failed for '%s' (socket ID %d).  "
			"XPS error code = %d, error message = '%s'",
			api_name, error_location, socket_id,
			xps_error_code, xps_error_message );
		break;
	default:
		mx_status = mx_error( MXE_DEVICE_ACTION_FAILED, api_name,
			"Function '%s' failed for '%s' (socket ID %d).  "
			"XPS error code = %d, error message = '%s'",
			api_name, error_location, socket_id,
			xps_error_code, xps_error_message );
		break;
	}

	return mx_status;
}

