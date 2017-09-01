/*
 * Name:    i_amptek_dp5.c
 *
 * Purpose: MX driver for Amptek MCAs that use the DP5 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_AMPTEK_DP5_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
#include "i_amptek_dp5.h"

MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list = {
	NULL,
	mxi_amptek_dp5_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_amptek_dp5_open,
	NULL,
	NULL,
	NULL,
	mxi_amptek_dp5_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_amptek_dp5_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AMPTEK_DP5_STANDARD_FIELDS
};

long mxi_amptek_dp5_num_record_fields
		= sizeof( mxi_amptek_dp5_record_field_defaults )
			/ sizeof( mxi_amptek_dp5_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp5_rfield_def_ptr
			= &mxi_amptek_dp5_record_field_defaults[0];

/*---*/

static mx_status_type mxi_amptek_dp5_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/*---*/

MX_EXPORT mx_status_type
mxi_amptek_dp5_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_amptek_dp5_create_record_structures()";

	MX_AMPTEK_DP5 *amptek_dp5;

	/* Allocate memory for the necessary structures. */

	amptek_dp5 = (MX_AMPTEK_DP5 *) malloc( sizeof(MX_AMPTEK_DP5) );

	if ( amptek_dp5 == (MX_AMPTEK_DP5 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPTEK_DP5 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = amptek_dp5;

	record->record_function_list = &mxi_amptek_dp5_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	amptek_dp5->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_amptek_dp5_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_amptek_dp5_open()";

	MX_AMPTEK_DP5 *amptek_dp5 = NULL;
	char *int_name = NULL;
	MX_RECORD *interface_record = NULL;
	unsigned long flags;
	mx_status_type mx_status;

#if MXI_AMPTEK_DP5_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	amptek_dp5 = (MX_AMPTEK_DP5 *) record->record_type_struct;

	if ( amptek_dp5 == (MX_AMPTEK_DP5 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AMPTEK_DP5 pointer for record '%s' is NULL.", record->name);
	}

	flags = amptek_dp5->amptek_dp5_flags;

	/* What type of interface are we using. */

	int_name = amptek_dp5->interface_type_name;

	if ( mx_strcasecmp( int_name, "ethernet" ) == 0 ) {
		amptek_dp5->interface_type = MXI_ETHERNET;
	} else
	if ( mx_strcasecmp( int_name, "rs232" ) == 0 ) {
		amptek_dp5->interface_type = MXI_RS232;
	} else
	if ( mx_strcasecmp( int_name, "usb" ) == 0 ) {
		amptek_dp5->interface_type = MXI_USB;
	} else {
		amptek_dp5->interface_type = -1;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized interface type '%s' for record '%s'.",
			int_name, record->name );
	}

	/* Connect to the device. */

	switch( amptek_dp5->interface_type ) {
	case MXI_ETHERNET:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Ethernet-based communication is not yet implemented "
			"for record '%s'.", record->name );
		break;
	case MXI_RS232:
		interface_record =
		    mx_get_record( record, amptek_dp5->interface_arguments );

		if ( interface_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The requested RS232 record '%s' was not found "
			"in the MX database for record '%s'.",
				amptek_dp5->interface_arguments,
				record->name );
		}

		/* Set the RS-232 port to the configuration that we need. */

		mx_status = mx_rs232_set_configuration( interface_record,
				115200, 8, 'N', 1, 'N', 0x0, 0x0, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXI_USB:
		break;
	default:
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_amptek_dp5_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AMPTEK_DP5_FOO:
			record_field->process_function
					= mxi_amptek_dp5_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}


/*---*/

#define MXF_AMPTEK_DP5_CHECKSUM_OFFSET	6

static long
mxi_amptek_dp5_checksum( char *buffer_ptr, long num_bytes_to_checksum )
{
	long i, checksum;

	checksum = 0;

	for ( i = 0; i < num_bytes_to_checksum; i++ ) {
		checksum += buffer_ptr[i];
	}

	return checksum;
}

MX_EXPORT mx_status_type
mxi_amptek_dp5_binary_command( MX_AMPTEK_DP5 *amptek_dp5,
				char *binary_command,
				size_t binary_command_length,
				char *binary_response,
				size_t max_binary_response_length,
				unsigned long amptek_dp5_flags )
{
	/*---*/

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_amptek_dp5_ascii_command( MX_AMPTEK_DP5 *amptek_dp5,
				char *ascii_command,
				char *ascii_response,
				size_t max_ascii_response_length,
				unsigned long amptek_dp5_flags )
{
	static const char fname[] = "mxi_amptek_dp5_ascii_command()";

	char binary_command[530];
	char binary_response[530];
	long ascii_command_length, checksum_offset, checksum;
	mx_bool_type debug;
	mx_status_type mx_status;

	if ( amptek_dp5 == (MX_AMPTEK_DP5 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPTEK_DP5 pointer passed was NULL." );
	}

	if ( amptek_dp5_flags & MXF_AMPTEK_DP5_DEBUG ) {
		debug = TRUE;
	} else
	if ( amptek_dp5->amptek_dp5_flags & MXF_AMPTEK_DP5_DEBUG ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	ascii_command_length = strlen( ascii_command );

	memset( binary_command, 0, sizeof(binary_command) );

	/* Set synchronization bytes. */

	binary_command[0] = 0xf5;
	binary_command[1] = 0xfa;

	/* Declare this to be a 'Text configuration' command. */

	binary_command[2] = 0x20;
	binary_command[3] = 2;

	/* Specify the ASCII command length. */

	binary_command[4] = ( ascii_command_length >> 8 ) & 0xff;
	binary_command[5] = ascii_command_length & 0xff;

	if ( ascii_command_length == 0 ) {
		checksum_offset = MXF_AMPTEK_DP5_CHECKSUM_OFFSET;
	} else {
		checksum_offset = MXF_AMPTEK_DP5_CHECKSUM_OFFSET
					+ ascii_command_length;

		memcpy( binary_command + MXF_AMPTEK_DP5_CHECKSUM_OFFSET,
					ascii_command, ascii_command_length );
	}

	/* Compute the checksum for this command. */

	checksum = mxi_amptek_dp5_checksum( binary_command, checksum_offset );

	binary_command[ checksum_offset ] = ( checksum >> 8 ) & 0xff;
	binary_command[ checksum_offset + 1 ] = checksum & 0xff;

	/* Send the command and get the response (if any). */

	mx_status = mxi_amptek_dp5_binary_command( amptek_dp5,
						binary_command,
						ascii_command_length + 8,
						binary_response,
						sizeof(binary_response),
						amptek_dp5_flags );

	return mx_status;
}

/*---*/

/*==================================================================*/

static mx_status_type
mxi_amptek_dp5_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_amptek_dp5_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AMPTEK_DP5 *amptek_dp5;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	amptek_dp5 = (MX_AMPTEK_DP5 *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AMPTEK_DP5_FOO:
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
		case MXLV_AMPTEK_DP5_FOO:
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

