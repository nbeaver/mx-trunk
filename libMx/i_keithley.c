/*
 * Name:    i_keithley.c
 *
 * Purpose: MX functions used with Keithley 2000, 2400, and 2700 series
 *          controllers.  The command language for all of these Keithley
 *          controllers is fairly similar.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_keithley.h"

MX_EXPORT mx_status_type
mxi_keithley_getline( MX_RECORD *keithley_record,
			MX_INTERFACE *port_interface,
			char *response, int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_keithley_getline()";

	mx_status_type mx_status;

	if ( keithley_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}
	if ( port_interface == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_INTERFACE pointer passed was NULL." );
	}
	if ( response == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'response' buffer pointer passed was NULL.  No chars read.");
	}

	switch( port_interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_getline( port_interface->record,
					response, response_buffer_length,
					NULL, debug_flag );
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_getline( port_interface->record,
					port_interface->address,
					response, response_buffer_length,
					NULL, debug_flag );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Interface record '%s' is not an RS-232 or GPIB record.",
			port_interface->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley_putline( MX_RECORD *keithley_record,
			MX_INTERFACE *port_interface,
			char *command, int debug_flag )
{
	static const char fname[] = "mxi_keithley_command()";

	mx_status_type mx_status;

	if ( keithley_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}
	if ( port_interface == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_INTERFACE pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	switch( port_interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_putline( port_interface->record,
						command, NULL, debug_flag );
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_putline( port_interface->record,
						port_interface->address,
						command, NULL, debug_flag );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Interface record '%s' is not an RS-232 or GPIB record.",
			port_interface->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley_command( MX_RECORD *keithley_record,
			MX_INTERFACE *port_interface,
			char *command,
			char *response, int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_keithley_command()";

	char error_status[40];
	int num_items, status_byte_register;
	mx_status_type mx_status;

	/* Send the command string. */

	mx_status = mxi_keithley_putline( keithley_record, port_interface,
						command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if one is expected. */

	if ( response != NULL ) {
		mx_status = mxi_keithley_getline( keithley_record,
					port_interface,
					response, response_buffer_length,
					debug_flag );
					
		if ( ( mx_status.code != MXE_SUCCESS )
		  && ( mx_status.code != MXE_TIMED_OUT ) )
		{
			return mx_status;
		}
	}

	/* See if there was an error in the command we just sent. */

	mx_status = mxi_keithley_putline( keithley_record, port_interface,
						"*STB?", debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley_getline( keithley_record, port_interface,
					error_status, sizeof(error_status),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( error_status, "%d", &status_byte_register );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find a status by value in the response to "
		"a *STB? command for interface '%s'.  Response = '%s'",
			port_interface->record->name, error_status );
	}

	/**** If no error occurred, then return. ****/

	if ( ( status_byte_register & MXF_KEITHLEY_STB_EAV ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Read in the error message from the Error Queue. */

	mx_status = mxi_keithley_putline( keithley_record, port_interface,
						":STAT:QUE:NEXT?", debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_keithley_getline( keithley_record, port_interface,
					error_status, sizeof(error_status),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: error message = '%s'", fname, error_status));

	return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	"The command '%s' sent to Keithley '%s' resulted in an error.  "
	"Error message = '%s'", command, keithley_record->name, error_status );
}

