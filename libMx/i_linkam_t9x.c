/*
 * Name:    i_linkam_t9x.c
 *
 * Purpose: MX interface driver for the Linkam T92, T93, T94, and T95 series
 *          of temperature controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LINKAM_T9X_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_motor.h"
#include "i_linkam_t9x.h"

MX_RECORD_FUNCTION_LIST mxi_linkam_t9x_record_function_list = {
	NULL,
	mxi_linkam_t9x_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_linkam_t9x_open
};

MX_RECORD_FIELD_DEFAULTS mxi_linkam_t9x_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LINKAM_T9X_STANDARD_FIELDS
};

long mxi_linkam_t9x_num_record_fields
		= sizeof( mxi_linkam_t9x_record_field_defaults )
		    / sizeof( mxi_linkam_t9x_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_linkam_t9x_rfield_def_ptr
			= &mxi_linkam_t9x_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_linkam_t9x_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_linkam_t9x_create_record_structures()";

	MX_LINKAM_T9X *linkam_t9x;

	/* Allocate memory for the necessary structures. */

	linkam_t9x = (MX_LINKAM_T9X *) malloc( sizeof(MX_LINKAM_T9X) );

	if ( linkam_t9x == (MX_LINKAM_T9X *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LINKAM_T9X structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = linkam_t9x;

	record->record_function_list = &mxi_linkam_t9x_record_function_list;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	linkam_t9x->record = record;

	linkam_t9x->module_type = -1;
	linkam_t9x->moves_are_relative = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linkam_t9x_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_linkam_t9x_open()";

	MX_LINKAM_T9X *linkam_t9x;
	char *mtn;
	size_t i, length;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL.");
	}

	linkam_t9x = (MX_LINKAM_T9X *) record->record_type_struct;

	if ( linkam_t9x == (MX_LINKAM_T9X *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINKAM_T9X pointer for record '%s' is NULL.",
			record->name);
	}

	/* Figure out the module type. */

	mtn = linkam_t9x->module_type_name;

	length = strlen(mtn);

	for ( i = 0; i < length; i++ ) {
		if ( islower(mtn[i]) ) {
			mtn[i] = toupper(mtn[i]);
		}
	}

	if ( strcmp( mtn, "T95" ) == 0 ) {
		linkam_t9x->module_type = MXT_LINKAM_T9X_IS_T92;
		linkam_t9x->moves_are_relative = TRUE;
	} else
	if ( strcmp( mtn, "T94" ) == 0 ) {
		linkam_t9x->module_type = MXT_LINKAM_T9X_IS_T94;
		linkam_t9x->moves_are_relative = FALSE;
	} else
	if ( strcmp( mtn, "T93" ) == 0 ) {
		linkam_t9x->module_type = MXT_LINKAM_T9X_IS_T93;
		linkam_t9x->moves_are_relative = FALSE;
	} else
	if ( strcmp( mtn, "T92" ) == 0 ) {
		linkam_t9x->module_type = MXT_LINKAM_T9X_IS_T92;
		linkam_t9x->moves_are_relative = FALSE;
	}

	/* Throw away any unread characters. */

	mx_status = mx_rs232_discard_unread_input( linkam_t9x->rs232_record,
							MXI_LINKAM_T9X_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(500);

	/* Verify that the Linkam T9x controller is present by asking
	 * for its status.
	 */

	mx_status = mxi_linkam_t9x_get_status( linkam_t9x,
						MXI_LINKAM_T9X_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linkam_t9x_command( MX_LINKAM_T9X *linkam_t9x,
			char *command,
			char *response,
			size_t response_buffer_length,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_linkam_t9x_command()";

	char local_buffer[80];
	char *local_response;
	size_t local_response_length;
	size_t bytes_read;
	mx_status_type mx_status;

	if ( linkam_t9x == (MX_LINKAM_T9X *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LINKAM_T9X pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command pointer passed was NULL." );
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
		    fname, command, linkam_t9x->record->name ));
	}

	mx_status = mx_rs232_putline( linkam_t9x->rs232_record,
					command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the response. */

	if ( response == NULL ) {
		local_response = local_buffer;
		local_response_length = sizeof(local_buffer);
	} else {
		local_response = response;
		local_response_length = response_buffer_length;
	}

	mx_status = mx_rs232_getline( linkam_t9x->rs232_record,
					local_response, local_response_length,
					&bytes_read, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
#if 1
		{
			char *ptr = local_response;

			fprintf( stderr,
			"%s: response (%d bytes) = ( ", fname, (int)bytes_read);

			while (1) {
				fprintf( stderr, "%#x ", ((*ptr) & 0xff) );

				if ( (*ptr) == '\0' )
					break;

				ptr++;
			}

			fprintf( stderr, ")\n" );
		}
#endif
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
		    fname, local_response, linkam_t9x->record->name ));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linkam_t9x_get_status( MX_LINKAM_T9X *linkam_t9x,
				mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_linkam_t9x_get_status()";

	char response[80];
	int t_response_length;
	long raw_temperature;
	mx_status_type mx_status;

	if ( linkam_t9x == (MX_LINKAM_T9X *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LINKAM_T9X pointer passed was NULL." );
	}

	mx_status = mxi_linkam_t9x_command( linkam_t9x, "T",
					response, sizeof(response),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	t_response_length = strlen(response);

	if ( t_response_length < 10 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' to the 'T' command from "
		"Linkam controller '%s' was shorter (%d) than "
		"the expected length of 10 bytes.",
			response, linkam_t9x->record->name,
			t_response_length );
	}

	linkam_t9x->status_byte    = response[0];
	linkam_t9x->error_byte     = response[1];
	linkam_t9x->pump_byte      = response[2];
	linkam_t9x->general_status = response[3];

	raw_temperature = mx_hex_string_to_long( &response[6] );

	linkam_t9x->temperature = 0.1 * (double) raw_temperature;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: status_byte    = %#x", fname,linkam_t9x->status_byte));
		MX_DEBUG(-2,
		("%s: error_byte     = %#x", fname,linkam_t9x->error_byte));
		MX_DEBUG(-2,
		("%s: pump_byte      = %#x", fname,linkam_t9x->pump_byte));
		MX_DEBUG(-2,
		("%s: general_status = %#x", fname,linkam_t9x->general_status));
		MX_DEBUG(-2,
		("%s: temperature    = %f", fname,linkam_t9x->temperature));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linkam_t9x_set_motor_status_from_error_byte( MX_LINKAM_T9X *linkam_t9x,
						MX_RECORD *motor_record )
{
	static const char fname[] =
		"mxi_linkam_t9x_set_motor_status_from_error_byte()";

	MX_MOTOR *motor;
	unsigned char eb;

	if ( linkam_t9x == (MX_LINKAM_T9X *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LINKAM_T9X pointer passed was NULL." );
	}
	if ( motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The motor_record pointer passed was NULL." );
	}

	motor = motor_record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for motor '%s' is NULL.",
			motor_record->name );
	}

	eb = linkam_t9x->error_byte;

	/* Bit 0 - Cooling rate too fast */

	if ( eb & 0x1 ) {
		motor->status |= MXSF_MTR_CONFIGURATION_ERROR;

		mx_warning(
		"Cooling rate too fast for Linkam T9x controller '%s'.",
			linkam_t9x->record->name );
	}

	/* Bit 1 - Open circuit */

	if ( eb & 0x2 ) {
		motor->status |= MXSF_MTR_HARDWARE_ERROR;

		mx_warning(
		"Open circuit for Linkam T9x controller '%s'.",
			linkam_t9x->record->name );
	}

	/* Bit 2 - Power surge */

	if ( eb & 0x4 ) {
		motor->status |= MXSF_MTR_DRIVE_FAULT;

		mx_warning(
		"Power surge for Linkam T9x controller '%s'.",
			linkam_t9x->record->name );
	}

	/* Bit 3 - No Exit 300 */

	return MX_SUCCESSFUL_RESULT;
}

