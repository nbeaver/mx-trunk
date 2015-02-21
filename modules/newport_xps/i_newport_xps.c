/*
 * Name:    i_newport_xps.c
 *
 * Purpose: MX driver for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NEWPORT_XPS_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
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
mxi_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_newport_xps_open()";

	MX_NEWPORT_XPS *newport_xps = NULL;
	int xps_status;

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
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"The attempt to connect to Newport XPS controller '%s' failed.",
			record->name );
	}

#if 0
	/* Login to the Newport XPS controller. */

	xps_status = Login( newport_xps->socket_id,
				newport_xps->username,
				newport_xps->password );
#else
	xps_status = 0;
#endif

	if ( xps_status != SUCCESS ) {
		return mxi_newport_xps_error( newport_xps->socket_id,
						"Login()",
						xps_status );
	}

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
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_newport_xps_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_NEWPORT_XPS *newport_xps;
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
					newport_xps->socket_id,
					"ControllerStatusGet()",
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
					newport_xps->socket_id,
					"ControllerStatusGet()",
					xps_status );
			}

			newport_xps->controller_status = controller_status;

			xps_status = ControllerStatusStringGet(
						newport_xps->socket_id,
						controller_status,
					newport_xps->controller_status_string );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"ControllerStatusStringGet()",
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_ELAPSED_TIME:
			xps_status = ElapsedTimeGet( newport_xps->socket_id,
						&(newport_xps->elapsed_time) );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"ElapsedTimeGet()",
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_FIRMWARE_VERSION:
			xps_status = FirmwareVersionGet(
						newport_xps->socket_id,
						newport_xps->firmware_version );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"FirmwareVersionGet()",
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_HARDWARE_TIME:
			xps_status = HardwareDateAndTimeGet(
						newport_xps->socket_id,
						newport_xps->hardware_time );

			if ( xps_status != SUCCESS ) {
				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"HardwareDateAndTimeGet()",
					xps_status );
			}
			break;
		case MXLV_NEWPORT_XPS_LIBRARY_VERSION:
			strlcpy( newport_xps->library_version,
				GetLibraryVersion(),
				sizeof(newport_xps->library_version) );
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

/*--------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_newport_xps_error( int socket_id,
			char *api_name,
			int error_code )
{
	static const char fname[] = "mxi_newport_xps_error()";

	int status_of_error_string_get;
	char error_message[300];
	mx_status_type mx_status;

	if ( error_code == SUCCESS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	status_of_error_string_get = ErrorStringGet( socket_id,
						error_code, error_message );

	if ( status_of_error_string_get != SUCCESS ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to get the error string for XPS error code %d "
		"failed for socket ID %d",
			error_code, socket_id );
	}

	mx_status = mx_error( MXE_DEVICE_ACTION_FAILED, api_name,
			"The XPS function call for socket ID %d "
			"failed.  XPS error code = %d, error message = '%s'",
				socket_id,
				error_code,
				error_message );

	return mx_status;
}

