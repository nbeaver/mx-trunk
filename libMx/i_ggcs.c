/*
 * Name:    i_ggcs.c
 *
 * Purpose: MX driver for a Bruker GGCS goniostat controller.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

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
#include "i_ggcs.h"

MX_RECORD_FUNCTION_LIST mxi_ggcs_record_function_list = {
	mxi_ggcs_initialize_type,
	mxi_ggcs_create_record_structures,
	mxi_ggcs_finish_record_initialization,
	mxi_ggcs_delete_record,
	NULL,
	mxi_ggcs_read_parms_from_hardware,
	mxi_ggcs_write_parms_to_hardware,
	mxi_ggcs_open,
	mxi_ggcs_close
};

MX_GENERIC_FUNCTION_LIST mxi_ggcs_generic_function_list = {
	mxi_ggcs_getchar,
	mxi_ggcs_putchar,
	mxi_ggcs_read,
	mxi_ggcs_write,
	mxi_ggcs_num_input_bytes_available,
	mxi_ggcs_discard_unread_input,
	mxi_ggcs_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_ggcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_GGCS_STANDARD_FIELDS
};

long mxi_ggcs_num_record_fields
		= sizeof( mxi_ggcs_record_field_defaults )
			/ sizeof( mxi_ggcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_ggcs_rfield_def_ptr
			= &mxi_ggcs_record_field_defaults[0];

#define MXI_GGCS_DEBUG		FALSE

/*==========================*/

MX_EXPORT mx_status_type
mxi_ggcs_initialize_type( long type )
{
	/* Nothing needed here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ggcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_ggcs_create_record_structures()";

	MX_GENERIC *generic;
	MX_GGCS *ggcs;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	ggcs = (MX_GGCS *) malloc( sizeof(MX_GGCS) );

	if ( ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_GGCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = ggcs;
	record->class_specific_function_list = &mxi_ggcs_generic_function_list;

	generic->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ggcs_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_ggcs_finish_record_initialization()";

	MX_GGCS *ggcs;

	ggcs = (MX_GGCS *) record->record_type_struct;

	/* Only RS-232 records are supported. */

	if (ggcs->rs232_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an interface record.",
			ggcs->rs232_record->name );
	}

	if ( ggcs->rs232_record->mx_class != MXI_RS232 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"'%s' is not an RS-232 interface.",
			ggcs->rs232_record->name );
	}

	/* Mark the GGCS device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ggcs_delete_record( MX_RECORD *record )
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
mxi_ggcs_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ggcs_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ggcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_ggcs_open()";

	MX_GGCS *ggcs;
	char response[80];
	mx_status_type status;

	MX_DEBUG(-2, ("mxi_ggcs_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (record->record_type_struct);

	if ( ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GGCS pointer for record '%s' is NULL.",
		record->name);
	}

	/* Throw away any pending input and output on the GGCS RS-232 port. */

	status = mx_rs232_discard_unread_input( ggcs->rs232_record,
					MXI_GGCS_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_discard_unwritten_output( ggcs->rs232_record,
					MXI_GGCS_DEBUG );

	switch( status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return status;
		break;
	}

	/* Verify that the GGCS controller is active by querying it for
	 * its firmware version number.
	 */

	status = mx_rs232_putline( ggcs->rs232_record, "SW",
						NULL, MXI_GGCS_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Read the controller version string. */

	status = mx_rs232_getline( ggcs->rs232_record,
			response, sizeof response, NULL, MXI_GGCS_DEBUG );

	/* Check to see if we timed out. */

	switch( status.code ) {
	case MXE_SUCCESS:
		break;
	case MXE_NOT_READY:
		return mx_error( MXE_NOT_READY, fname,
		"No response from the GGCS controller '%s'.  Is it turned on?",
			record->name );
		break;
	default:
		return status;
		break;
	}

	/* Check to see if the response string matches what we expect. */

	if ( strncmp( response, "GC ", 3 ) != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The device for interface '%s' is not a GGCS controller.  "
		"Response to 'SW' command was '%s'",
			record->name, response );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_ggcs_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_ggcs_close()";

	MX_GGCS *ggcs;
	mx_status_type status;

	MX_DEBUG(-2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (record->record_type_struct);

	if ( ggcs == (MX_GGCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_GGCS pointer for record '%s' is NULL.",
		record->name);
	}

	/* Throw away any pending input and output on the GGCS RS-232 port. */

	status = mx_rs232_discard_unread_input( ggcs->rs232_record,
					MXI_GGCS_DEBUG );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_rs232_discard_unwritten_output( ggcs->rs232_record,
					MXI_GGCS_DEBUG );

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
mxi_ggcs_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_ggcs_getchar()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_getchar( ggcs->rs232_record, c, flags );

	return status;
}

MX_EXPORT mx_status_type
mxi_ggcs_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_ggcs_putchar()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_putchar( ggcs->rs232_record, c, flags );

	return status;
}

MX_EXPORT mx_status_type
mxi_ggcs_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_ggcs_read()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_read( ggcs->rs232_record, buffer, count, NULL, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxi_ggcs_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_ggcs_write()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_write( ggcs->rs232_record, buffer, count, NULL, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxi_ggcs_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available )
{
	static const char fname[] = "mxi_ggcs_num_input_bytes_available()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_num_input_bytes_available(
			ggcs->rs232_record, num_input_bytes_available );

	return status;
}

MX_EXPORT mx_status_type
mxi_ggcs_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_ggcs_discard_unread_input()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_discard_unread_input(ggcs->rs232_record, debug_flag);

	return status;
}

MX_EXPORT mx_status_type
mxi_ggcs_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_ggcs_discard_unwritten_output()";

	MX_GGCS *ggcs;
	mx_status_type status;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	ggcs = (MX_GGCS *) (generic->record->record_type_struct);

	if ( ggcs == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_GGCS pointer for MX_GENERIC structure is NULL." );
	}

	status = mx_rs232_discard_unwritten_output( ggcs->rs232_record,
					debug_flag );

	return status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_ggcs_command( MX_GGCS *ggcs,
		char *command, char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_ggcs_command()";

	unsigned long sleep_ms;
	int i, max_attempts;
	mx_status_type status;

	if ( ggcs == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_GGCS pointer passed was NULL." );
	}

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	/* Send the command string. */

	status = mx_rs232_putline( ggcs->rs232_record,
						command, NULL, debug_flag );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Get the response, if one is expected. */

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	if ( response != NULL ) {
		for ( i=0; i < max_attempts; i++ ) {
			if ( i > 0 ) {
				MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
				ggcs->record->name, command ));
			}

			status = mx_rs232_getline( ggcs->rs232_record,
					response, response_buffer_length,
					NULL, debug_flag );

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
					ggcs->rs232_record,
					debug_flag );

			if ( status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					ggcs->record->name );
			}

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"No response seen to '%s' command", command );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

