/*
 * Name:    i_ortec974.c
 *
 * Purpose: MX driver for EG&G Ortec 974 counter/timer units.
 *
 * Note:    You also need the scaler driver in d_ortec974_scaler.c and the
 *          timer driver in d_ortec974_timer.c in order to make use of
 *          an Ortec 974.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_ORTEC974_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_generic.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_ortec974.h"

MX_RECORD_FUNCTION_LIST mxi_ortec974_record_function_list = {
	mxi_ortec974_initialize_type,
	mxi_ortec974_create_record_structures,
	mxi_ortec974_finish_record_initialization,
	mxi_ortec974_delete_record,
	NULL,
	mxi_ortec974_read_parms_from_hardware,
	mxi_ortec974_write_parms_to_hardware,
	mxi_ortec974_open,
	mxi_ortec974_close
};

MX_GENERIC_FUNCTION_LIST mxi_ortec974_generic_function_list = {
	mxi_ortec974_getchar,
	mxi_ortec974_putchar,
	mxi_ortec974_read,
	mxi_ortec974_write,
	mxi_ortec974_num_input_bytes_available,
	mxi_ortec974_discard_unread_input,
	mxi_ortec974_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_ortec974_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_ORTEC974_STANDARD_FIELDS
};

mx_length_type mxi_ortec974_num_record_fields
		= sizeof( mxi_ortec974_record_field_defaults )
			/ sizeof( mxi_ortec974_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_ortec974_rfield_def_ptr
			= &mxi_ortec974_record_field_defaults[0];

/*==========================*/

MX_EXPORT mx_status_type
mxi_ortec974_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_ortec974_create_record_structures()";

	MX_GENERIC *generic;
	MX_ORTEC974 *ortec974;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	ortec974 = (MX_ORTEC974 *) malloc( sizeof(MX_ORTEC974) );

	if ( ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_ORTEC974 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = ortec974;
	record->class_specific_function_list
				= &mxi_ortec974_generic_function_list;

	generic->record = record;
	ortec974->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_ortec974_finish_record_initialization()";

	MX_ORTEC974 *ortec974;
	MX_RECORD *interface_record;
	MX_RS232 *rs232;
	int i;

	ortec974 = (MX_ORTEC974 *) record->record_type_struct;

	interface_record = ortec974->module_interface.record;

	/* Only RS-232 and GPIB records are supported. */

	if ( interface_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an interface record.",
			interface_record->name );
	}

	if ( (interface_record->mx_class != MXI_RS232)
	  && (interface_record->mx_class != MXI_GPIB) ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an RS-232 or GPIB interface.",
			interface_record->name );
	}

	/* Make some interface type specific tests. */

	switch( interface_record->mx_class ) {
	case MXI_RS232:

		/* The Ortec 974 does not support hardware flow control
		 * on RS-232 ports, so abort if this is set.
		 */

		rs232 = (MX_RS232 *) interface_record->record_class_struct;

		if ( (rs232->flow_control != MXF_232_NO_FLOW_CONTROL)
		  && (rs232->flow_control != MXF_232_SOFTWARE_FLOW_CONTROL) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The Ortec 974 cannot use RS-232 hardware flow control.  "
		"Use software flow control instead." );
		}
	}

	/* Mark the ORTEC974 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	/* Null out the list of attached channels. */

	for ( i = 0; i < MX_NUM_ORTEC974_CHANNELS; i++ ) {
		ortec974->channel_record[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_ortec974_open()";

	MX_ORTEC974 *ortec974;
	MX_RECORD *interface_record;
	mx_status_type status;

	MX_DEBUG(2, ("mxi_ortec974_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *) (record->record_type_struct);

	if ( ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for record '%s' is NULL.", record->name);
	}

	interface_record = ortec974->module_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Interface MX_RECORD pointer for Ortec 974 record '%s' is NULL.",
			record->name );
	}

	/* Perform interface type specific initialization. */

	switch( ortec974->module_interface.record->mx_class ) {
	case MXI_RS232:
		/* Tell the 974 not to echo command characters that the
		 * computer sends to it.
		 */

		status = mx_rs232_putline( interface_record,
				"COMPUTER", NULL, MXI_ORTEC974_DEBUG );

		if ( status.code != MXE_SUCCESS )
			return status;

		mx_msleep(500);

		/* Send an XON character (ctrl-Q) to the 974. */

		status = mx_rs232_putchar( interface_record,
					'\021', MXF_232_WAIT );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Throw away any characters that the 974 may have already
		 * sent to us.
		 */

		status = mx_rs232_discard_unread_input( interface_record,
							MXI_ORTEC974_DEBUG );

		if ( status.code != MXE_SUCCESS )
			return status;

		break;

	case MXI_GPIB:
		/* Open a connection to this GPIB device's address. */

		status = mx_gpib_open_device( interface_record,
					ortec974->module_interface.address );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* If the 974 was just powered on, it will have generated
		 * a response record.  We need to get rid of this record
		 * before we can proceed.
		 */

		status = mx_gpib_selective_device_clear( interface_record,
					ortec974->module_interface.address );

		if ( status.code != MXE_SUCCESS )
			return status;

#if 0
		/* Do a remote enable. */

		status = mx_gpib_remote_enable( interface_record,
					ortec974->module_interface->address );

		if ( status.code != MXE_SUCCESS )
			return status;

#endif
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The interface record '%s' for Ortec 974 record '%s' has an illegal class %ld."
"  Only RS-232 and GPIB interfaces are supported for this module.",
			interface_record->name, record->name,
			ortec974->module_interface.record->mx_class );
	}

	/* Just in case the timer is running, tell it to stop. */

	status = mxi_ortec974_command( ortec974, "STOP",
					NULL, 0, NULL, 0, MXI_ORTEC974_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Configure the 974 to only send counter values when requested. */

	status = mxi_ortec974_command( ortec974, "DISABLE_ALARM",
					NULL, 0, NULL, 0, MXI_ORTEC974_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Put the 974 into remote control mode. */

	status = mxi_ortec974_command( ortec974, "ENABLE_REMOTE",
					NULL, 0, NULL, 0, MXI_ORTEC974_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_ortec974_close()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *) (record->record_type_struct);

	if ( ortec974 == (MX_ORTEC974 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for record '%s' is NULL.", record->name);
	}

	/* Stop the counters in case they are running. */

	status = mxi_ortec974_command( ortec974, "STOP",
					NULL, 0, NULL, 0, MXI_ORTEC974_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Put the 974 into local control mode. */

	status = mxi_ortec974_command( ortec974, "ENABLE_LOCAL",
					NULL, 0, NULL, 0, MXI_ORTEC974_DEBUG );

	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_ortec974_getchar()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

		status = mx_rs232_getchar(
			ortec974->module_interface.record, c, flags );

	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}
	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_ortec974_putchar()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

		status = mx_rs232_putchar(
			ortec974->module_interface.record, c, flags );

	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}
	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_ortec974_read()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

		status = mx_rs232_read( ortec974->module_interface.record,
						buffer, count, NULL, 0 );
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}

	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_ortec974_write()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

		status = mx_rs232_write(
		ortec974->module_interface.record, buffer, count, NULL, 0 );

	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}
	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_num_input_bytes_available( MX_GENERIC *generic,
				uint32_t *num_input_bytes_available )
{
	static const char fname[] = "mxi_ortec974_num_input_bytes_available()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

	status = mx_rs232_num_input_bytes_available(
			ortec974->module_interface.record,
			num_input_bytes_available );

	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}
	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_ortec974_discard_unread_input()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

		status = mx_rs232_discard_unread_input(
			ortec974->module_interface.record, debug_flag );

	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}
	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_ortec974_discard_unwritten_output()";

	MX_ORTEC974 *ortec974;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ortec974 = (MX_ORTEC974 *)(generic->record->record_type_struct);

	if ( ortec974 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ORTEC974 pointer for MX_GENERIC structure is NULL." );
	}

	if ( ortec974->module_interface.record->mx_class == MXI_RS232 ) {

		status = mx_rs232_discard_unwritten_output(
			ortec974->module_interface.record, debug_flag );

	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"%s is not yet implemented.", fname );
	}
	return status;
}

/* === Functions specific to this driver. === */

static mx_status_type
mxi_ortec974_wait_for_response_line( MX_ORTEC974 *ortec974,
			char *buffer,
			int buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_ortec974_wait_for_response_line()";

	MX_GENERIC *generic;
	int i, max_attempts, sleep_ms;
	mx_status_type status;

	generic = (MX_GENERIC *) ortec974->record->record_class_struct;

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	for ( i=0; i < max_attempts; i++ ) {
		if ( i > 0 ) {
			MX_DEBUG(-2,
		("%s: No response yet from Ortec 974 '%s'.  Retrying...",
				fname, ortec974->record->name ));
		}

		status = mxi_ortec974_getline( ortec974, buffer,
				buffer_length, debug_flag );

		if ( status.code == MXE_SUCCESS ) {

			/* If we got a zero length string, something
			 * went wrong.
			 */

#if 0
			if ( strlen(buffer) == 0 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"No data was received from the Ortec 974 '%s' when data was expected.",
					ortec974->record->name );
			}
#endif

			/* Otherwise, exit the for() loop. */

			break;

		} else if ( status.code != MXE_NOT_READY ) {
			return mx_error( status.code, fname,
			"Error reading response line from Ortec 974." );
		}
		mx_msleep(sleep_ms);
	}

	if ( i >= max_attempts ) {
		status = mxi_ortec974_discard_unread_input(
				generic, debug_flag );

		if ( status.code != MXE_SUCCESS ) {
			mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread input in buffer for record '%s'",
					ortec974->record->name );
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"No response seen from Ortec 974." );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}

MX_EXPORT mx_status_type
mxi_ortec974_command( MX_ORTEC974 *ortec974, char *command,
		char *response, int response_buffer_length,
		char *error_message, int error_message_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_ortec974_command()";

	char percent_response[14];
	char *buffer;
	int saw_response_line, buffer_length;
	mx_status_type status;

	if ( ortec974 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_ORTEC974 pointer passed was NULL." );
	}

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	/* === Send the command string. === */

	status = mxi_ortec974_putline( ortec974, command, debug_flag );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* The regular response line and the percent response line may
	 * come back in any order, so we cannot assume a particular order.
	 */

	/* === Get the first line of response to this command. === */

	saw_response_line = FALSE;

	if ( response == NULL ) {
		buffer = &percent_response[0];
		buffer_length = sizeof(percent_response) - 1;
	} else {
		buffer = response;
		buffer_length = response_buffer_length;
	}

	status = mxi_ortec974_wait_for_response_line( ortec974, buffer,
					buffer_length, debug_flag );

	if ( status.code != MXE_SUCCESS )
		return status;

	if ( buffer[0] == '%' ) {
		/* Got a percent response line. */

		saw_response_line = TRUE;

		if ( strncmp( buffer, "%000", 4 ) != 0 ) {

			/* If we were also expecting a line of data, we
			 * must read it and then throw it away.
			 */
			if ( response != NULL ) {

				strncpy( percent_response, buffer,
					sizeof( percent_response ) - 1 );

				(void) mxi_ortec974_wait_for_response_line(
					ortec974, response,
					response_buffer_length, debug_flag );
			}

			return mx_error( MXE_FUNCTION_FAILED, fname,
				"Error code '%s' returned by Ortec 974.",
				percent_response );
		}
	}

	/* === Get the second line (if any). === */

	if ( saw_response_line == FALSE ) {

		if ( response == NULL ) {
			(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
				"Command '%s' did not expect a data response, "
				"but got one anyway.  Response = '%s'",
				command, buffer );
		}

		/* There must still be a percent response line to read. */

		status = mxi_ortec974_wait_for_response_line( ortec974,
				percent_response, sizeof(percent_response)-1,
				debug_flag );

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( percent_response[0] == '%' ) {
			/* Got a percent response line. */

			saw_response_line = TRUE;

			if ( strncmp( percent_response, "%000", 4 ) != 0 ) {

				return mx_error( MXE_FUNCTION_FAILED, fname,
				    "Error code '%s' returned by Ortec 974.",
					percent_response );
			}
		} else {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not get a 'percent response' status code back from the Ortec 974."
	"  Instead got '%s'", percent_response );
		}
	} else {
		/* If we got here and the response buffer pointer is NULL,
		 * this means that the only thing we were supposed to get
		 * back from the 974 was a percent response line.  In this
		 * case, we are now done.
		 */

		if ( response != NULL ) {
			/* Otherwise, still have to get a line of data. */

			status = mxi_ortec974_wait_for_response_line( ortec974,
					response, response_buffer_length,
					debug_flag );

			if ( status.code != MXE_SUCCESS )
				return status;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ortec974_getline( MX_ORTEC974 *ortec974,
		char *buffer, long buffer_length, int debug_flag )
{
	static const char fname[] = "mxi_ortec974_getline()";

	MX_INTERFACE *interface;
	mx_status_type status;

	interface = &(ortec974->module_interface);

	if ( interface->record->mx_class == MXI_RS232 ) {

		status = mx_rs232_getline(interface->record,
				buffer, buffer_length, NULL, 0 );
	} else {
		status = mx_gpib_getline(interface->record, interface->address,
				buffer, buffer_length, NULL, 0 );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: buffer = '%s'", fname, buffer) );
	}

	return status;
}

MX_EXPORT mx_status_type
mxi_ortec974_putline( MX_ORTEC974 *ortec974, char *buffer, int debug_flag )
{
	static const char fname[] = "mxi_ortec974_putline()";

	MX_INTERFACE *interface;
	mx_status_type status;

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: buffer = '%s'", fname, buffer));
	}

	interface = &(ortec974->module_interface);

	if ( interface->record->mx_class == MXI_RS232 ) {

		status = mx_rs232_putline(interface->record,
				buffer, NULL, 0 );
	} else {
		status = mx_gpib_putline(interface->record, interface->address,
				buffer, NULL, 0 );
	}

	return status;
}

