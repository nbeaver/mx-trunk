/*
 * Name:    i_modbus_serial_rtu.c
 *
 * Purpose: MX driver for MODBUS serial fieldbus communication using
 *          the RTU format.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_rs232.h"
#include "mx_modbus.h"
#include "i_modbus_serial_rtu.h"

MX_RECORD_FUNCTION_LIST mxi_modbus_serial_rtu_record_function_list = {
	NULL,
	mxi_modbus_serial_rtu_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_modbus_serial_rtu_open
};

MX_MODBUS_FUNCTION_LIST mxi_modbus_serial_rtu_modbus_function_list = {
	mxi_modbus_serial_rtu_command
};

MX_RECORD_FIELD_DEFAULTS mxi_modbus_serial_rtu_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MODBUS_STANDARD_FIELDS,
	MXI_MODBUS_SERIAL_RTU_STANDARD_FIELDS
};

long mxi_modbus_serial_rtu_num_record_fields
	= sizeof( mxi_modbus_serial_rtu_record_field_defaults )
	/ sizeof( mxi_modbus_serial_rtu_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_modbus_serial_rtu_rfield_def_ptr
			= &mxi_modbus_serial_rtu_record_field_defaults[0];

/* ---- */

/* Private functions for the use of the driver. */

static mx_status_type
mxi_modbus_serial_rtu_get_pointers( MX_MODBUS *modbus,
			MX_MODBUS_SERIAL_RTU **modbus_serial_rtu,
			const char *calling_fname )
{
	static const char fname[] = "mxi_modbus_serial_rtu_get_pointers()";

	MX_RECORD *modbus_serial_rtu_record;

	if ( modbus == (MX_MODBUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MODBUS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( modbus_serial_rtu == (MX_MODBUS_SERIAL_RTU **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MODBUS_SERIAL_RTU pointer passed by '%s' was NULL.",
			calling_fname );
	}

	modbus_serial_rtu_record = modbus->record;

	if ( modbus_serial_rtu_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The modbus_serial_rtu_record pointer for the "
			"MX_MODBUS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*modbus_serial_rtu = (MX_MODBUS_SERIAL_RTU *)
			modbus_serial_rtu_record->record_type_struct;

	if ( *modbus_serial_rtu == (MX_MODBUS_SERIAL_RTU *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MODBUS_SERIAL_RTU pointer for record '%s' is NULL.",
			modbus_serial_rtu_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* The following CRC calculation is from the document "MODBUS over serial line
 * specification and implementation guide V1.0" found on the modbus.org
 * web site.
 */

static void
mxi_modbus_serial_rtu_compute_crc( uint8_t *message_ptr,
				unsigned long data_length,
				uint8_t *crc_low_byte,
				uint8_t *crc_high_byte )
{
	static uint8_t crc_high_table[] = {
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
	0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
	0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
	0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
	0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
	};

	static uint8_t crc_low_table[] = {
	0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
	0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
	0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
	0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
	0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
	0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
	0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
	0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
	0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
	0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
	0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
	0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
	0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
	0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
	0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
	0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
	0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
	0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
	0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
	0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
	0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
	0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
	0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
	0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
	0x43, 0x83, 0x41, 0x81, 0x80, 0x40
	};

	uint8_t low_byte, high_byte;
	int i, n;

	low_byte = 0xff;
	high_byte = 0xff;

	for ( n = 0; n < data_length; n++ ) {

		i = low_byte ^ message_ptr[n];

		low_byte = high_byte ^ crc_high_table[i];

		high_byte = crc_low_table[i];
	}

	*crc_low_byte = low_byte;
	*crc_high_byte = high_byte;

	return;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_modbus_serial_rtu_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_modbus_serial_rtu_create_record_structures()";

	MX_MODBUS *modbus;
	MX_MODBUS_SERIAL_RTU *modbus_serial_rtu;

	/* Allocate memory for the necessary structures. */

	modbus = (MX_MODBUS *) malloc( sizeof(MX_MODBUS) );

	if ( modbus == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MODBUS structure." );
	}

	modbus_serial_rtu = (MX_MODBUS_SERIAL_RTU *)
				malloc( sizeof(MX_MODBUS_SERIAL_RTU) );

	if ( modbus_serial_rtu == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MODBUS_SERIAL_RTU structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = modbus;
	record->record_type_struct = modbus_serial_rtu;
	record->class_specific_function_list
			= &mxi_modbus_serial_rtu_modbus_function_list;

	modbus->record = record;
	modbus_serial_rtu->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_modbus_serial_rtu_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_modbus_serial_rtu_open()";

	MX_MODBUS *modbus;
	MX_MODBUS_SERIAL_RTU *modbus_serial_rtu;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	modbus = (MX_MODBUS *) record->record_class_struct;

	mx_status = mxi_modbus_serial_rtu_get_pointers( modbus,
						&modbus_serial_rtu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( modbus_serial_rtu->address > 247 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"MODBUS interface record '%s' is using illegal MODBUS serial "
		"address %lu.  The allowed values are from 0 to 247.",
			record->name, modbus_serial_rtu->address );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_modbus_serial_rtu_send_request( MX_MODBUS *modbus )
{
	static const char fname[] = "mxi_modbus_serial_rtu_send_request()";

	MX_MODBUS_SERIAL_RTU *modbus_serial_rtu;
	uint8_t crc_low_byte, crc_high_byte;
	uint8_t *ptr;
	mx_status_type mx_status;

	mx_status = mxi_modbus_serial_rtu_get_pointers( modbus,
						&modbus_serial_rtu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the MODBUS serial message.  The RTU format requires
	 * that the transmission be uninterrupted, so we must consolidate
	 * all of the data into one buffer before transmission.
	 */

	/* First, set the address field. */

	modbus_serial_rtu->send_buffer[0] = (uint8_t )
				(( modbus_serial_rtu->address ) & 0xff);

	/* Copy the MODBUS message body. */

	ptr = &(modbus_serial_rtu->send_buffer[1]);

	memcpy( ptr, modbus->request_pointer, modbus->request_length );

	/* Append the CRC value. */

	mxi_modbus_serial_rtu_compute_crc( modbus_serial_rtu->send_buffer,
					modbus->request_length + 1,
					&crc_low_byte, &crc_high_byte );

	ptr = &(modbus_serial_rtu->send_buffer[modbus->request_length + 1]);

	ptr[0] = crc_low_byte;
	ptr[1] = crc_high_byte;

	/* Send the message. */

	mx_status = mx_rs232_write( modbus_serial_rtu->rs232_record,
				(char *) modbus_serial_rtu->send_buffer,
				modbus->request_length + 3,
				NULL, 0 );
	return mx_status;
}

static mx_status_type
mxi_modbus_serial_rtu_receive_response( MX_MODBUS *modbus )
{
	static const char fname[] = "mxi_modbus_serial_rtu_receive_response()";

	MX_MODBUS_SERIAL_RTU *modbus_serial_rtu;
	size_t response_length, bytes_to_read;
	uint8_t response_address;
	uint8_t *message_ptr;
	mx_status_type mx_status;

	mx_status = mxi_modbus_serial_rtu_get_pointers( modbus,
						&modbus_serial_rtu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( modbus_serial_rtu->address == 0 ) {

		/* Messages sent to the broadcast address (0) will not
		 * have a response.
		 */

		modbus->actual_response_length = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* MODBUS serial RTU responses will always contain at least three bytes,
	 * namely, the station address followed by the first two byte of the
	 * MODBUS message body, so read them in order to figure out how long
	 * the rest of the message is.
	 */

	mx_status = mx_rs232_read( modbus_serial_rtu->rs232_record,
				(char *) modbus_serial_rtu->receive_buffer,
				3, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_modbus_compute_response_length( modbus->record,
					modbus_serial_rtu->receive_buffer + 1,
					&response_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the rest of the message.  We have already read two bytes
	 * of the message body.  However, we also need to read the trailing CRC
	 * which is also two bytes long, so the number of bytes to read
	 * equals the value returned for 'response_length'.
	 */

	bytes_to_read = response_length - 2 + 2;

	if ( bytes_to_read > (MXU_MODBUS_SERIAL_ADU_LENGTH - 3) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The MODBUS serial RTU message now being received by "
		"MODBUS serial RTU interface '%s' is "
		"of length %d bytes which is longer than the maximum "
		"allowed length of %d bytes.",
			modbus->record->name,
			(int)( bytes_to_read + 3 ),
			MXU_MODBUS_SERIAL_ADU_LENGTH );
	}

	message_ptr = modbus_serial_rtu->receive_buffer + 3;
	
	if ( bytes_to_read > 0 ) {
		mx_status = mx_rs232_read( modbus_serial_rtu->rs232_record,
					(char *) message_ptr, bytes_to_read,
					NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the MODBUS response pointer to point to the beginning
	 * of the transport independent part of the MODBUS message,
	 * namely, the function code value.
	 */

	modbus->response_pointer = modbus_serial_rtu->receive_buffer + 1;

	/* Did the response come from the address that it was supposed to? */

	response_address = modbus_serial_rtu->receive_buffer[0];

	if ( response_address != modbus_serial_rtu->address ) {
		mx_warning( "Response to MODBUS serial RTU command came from "
			"address %#x rather than the expected address %#lx.",
			response_address, modbus_serial_rtu->address );
	}

	/* We are now done, so return. */

	modbus->actual_response_length = response_length;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_modbus_serial_rtu_command( MX_MODBUS *modbus )
{
	mx_status_type mx_status;

	mx_status = mxi_modbus_serial_rtu_send_request( modbus );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_modbus_serial_rtu_receive_response( modbus );

	return mx_status;
}

