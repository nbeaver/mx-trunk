/*
 * Name:    i_amptek_dp4.c
 *
 * Purpose: MX driver for Amptek MCAs that use the DP4 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_AMPTEK_DP4_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_usb.h"
#include "i_amptek_dp4.h"

MX_RECORD_FUNCTION_LIST mxi_amptek_dp4_record_function_list = {
	NULL,
	mxi_amptek_dp4_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_amptek_dp4_open,
	NULL,
	NULL,
	NULL,
	mxi_amptek_dp4_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_amptek_dp4_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AMPTEK_DP4_STANDARD_FIELDS
};

long mxi_amptek_dp4_num_record_fields
		= sizeof( mxi_amptek_dp4_record_field_defaults )
			/ sizeof( mxi_amptek_dp4_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp4_rfield_def_ptr
			= &mxi_amptek_dp4_record_field_defaults[0];

/*---*/

static mx_status_type mxi_amptek_dp4_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*---*/

/*==================================================================*/

MX_EXPORT mx_status_type
mxi_amptek_dp4_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_amptek_dp4_create_record_structures()";

	MX_AMPTEK_DP4 *amptek_dp4;

	/* Allocate memory for the necessary structures. */

	amptek_dp4 = (MX_AMPTEK_DP4 *) malloc( sizeof(MX_AMPTEK_DP4) );

	if ( amptek_dp4 == (MX_AMPTEK_DP4 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPTEK_DP4 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = amptek_dp4;

	record->record_function_list = &mxi_amptek_dp4_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	amptek_dp4->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_amptek_dp4_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_amptek_dp4_open()";

	MX_AMPTEK_DP4 *amptek_dp4 = NULL;
	MX_RECORD *usb_record = NULL;
	unsigned long order_number;
	unsigned long flags;
	MX_USB_DEVICE *usb_device = NULL;
	mx_status_type mx_status;

#if MXI_AMPTEK_DP4_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	mx_status = MX_SUCCESSFUL_RESULT;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	amptek_dp4 = (MX_AMPTEK_DP4 *) record->record_type_struct;

	if ( amptek_dp4 == (MX_AMPTEK_DP4 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AMPTEK_DP4 pointer for record '%s' is NULL.", record->name);
	}

	flags = amptek_dp4->amptek_dp4_flags;

	usb_record = amptek_dp4->usb_record;

	if ( usb_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The usb_record pointer for Amptek DP4 '%s' is NULL.",
			record->name );
	}

	if ( flags & MXF_AMPTEK_DP4_FIND_BY_ORDER ) {

		/* If we have been requested to find the Amptek DP4
		 * using the order in which it is found in the
		 * enumerated devices, then we do that.
		 *
		 * WARNING: The order number can change depending
		 * on which USB port you plugged it into, the order
		 * in which you plugged usb devices in, and other
		 * fun things like the phase of the moon.  You are
		 * much better off using the serial number.
		 */

		order_number = atol( amptek_dp4->serial_number );

		mx_status = mx_usb_find_device_by_order(
						usb_record, &usb_device,
						MXT_AMPTEK_DP4_VENDOR_ID,
						MXT_AMPTEK_DP4_PRODUCT_ID,
						order_number,
						1, 0, 0, FALSE );
	} else {
		/* Otherwise, we look for the device using its
		 * serial number.  (Strongly preferred !)
		 */

		mx_status = mx_usb_find_device_by_serial_number(
						usb_record, &usb_device,
						MXT_AMPTEK_DP4_VENDOR_ID,
						MXT_AMPTEK_DP4_PRODUCT_ID,
						amptek_dp4->serial_number,
						1, 0, 0, FALSE );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0	/*----------------------- BEGIN ----------------------*/

		amptek_dp4->u.usb.usb_device = usb_device;
		break;
	default:
		break;
	}

	amptek_dp4->interface_record = interface_record;

	/* Verify that the Amptek DP4 is working by sending it a
	 * 'request status packet' message, which should return the
	 * actual status data into the array 'status_packet'.
	 */

	mx_status = mxi_amptek_dp4_binary_command( amptek_dp5, 1, 1,
							NULL, NULL,
							NULL, 0,
							status_packet,
							sizeof(status_packet),
							NULL, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amptek_dp4->firmware_version = status_packet[24];

	amptek_dp4->fpga_version = status_packet[25];

	amptek_dp4->serial_number = ( status_packet[29] << 24 )
				+ ( status_packet[28] << 16 )
				+ ( status_packet[27] << 8 )
				+ status_packet[26];

	/* If requested, reset Amptek DP4 parameters to their defaults. */

	if ( flags & MXF_AMPTEK_DP4_RESET_TO_DEFAULTS ) {
		mx_status = mxi_amptek_dp4_ascii_command( amptek_dp5,
						"RESC=Y;", NULL, 0, TRUE,
						amptek_dp4->amptek_dp5_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( strlen( amptek_dp4->load_filename ) > 0 ) {
		mx_status = mxi_amptek_dp4_load_filename( amptek_dp5,
						amptek_dp4->load_filename );
	}

#endif	/*----------------------- END ----------------------*/
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_amptek_dp4_special_processing_setup( MX_RECORD *record )
{
	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case 0:
			record_field->process_function
					= mxi_amptek_dp4_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*==================================================================*/

static mx_status_type
mxi_amptek_dp4_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxi_amptek_dp4_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AMPTEK_DP4 *amptek_dp4;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	amptek_dp4 = (MX_AMPTEK_DP4 *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case 0:
			/* Just return the string most recently written
			 * by the process function.
			 */
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
		case 0:
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

#if 0	/*----------------------- BEGIN ----------------------*/

/*==================================================================*/

static long
mxi_amptek_dp4_checksum( char *buffer_ptr, long num_bytes_to_checksum )
{
	unsigned long i, checksum, sum;
	uint8_t buffer_byte;

	sum = 0;

	for ( i = 0; i < num_bytes_to_checksum; i++ ) {
		buffer_byte = (uint8_t) (buffer_ptr[i]);
		sum += buffer_byte;
#if 0
		MX_DEBUG(-2,("cksum: buffer_byte(%lu) = %#lx, sum = %#lx",
			i, (unsigned long) buffer_byte, sum));
#endif
	}

	/* Now compute the 16-bit two's complement of the sum. */

	checksum = 0x10000 - sum;

	checksum &= 0xffff;

#if 0
	MX_DEBUG(-2,("cksum: checksum = %#lx", checksum));
#endif

	return checksum;
}

MX_EXPORT mx_status_type
mxi_amptek_dp4_ascii_command( MX_AMPTEK_DP4 *amptek_dp5,
				char *ascii_command,
				char *ascii_response,
				unsigned long max_ascii_response_length,
				mx_bool_type change_default,
				unsigned long amptek_dp4_flags )
{
	static const char fname[] = "mxi_amptek_dp4_ascii_command()";

	char raw_command[MXU_AMPTEK_DP4_MAX_WRITE_PACKET_LENGTH+1];
	char raw_response[MXU_AMPTEK_DP4_MAX_READ_PACKET_LENGTH+1];
	unsigned long ascii_command_length, ascii_response_length;
	unsigned long command_checksum, response_checksum;
	unsigned long command_checksum_offset, response_checksum_offset;
	unsigned long computed_raw_response_length, actual_raw_response_length;
	unsigned long computed_response_checksum;
	mx_bool_type debug, ascii_readback_expected;
	mx_status_type mx_status;

	if ( amptek_dp4 == (MX_AMPTEK_DP4 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPTEK_DP4 pointer passed was NULL." );
	}

	if ( amptek_dp4_flags & MXF_AMPTEK_DP4_DEBUG_ASCII ) {
		debug = TRUE;
	} else
	if ( amptek_dp4->amptek_dp5_flags & MXF_AMPTEK_DP4_DEBUG_ASCII ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	/* Infer whether or not an ASCII readback is expected. */

	if ( ( ascii_response == NULL) || ( max_ascii_response_length == 0 ) ) {
		ascii_readback_expected = FALSE;
	} else {
		ascii_readback_expected = TRUE;
	}

	ascii_command_length = strlen( ascii_command );

	memset( raw_command, 0, sizeof(raw_command) );

	/* Set synchronization bytes. */

	raw_command[0] = 0xf5;
	raw_command[1] = 0xfa;

	/* Declare this to be some form of a 'Text configuration' command. */

	raw_command[2] = 0x20;

	if ( ascii_readback_expected ) {
		raw_command[3] = 3;
	} else
	if ( change_default ) {
		raw_command[3] = 2;
	} else {
		raw_command[3] = 4;
	}

	/* Specify the ASCII command length. */

	raw_command[4] = ( ascii_command_length >> 8 ) & 0xff;
	raw_command[5] = ascii_command_length & 0xff;

	/* Make sure that our caller is not trying to use a buffer
	 * that is too long.
	 */

	if ( ascii_command_length > MXU_AMPTEK_DP4_MAX_WRITE_DATA_LENGTH ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"ASCII command '%s' for Amptek DP4 interface '%s' "
		"is longer (%ld) than the maximum allowed length (%d) for "
		"an Amptek DP4 interface.",
			ascii_command, amptek_dp4->record->name,
			ascii_command_length,
			MXU_AMPTEK_DP4_MAX_WRITE_DATA_LENGTH );
	}

	/* Copy in the ASCII data. */

	memcpy( raw_command + MXU_AMPTEK_DP4_HEADER_LENGTH,
				ascii_command, ascii_command_length );

	/* Compute the checksum for this command. */

	command_checksum_offset = MXU_AMPTEK_DP4_HEADER_LENGTH
					+ ascii_command_length;

	command_checksum =
	    mxi_amptek_dp4_checksum( raw_command, command_checksum_offset );

	raw_command[ command_checksum_offset ] = (command_checksum >> 8) & 0xff;
	raw_command[ command_checksum_offset + 1 ] = command_checksum & 0xff;

	if ( debug ) {
		fprintf( stderr, "Sending '%s' to '%s'.\n",
			ascii_command, amptek_dp4->record->name );
	}

	/* Send the command and get the response (if any). */

	mx_status = mxi_amptek_dp4_raw_command( amptek_dp5,
						raw_command,
						raw_response,
					MXU_AMPTEK_DP4_MAX_READ_PACKET_LENGTH,
						&actual_raw_response_length,
						amptek_dp4_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the caller has not requested an ASCII response string,
	 * then we are done and can return now.
	 */

	if ( ascii_readback_expected == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* How long is the ASCII response? */

	ascii_response_length = ( (raw_response[4] & 0xff) << 8 )
					+ ( raw_response[5] & 0xff );

	/* Is the ASCII response length consistent with the raw length? */

	computed_raw_response_length = ascii_response_length
				+ MXU_AMPTEK_DP4_HEADER_AND_CHECKSUM_LENGTH;

	if ( computed_raw_response_length != actual_raw_response_length ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The computed raw response length (%ld) does not match the "
		"raw response length (%ld) returned by "
		"mxi_amptek_dp4_raw_command() for ASCII command '%s' used by "
		"Amptek DP4 interface '%s'.",
			computed_raw_response_length,
			actual_raw_response_length,
			ascii_command, amptek_dp4->record->name );
	}

	/* Is the returned checksum correct? */

	response_checksum_offset = MXU_AMPTEK_DP4_HEADER_LENGTH
						+ ascii_response_length;

	computed_response_checksum =
	    mxi_amptek_dp4_checksum( raw_response, response_checksum_offset );

	response_checksum =
		( ( raw_response[ response_checksum_offset ] & 0xff ) << 8 )
		+ ( raw_response[ response_checksum_offset + 1 ] & 0xff );

	if ( response_checksum != computed_response_checksum ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The checksum (%ld) sent in the response to ASCII command '%s' "
		"for Amptek DP4 interface '%s' does not match the "
		"checksum (%ld) that we compute from the returned packet.",
			response_checksum, ascii_command,
			amptek_dp4->record->name,
			computed_response_checksum );
	}

	/* Copy back the ASCII response. */

	if ( ascii_response_length >= max_ascii_response_length ) {

		/* Make sure we leave room for a null terminator at the end. */

		ascii_response_length = max_ascii_response_length - 1;
	}

	memcpy( ascii_response,
		raw_response + MXU_AMPTEK_DP4_HEADER_LENGTH,
		ascii_response_length );

	/* Make sure that it is null terminated. */

	ascii_response[ascii_response_length] = '\0';

	if ( debug ) {
		fprintf( stderr, "Received '%s' from '%s'.\n",
			ascii_response, amptek_dp4->record->name );
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mxi_amptek_dp4_binary_command( MX_AMPTEK_DP4 *amptek_dp5,
				long pid1, long pid2,
				long *response_pid1,
				long *response_pid2,
				char *binary_command,
				unsigned long binary_command_length,
				char *binary_response,
				unsigned long max_binary_response_length,
				unsigned long *actual_binary_response_length,
				unsigned long amptek_dp4_flags )
{
	static const char fname[] = "mxi_amptek_dp4_binary_command()";

	char raw_command[MXU_AMPTEK_DP4_MAX_WRITE_PACKET_LENGTH+1];
	char raw_response[MXU_AMPTEK_DP4_MAX_READ_PACKET_LENGTH+1];
	unsigned long binary_response_data_length;
	unsigned long command_checksum, response_checksum;
	unsigned long command_checksum_offset, response_checksum_offset;
	unsigned long max_raw_response_packet_length;
	unsigned long computed_raw_response_packet_length;
	unsigned long actual_raw_response_packet_length;
	unsigned long computed_response_checksum;
	mx_status_type mx_status;

	if ( amptek_dp4 == (MX_AMPTEK_DP4 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPTEK_DP4 pointer passed was NULL." );
	}

	memset( raw_command, 0, sizeof(raw_command) );

	/* Set the synchronization bytes. */

	raw_command[0] = 0xf5;
	raw_command[1] = 0xfa;

	/* Specify the command type. */

	raw_command[2] = pid1;
	raw_command[3] = pid2;

	/* Specify the command length. */

	raw_command[4] = ( binary_command_length >> 8 ) & 0xff;
	raw_command[5] = ( binary_command_length & 0xff );

	/* Make sure that our caller is not trying to use a buffer
	 * that is too long.
	 */

	if ( binary_command_length > MXU_AMPTEK_DP4_MAX_WRITE_DATA_LENGTH ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The binary command for Amptek DP4 interface '%s' "
		"is longer (%ld) than the maximum allowed length (%d) for "
		"an Amptek DP4 interface.",
			amptek_dp4->record->name,
			binary_command_length,
			MXU_AMPTEK_DP4_MAX_WRITE_DATA_LENGTH );
	}

	/* Copy in the binary data. */

	memcpy( raw_command + MXU_AMPTEK_DP4_HEADER_LENGTH,
				binary_command, binary_command_length );

	/* Compute the checksum for this command. */

	command_checksum_offset = MXU_AMPTEK_DP4_HEADER_LENGTH
					+ binary_command_length;

	command_checksum =
	    mxi_amptek_dp4_checksum( raw_command, command_checksum_offset );

	raw_command[ command_checksum_offset ] = (command_checksum >> 8) & 0xff;
	raw_command[ command_checksum_offset + 1 ] = command_checksum & 0xff;

	/* Send the command and get the response (if any). */

	max_raw_response_packet_length = max_binary_response_length
		+ MXU_AMPTEK_DP4_HEADER_AND_CHECKSUM_LENGTH;

	mx_status = mxi_amptek_dp4_raw_command( amptek_dp5,
					raw_command,
					raw_response,
					max_raw_response_packet_length,
					&actual_raw_response_packet_length,
					amptek_dp4_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response_pid1 != (long *) NULL ) {
		*response_pid1 = raw_response[2] & 0xff;
	}
	if ( response_pid2 != (long *) NULL ) {
		*response_pid2 = raw_response[3] & 0xff;
	}

	/* If the caller has not requested a binary response string,
	 * then we are done and can return now.
	 */

	if (( binary_response == NULL) || ( max_binary_response_length == 0 )) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* How long is the binary response? */

	binary_response_data_length = ( (raw_response[4] & 0xff) << 8 )
					+ ( raw_response[5] & 0xff );

	/* Is the binary response data length consistent with the raw length? */

	computed_raw_response_packet_length = binary_response_data_length
				+ MXU_AMPTEK_DP4_HEADER_AND_CHECKSUM_LENGTH;

	if ( computed_raw_response_packet_length
			!= actual_raw_response_packet_length )
	{
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The computed raw response packet length (%ld) does not "
		"match the raw response packet length (%ld) returned by "
		"mxi_amptek_dp4_raw_command() for the binary command used by "
		"Amptek DP4 interface '%s'.",
			computed_raw_response_packet_length,
			actual_raw_response_packet_length,
			amptek_dp4->record->name );
	}

	/* Is the returned checksum correct? */

	response_checksum_offset = MXU_AMPTEK_DP4_HEADER_LENGTH
						+ binary_response_data_length;

	computed_response_checksum =
	    mxi_amptek_dp4_checksum( raw_response, response_checksum_offset );

	response_checksum =
		( ( raw_response[ response_checksum_offset ] & 0xff ) << 8 )
		+ ( raw_response[ response_checksum_offset + 1 ] & 0xff );

	if ( response_checksum != computed_response_checksum ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The checksum (%ld) sent in the response to a binary command "
		"for Amptek DP4 interface '%s' does not match the "
		"checksum (%ld) that we compute from the returned packet.",
			response_checksum, 
			amptek_dp4->record->name,
			computed_response_checksum );
	}

	/* Copy back the binary response. */

	if ( binary_response_data_length > max_binary_response_length ) {
		binary_response_data_length = max_binary_response_length;
	}

	memcpy( binary_response,
		raw_response + MXU_AMPTEK_DP4_HEADER_LENGTH,
		binary_response_data_length );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxi_amptek_dp4_raw_command( MX_AMPTEK_DP4 *amptek_dp5,
				char *raw_command,
				char *raw_response,
				unsigned long max_raw_response_length,
				unsigned long *actual_raw_response_length,
				unsigned long amptek_dp4_flags )
{
	static const char fname[] = "mxi_amptek_dp4_raw_command()";

	char local_raw_response[MXU_AMPTEK_DP4_MAX_READ_PACKET_LENGTH+1];
	char *raw_response_ptr = NULL;
	unsigned long i;
	unsigned long raw_read_data_length, raw_write_data_length;
	unsigned long raw_read_packet_length, raw_write_packet_length;
	unsigned long response_pid_1, response_pid_2;
	mx_bool_type debug;
	mx_status_type mx_status;

	if ( amptek_dp4 == (MX_AMPTEK_DP4 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPTEK_DP4 pointer passed was NULL." );
	}

	if ( raw_command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The raw_command pointer for Amptek DP4 '%s' is NULL.",
			amptek_dp4->record->name );
	}

	if ( raw_response == (char *) NULL ) {
		raw_response_ptr = local_raw_response;
		max_raw_response_length = MXU_AMPTEK_DP4_MAX_READ_PACKET_LENGTH;
	} else {
		raw_response_ptr = raw_response;
	}

	if ( amptek_dp4_flags & MXF_AMPTEK_DP4_DEBUG_RAW ) {
		debug = TRUE;
	} else
	if ( amptek_dp4->amptek_dp5_flags & MXF_AMPTEK_DP4_DEBUG_RAW ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	raw_write_data_length = ( raw_command[4] << 8 ) + raw_command[5];

	raw_write_packet_length = raw_write_data_length
				+ MXU_AMPTEK_DP4_HEADER_AND_CHECKSUM_LENGTH;

	if ( debug ) {
		unsigned long max_show_write_bytes;
		uint8_t byte_to_write;

		if ( raw_write_packet_length >
				MXU_AMPTEK_DP4_MAX_SHOW_WRITE_BYTES)
		{
			max_show_write_bytes =
				MXU_AMPTEK_DP4_MAX_SHOW_WRITE_BYTES;
		} else {
			max_show_write_bytes = raw_write_packet_length;
		}

		fprintf(stderr, "Sending to '%s': ", amptek_dp4->record->name);

		if ( max_show_write_bytes > 0 ) {
			byte_to_write = raw_command[0];

			fprintf(stderr, "%#hx",
					(unsigned short) byte_to_write );
		}
		for ( i = 1; i < max_show_write_bytes; i++ ) {
			byte_to_write = raw_command[i];

			fprintf(stderr, ", %#hx",
					(unsigned short) byte_to_write );
		}

		fprintf( stderr, "\n" );

	}

	/* Send the raw command. */

	switch( amptek_dp4->interface_type ) {
	case MXI_ETHERNET:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Ethernet-based communicationis not yet implemented "
			"for record '%s'.", amptek_dp4->record->name );
		break;
	case MXI_RS232:
		mx_status = mx_rs232_write( amptek_dp4->interface_record,
						raw_command,
						raw_write_packet_length,
						NULL, 0 );
		break;
	case MXI_USB:
		mx_status = mx_usb_bulk_write( amptek_dp4->u.usb.usb_device,
						MXT_AMPTEK_DP4_WRITE_ENDPOINT,
						raw_command,
						raw_write_packet_length,
						NULL, amptek_dp4->timeout );
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported interface type (%ld) requested for "
		"Amptek DP4 interface '%s'.",
			amptek_dp4->interface_type,
			amptek_dp4->record->name );
		break;
	}

	/* Read back the raw response. */

	switch( amptek_dp4->interface_type ) {
	case MXI_ETHERNET:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Ethernet-based communicationis not yet implemented "
			"for record '%s'.", amptek_dp4->record->name );
		break;
	case MXI_RS232:
		mx_status = mx_rs232_read_with_timeout(
						amptek_dp4->interface_record,
						raw_response_ptr,
						max_raw_response_length,
						NULL, 0, amptek_dp4->timeout );
		break;
	case MXI_USB:
		mx_status = mx_usb_bulk_read( amptek_dp4->u.usb.usb_device,
						MXT_AMPTEK_DP4_READ_ENDPOINT,
						raw_response_ptr,
						max_raw_response_length,
						NULL, amptek_dp4->timeout);
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported interface type (%ld) requested for "
		"Amptek DP4 interface '%s'.",
			amptek_dp4->interface_type,
			amptek_dp4->record->name );
		break;
	}

	/*---*/

	raw_read_data_length =
		( raw_response_ptr[4] << 8 ) + raw_response_ptr[5];

	raw_read_packet_length = raw_read_data_length
				+ MXU_AMPTEK_DP4_HEADER_AND_CHECKSUM_LENGTH;

	if ( debug ) {
		unsigned long max_show_read_bytes;
		uint8_t byte_read;

		if ( raw_read_packet_length >
				MXU_AMPTEK_DP4_MAX_SHOW_READ_BYTES)
		{
			max_show_read_bytes =
				MXU_AMPTEK_DP4_MAX_SHOW_READ_BYTES;
		} else {
			max_show_read_bytes = raw_read_packet_length;
		}

		fprintf( stderr,
			"Received from '%s': ",
			amptek_dp4->record->name );

		if ( max_show_read_bytes > 0 ) {
			byte_read = raw_response_ptr[0];

			fprintf(stderr, "%#hx", (unsigned short) byte_read );
		}
		for ( i = 1; i < max_show_read_bytes; i++ ) {
			byte_read = raw_response_ptr[i];

			fprintf(stderr, ", %#hx", (unsigned short) byte_read );
		}

		fprintf( stderr, "\n" );
	}

	/* Check to see if there was an error. */

	response_pid_1 = ( (unsigned long) raw_response_ptr[2] ) & 0xff;
	response_pid_2 = ( (unsigned long) raw_response_ptr[3] ) & 0xff;

	if ( response_pid_1 == 0xff ) {
		char errname[80];

		switch( response_pid_2 ) {
		case 0x0:	/* OK */
		case 0x0C:	/* OK, with Interface Sharing Request. */
		    break;

		case 0x5:	/* Bad Parameter. */

		    return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "The command sent to Amptek DP4 interface '%s' had a "
		    "bad parameter value.", amptek_dp4->record->name );
		    break;

		case 0x7:	/* Unrecognized Command. */

		    return mx_error( MXE_ILLEGAL_COMMAND, fname,
		    "The command sent to Amptek DP4 '%s' was not a "
		    "valid command.", amptek_dp4->record->name );
		    break;

		default:
		    /* Yes, we _are_ nesting switches that use the same switch
		     * control variable 'response_pid_2'.  It makes it
		     * easier to handle the case of more than 1 'OK' response.
		     *
		     */
		    switch( response_pid_2 ) {
		    case 0x1:
			strlcpy( errname, "Sync Error", sizeof(errname) );
			break;
		    case 0x2:
			strlcpy( errname, "PID Error", sizeof(errname) );
			break;
		    case 0x3:
			strlcpy( errname, "LEN Error", sizeof(errname) );
			break;
		    case 0x4:
			strlcpy( errname, "Checksum Error", sizeof(errname) );
			break;
		    case 0x5:
			strlcpy( errname, "Bad Parameter", sizeof(errname) );
			break;
		    case 0x6:
			strlcpy( errname, "Bad Hex Record", sizeof(errname) );
			break;
		    case 0x7:
			strlcpy( errname, "Unrecognized Command",
						sizeof(errname) );
			break;
		    case 0x8:
			strlcpy( errname, "FPGA Error", sizeof(errname) );
			break;
		    case 0x9:
			strlcpy( errname, "CP2201 Not Found (No Ethernet)",
						sizeof(errname) );
			break;
		    case 0x0A:
			strlcpy( errname, "Scope Data Not Available",
						sizeof(errname) );
			break;
		    case 0x0B:
			strlcpy( errname, "PC5 Not Present", sizeof(errname) );
			break;
		    case 0x0E:
			strlcpy( errname, "I2C Error", sizeof(errname) );
			break;
		    case 0x10:
			strlcpy( errname,
				"Feature not supported by this FPGA version",
				sizeof(errname) );
			break;
		    case 0x11:
			strlcpy( errname, "Calibration data not present",
						sizeof(errname) );
			break;
		    default:
			snprintf( errname, sizeof(errname),
				"Unrecognized error (%#lx)",
				(unsigned long) response_pid_2 );
			break;
		    }

		    return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		    "A command sent to Amptek DP4 '%s' resulted in "
		    "error code (%#lx) '%s'.",
		    	amptek_dp4->record->name,
			response_pid_2, errname );
		}
	}

	if ( actual_raw_response_length != NULL ) {
		*actual_raw_response_length = raw_read_packet_length;
	}

	MXW_UNUSED( mx_status );

	return MX_SUCCESSFUL_RESULT;
}

#endif	/*----------------------- END ----------------------*/
