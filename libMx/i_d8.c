/*
 * Name:    i_d8.c
 *
 * Purpose: MX driver for a Bruker D8 goniostat controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_D8_DEBUG		FALSE

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
#include "i_d8.h"

MX_RECORD_FUNCTION_LIST mxi_d8_record_function_list = {
	mxi_d8_initialize_type,
	mxi_d8_create_record_structures,
	mxi_d8_finish_record_initialization,
	mxi_d8_delete_record,
	NULL,
	mxi_d8_read_parms_from_hardware,
	mxi_d8_write_parms_to_hardware,
	mxi_d8_open,
	mxi_d8_close
};

MX_GENERIC_FUNCTION_LIST mxi_d8_generic_function_list = {
	mxi_d8_getchar,
	mxi_d8_putchar,
	mxi_d8_read,
	mxi_d8_write,
	mxi_d8_num_input_bytes_available,
	mxi_d8_discard_unread_input,
	mxi_d8_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_d8_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_D8_STANDARD_FIELDS
};

mx_length_type mxi_d8_num_record_fields
		= sizeof( mxi_d8_record_field_defaults )
			/ sizeof( mxi_d8_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_d8_rfield_def_ptr
			= &mxi_d8_record_field_defaults[0];

static mx_status_type mxi_d8_construct_error_message( char *d8_response,
				char *record_name, const char *fname );

/*==========================*/

MX_EXPORT mx_status_type
mxi_d8_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_d8_create_record_structures()";

	MX_GENERIC *generic;
	MX_D8 *d8;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	d8 = (MX_D8 *) malloc( sizeof(MX_D8) );

	if ( d8 == (MX_D8 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_D8 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = d8;
	record->class_specific_function_list = &mxi_d8_generic_function_list;

	generic->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_d8_finish_record_initialization()";

	MX_D8 *d8;

	d8 = (MX_D8 *) record->record_type_struct;

	/* Only RS-232 records are supported. */

	if (d8->rs232_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an interface record.",
			d8->rs232_record->name );
	}

	if ( d8->rs232_record->mx_class != MXI_RS232 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an RS-232 interface.",
			d8->rs232_record->name );
	}

	/* Mark the D8 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_delete_record( MX_RECORD *record )
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
mxi_d8_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_d8_open()";

	MX_D8 *d8;
	char response[80];
	mx_status_type status;

	MX_DEBUG(-2, ("mxi_d8_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (record->record_type_struct);

	if ( d8 == (MX_D8 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_D8 pointer for record '%s' is NULL.",
		record->name);
	}

	/* Throw away any pending input and output on the D8 RS-232 port. */

	status = mx_rs232_discard_unread_input( d8->rs232_record,
					MXI_D8_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_discard_unwritten_output( d8->rs232_record,
					MXI_D8_DEBUG );

	switch( status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return status;
		break;
	}

	/* Put the D8 controller into remote mode. */

	status = mx_rs232_putline( d8->rs232_record, "RC1",
						NULL, MXI_D8_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* A blank line should be sent back to us. */

	(void) mx_rs232_getline( d8->rs232_record,
			response, sizeof response, NULL, MXI_D8_DEBUG );

	/* Verify that the D8 controller is active by querying it for
	 * its firmware version number.
	 */

	status = mx_rs232_putline( d8->rs232_record, "RV",
						NULL, MXI_D8_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Read the controller version string. */

	status = mx_rs232_getline( d8->rs232_record,
			response, sizeof response, NULL, MXI_D8_DEBUG );

	/* Check to see if we timed out. */

	switch( status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_READY:
		return mx_error( MXE_NOT_READY, fname,
		"No response from the D8 controller '%s'.  Is it turned on?",
			record->name );
		break;
	default:
		return status;
		break;
	}

	/* Check to see if the response string matches what we expect. */

	if ( strncmp( response, "RVD8-Diffractometer", 19 ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The device for interface '%s' is not a D8 controller.  "
		"Response to 'RV' command was '%s'",
			record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_d8_close()";

	MX_D8 *d8;
	mx_status_type status;

	MX_DEBUG(-2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (record->record_type_struct);

	if ( d8 == (MX_D8 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_D8 pointer for record '%s' is NULL.",
		record->name);
	}

	/* Throw away any pending input and output on the D8 RS-232 port. */

	status = mx_rs232_discard_unread_input( d8->rs232_record,
					MXI_D8_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_discard_unwritten_output( d8->rs232_record,
					MXI_D8_DEBUG );

	switch( status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return status;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_d8_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_d8_getchar()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_getchar( d8->rs232_record, c, flags );

	return status;
}

MX_EXPORT mx_status_type
mxi_d8_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_d8_putchar()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_putchar( d8->rs232_record, c, flags );

	return status;
}

MX_EXPORT mx_status_type
mxi_d8_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_d8_read()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_read( d8->rs232_record, buffer, count, NULL, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxi_d8_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_d8_write()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_write( d8->rs232_record, buffer, count, NULL, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxi_d8_num_input_bytes_available( MX_GENERIC *generic,
				uint32_t *num_input_bytes_available )
{
	static const char fname[] = "mxi_d8_num_input_bytes_available()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_num_input_bytes_available(
			d8->rs232_record, num_input_bytes_available );

	return status;
}

MX_EXPORT mx_status_type
mxi_d8_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_d8_discard_unread_input()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_discard_unread_input(d8->rs232_record, debug_flag);

	return status;
}

MX_EXPORT mx_status_type
mxi_d8_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_d8_discard_unwritten_output()";

	MX_D8 *d8;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	d8 = (MX_D8 *) (generic->record->record_type_struct);

	if ( d8 == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_D8 pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_discard_unwritten_output( d8->rs232_record,
					debug_flag );

	return status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_d8_command( MX_D8 *d8,
		char *command, char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_d8_command()";

	unsigned long sleep_ms;
	int i, max_attempts;
	char internal_response_buffer[80];
	char *response_ptr;
	int max_chars;
	mx_status_type status;

	if ( d8 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_D8 pointer passed was NULL." );
	}

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	/* Send the command string. */

	status = mx_rs232_putline( d8->rs232_record,
						command, NULL, debug_flag );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the response, if one is expected. */

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	if ( response == NULL ) {
		response_ptr = &internal_response_buffer[0];
		max_chars = sizeof(internal_response_buffer) - 1;
	} else {
		response_ptr = response;
		max_chars = response_buffer_length;
	}

	for ( i=0; i < max_attempts; i++ ) {
		if ( i > 0 ) {
			MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
				d8->record->name, command ));
		}

		status = mx_rs232_getline( d8->rs232_record,
				response_ptr, max_chars, NULL, debug_flag );

		if ( status.code == MXE_SUCCESS ) {
			break;		/* Exit the for() loop. */

		} else if ( status.code != MXE_NOT_READY ) {
			MX_DEBUG(-2,
				("*** Exiting with status = %ld",status.code));
			return status;
		}
		mx_msleep(sleep_ms);
	}

	if ( i >= max_attempts ) {
		status = mx_rs232_discard_unread_input(
					d8->rs232_record,
					debug_flag );

		if ( status.code != MXE_SUCCESS ) {
			mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					d8->record->name );
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"No response seen to '%s' command", command );
	}

	if ( *response_ptr == '?' ) {
		return mxi_d8_construct_error_message( response_ptr,
					d8->record->name, fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_d8_construct_error_message( char *d8_response,
			char *record_name, const char *calling_fname )
{
	static char d8_error_message[][80] = {
	/*  0 */	"Unknown error code 0",
	/*  1 */	"Command unknown",
	/*  2 */	"Argument required",
	/*  3 */	"Incorrect argument(s)",
	/*  4 */	"Command only permissible in remote control mode",
	/*  5 */	"Command only permissible in setup mode",
	/*  6 */	"Setup mode active",
	/*  7 */	"No argument(s) permissible",
	/*  8 */	"Component not installed",
	/*  9 */	"Position of drive unknown",
	/* 10 */	"Component / Drive not initialized",
	/* 11 */	"Component / Drive at limit switch",
	/* 12 */	"Measuring function running",
	/* 13 */	"Service switch in position SERVICE",
	/* 14 */	"No response from X-ray generator",
	/* 15 */	"Remote control by another serial communication port",
	/* 16 */	"Serial communication port busy",
	/* 17 */	"File not found",
	/* 18 */	"File access error",
	/* 19 */	"File read/write error",
	/* 20 */	"X-ray generator error message",
	/* 21 */	"X-ray generator error message",
	/* 22 */	"X-ray generator error message",
	/* 23 */	"X-ray generator error message",
	/* 24 */	"X-ray generator error message",
	/* 25 */	"X-ray generator error message",
	/* 26 */	"X-ray generator error message",
	/* 27 */	"X-ray generator error message",
	/* 28 */	"X-ray generator error message",
	/* 29 */	"X-ray generator error message",
	};

	static int num_error_messages = sizeof( d8_error_message ) /
					sizeof( d8_error_message[0] );

	char buffer[40];
	int num_tokens, d8_error_code;
	char *error_message_ptr;

	num_tokens = sscanf( d8_response, "?%d", &d8_error_code );

	if ( num_tokens != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, calling_fname,
		"No error code seen in D8 response string '%s'", d8_response );
	}

	if ( d8_error_code >= 0 && d8_error_code < num_error_messages ) {
		error_message_ptr = d8_error_message[ d8_error_code ];
	} else {
		sprintf( buffer, "Unknown error code %d", d8_error_code );
		error_message_ptr = buffer;
	}

	return mx_error( MXE_INTERFACE_IO_ERROR, calling_fname,
"Error communicating with Bruker D8 controller '%s', code = %d, reason = '%s'",
		record_name, d8_error_code, error_message_ptr );
}

