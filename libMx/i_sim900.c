/*
 * Name:    i_sim900.c
 *
 * Purpose: MX driver for the Stanford Research Systems SIM900 mainframe.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SIM900_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_sim900.h"

MX_RECORD_FUNCTION_LIST mxi_sim900_record_function_list = {
	NULL,
	mxi_sim900_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_sim900_open
};

MX_RECORD_FIELD_DEFAULTS mxi_sim900_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_SIM900_STANDARD_FIELDS
};

long mxi_sim900_num_record_fields
		= sizeof( mxi_sim900_record_field_defaults )
			/ sizeof( mxi_sim900_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_sim900_rfield_def_ptr
			= &mxi_sim900_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_sim900_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_sim900_create_record_structures()";

	MX_SIM900 *sim900;

	/* Allocate memory for the necessary structures. */

	sim900 = (MX_SIM900 *) malloc( sizeof(MX_SIM900) );

	if ( sim900 == (MX_SIM900 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SIM900 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = sim900;

	record->record_function_list = &mxi_sim900_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	sim900->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_sim900_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_sim900_open()";

	MX_SIM900 *sim900;
	MX_RECORD *interface_record;
	unsigned long sim900_flags;

	char response[100];
	char copy_of_response[100];
	int argc, num_items;
	char **argv;

	long speed;
	unsigned long read_terminators, write_terminators;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	sim900 = (MX_SIM900 *) record->record_type_struct;

	if ( sim900 == (MX_SIM900 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_SIM900 pointer for record '%s' is NULL.", record->name);
	}

	sim900_flags = sim900->sim900_flags;

#if MXI_SIM900_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', sim900_flags = %#lx.",
		fname, record->name, sim900_flags ));
#endif

	interface_record = sim900->sim900_interface.record;

	switch( interface_record->mx_class ) {
	case MXI_RS232:
		/* Verify that the RS-232 port has appropriate settings. */

		mx_status = mx_rs232_get_configuration( interface_record,
				&speed, NULL, NULL, NULL, NULL,
				&read_terminators, &write_terminators );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		mx_status = mx_rs232_verify_configuration( interface_record,
			MXF_232_DONT_CARE, 8, 'N', 1, 'S', 0xa, 0xa );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( speed ) {
		case 1200:
		case 9600:
		case 19200:
		case 57600:
		case 115200:
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported speed %ld requested for the "
			"RS-232 port '%s' used by SIM900 mainframe '%s'.",
				speed, interface_record->name, record->name );
			break;
		}
#endif

		/* Discard any characters waiting to be sent or received. */

		mx_status = mx_rs232_discard_unwritten_output(
					interface_record, MXI_SIM900_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_discard_unread_input(
					interface_record, MXI_SIM900_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXI_GPIB:
		/* GPIB does not require any initialization. */

		break;
	
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Only RS-232 and GPIB interfaces are supported for "
		"SIM900 interface '%s'.  Interface record '%s' is "
		"of unsupported type '%s'.",
			record->name, interface_record->name,
			mx_get_driver_name( interface_record ) );

		break;
	}

	/* Send a device clear to reset the mainframe to the idle state. */

	mx_status = mxi_sim900_device_clear( sim900, MXI_SIM900_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that this is a SIM900 mainframe. */

	mx_status = mxi_sim900_command( sim900, "*IDN?",
					response, sizeof(response),
					MXI_SIM900_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( copy_of_response, response, sizeof(copy_of_response) );

	mx_string_split( copy_of_response, ",", &argc, &argv );

	if ( argc != 4 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find 4 tokens in the response '%s' to "
		"the *IDN? command sent to '%s'.",
			response, record->name );
	}
	if ( strcmp( argv[0], "Stanford_Research_Systems" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Controller '%s' is not a Stanford Research Systems device.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}
	if ( strcmp( argv[1], "SIM900" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Controller '%s' is not a SIM900 mainframe.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Get the version number. */

	num_items = sscanf( argv[3], "ver%lf", &(sim900->version) );

	if ( num_items != 1 ) {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not find the SIM900 version number in the token '%s' "
		"contained in the response '%s' to '*IDN?' by controller '%s'.",
			argv[3], response, record->name );

		free( argv );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#if 1

static mx_status_type
mxi_sim900_rs232_getline( MX_RS232 *rs232,
			char *response,
			size_t max_response_length )
{
	char c;
	unsigned long i;
	mx_bool_type in_terminator;
	mx_status_type mx_status;

	in_terminator = FALSE;

	for ( i = 0; i < max_response_length; i++ ) {
		mx_status = mx_rs232_getchar_with_timeout( rs232->record,
							&c, 0, rs232->timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( in_terminator ) {
			if ( c == MX_LF ) {
				response[i-1] = '\0';
				break;		/* Exit the for() loop. */
			} else {
				in_terminator = FALSE;
			}
		} else {
			if ( c == MX_CR ) {
				in_terminator = TRUE;
			}
		}

		response[i] = c;
	}

	/* Make sure the string is null terminated. */

	response[max_response_length - 1] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

#endif

/*---*/

MX_EXPORT mx_status_type
mxi_sim900_command( MX_SIM900 *sim900,
		char *command,
		char *response,
		size_t max_response_length,
		unsigned long sim900_flags )
{
	static const char fname[] = "mxi_sim900_command()";

	MX_RECORD *interface_record;
	long gpib_address;
	mx_bool_type debug;
	mx_status_type mx_status;

	MX_RS232 *rs232;

	if ( sim900 == (MX_SIM900 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIM900 pointer passed was NULL." );
	}

	if ( sim900_flags & MXF_SIM900_DEBUG ) {
		debug = TRUE;
	} else
	if ( sim900->sim900_flags & MXF_SIM900_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	interface_record = sim900->sim900_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The interface record pointer for SIM900 interface '%s' is NULL.",
			sim900->record->name );
	}

	/* Send the command and get the response. */

	if ( ( command != NULL ) && debug ) {
		MX_DEBUG(-2,("%s: sending command '%s' to '%s'.",
		    fname, command, sim900->record->name));
	}

	if ( interface_record->mx_class == MXI_RS232 ) {
		rs232 = (MX_RS232 *) interface_record->record_class_struct;

		if ( rs232 == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for record '%s' is NULL.",
				interface_record->name );
		}

		if ( command != NULL ) {
			mx_status = mx_rs232_putline( interface_record,
							command, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( response != NULL ) {
#if 1
			unsigned long raw_status_code;

			/* Use SIM900-specific getline method. */

			if ( rs232->timeout < 0.0 ) {
				mx_status = mx_rs232_getline(
							interface_record,
							response,
							max_response_length,
							NULL, 0 );
			} else {
				mx_status = mxi_sim900_rs232_getline(
							rs232,
							response,
							max_response_length );
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				raw_status_code = mx_status.code & (~MXE_QUIET);

				if ( raw_status_code != MXE_TIMED_OUT ) {
					return mx_status;
				} else {
					return mx_error( MXE_TIMED_OUT, fname,
				"Timed out while waiting for a response to the "
				"command '%s' sent to SIM900 '%s'.",
						command, sim900->record->name );
				}
			}
#else
			unsigned long num_bytes_available;

			/* Use generic RS-232 getline method. */

			/* Wait until there are bytes available to be read. */

			mx_status = mx_rs232_wait_for_input_available(
						interface_record,
						&num_bytes_available,
						rs232->timeout );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Get the response. */

			mx_status = mx_rs232_getline( interface_record,
					response, max_response_length, NULL, 0);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
#endif
		}

	} else {	/* GPIB */

		gpib_address = sim900->sim900_interface.address;

		if ( command != NULL ) {
			mx_status = mx_gpib_putline( interface_record,
							gpib_address,
							command, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( response != NULL ) {
			mx_status = mx_gpib_getline(
					interface_record, gpib_address,
					response, max_response_length, NULL, 0);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	if ( ( response != NULL ) && debug ) {
		MX_DEBUG(-2,("%s: received response '%s' from '%s'.",
			fname, response, sim900->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_sim900_device_clear( MX_SIM900 *sim900,
			unsigned long sim900_flags )
{
	static const char fname[] = "mxi_sim900_device_clear()";

	MX_RECORD *interface_record;
	mx_status_type mx_status;

	if ( sim900 == (MX_SIM900 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIM900 pointer passed was NULL." );
	}

	interface_record = sim900->sim900_interface.record;

	if ( interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The interface record pointer for SIM900 interface '%s' is NULL.",
			sim900->record->name );
	}


	/* Send the device clear request. */

	if ( sim900_flags & MXF_SIM900_DEBUG ) {

		MX_DEBUG(-2,("%s: sending device clear to '%s'.",
		    fname, sim900->record->name));
	}

	if ( interface_record->mx_class == MXI_RS232 ) {
		mx_status = mx_rs232_send_break( interface_record );
	} else {
		mx_status = mx_gpib_device_clear( interface_record );
	}

	return mx_status;
}

