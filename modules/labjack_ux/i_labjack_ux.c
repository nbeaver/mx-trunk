/*
 * Name:    i_labjack_ux.c
 *
 * Purpose: MX interface driver for the LabJack U3, U6, and U9 devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LABJACK_UX_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_labjack_ux.h"

MX_RECORD_FUNCTION_LIST mxi_labjack_ux_record_function_list = {
	NULL,
	mxi_labjack_ux_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_labjack_ux_open,
	mxi_labjack_ux_close
};

MX_RECORD_FIELD_DEFAULTS mxi_labjack_ux_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LABJACK_UX_STANDARD_FIELDS
};

long mxi_labjack_ux_num_record_fields
		= sizeof( mxi_labjack_ux_record_field_defaults )
			/ sizeof( mxi_labjack_ux_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_labjack_ux_rfield_def_ptr
			= &mxi_labjack_ux_record_field_defaults[0];

static mx_status_type
mxi_labjack_ux_get_pointers( MX_RECORD *record,
				MX_LABJACK_UX **labjack_ux,
				const char *calling_fname )
{
	static const char fname[] = "mxi_labjack_ux_get_pointers()";

	MX_LABJACK_UX *labjack_ux_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	labjack_ux_ptr = (MX_LABJACK_UX *) record->record_type_struct;

	if ( labjack_ux_ptr == (MX_LABJACK_UX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LABJACK_UX pointer for record '%s' is NULL.",
			record->name );
	}

	if ( labjack_ux != (MX_LABJACK_UX **) NULL ) {
		*labjack_ux = labjack_ux_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_labjack_ux_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_labjack_ux_create_record_structures()";

	MX_LABJACK_UX *labjack_ux;

	/* Allocate memory for the necessary structures. */

	labjack_ux = (MX_LABJACK_UX *) malloc( sizeof(MX_LABJACK_UX) );

	if ( labjack_ux == (MX_LABJACK_UX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_LABJACK_UX structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = labjack_ux;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	labjack_ux->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_labjack_ux_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_labjack_ux_open()";

	MX_LABJACK_UX *labjack_ux;
	int i, length;
	uint8_t command[256];
	uint8_t response[256];
	mx_status_type mx_status;

	mx_status = mxi_labjack_ux_get_pointers( record, &labjack_ux, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen( labjack_ux->product_name );

	for ( i = 0; i < length; i++ ) {
		if ( islower( labjack_ux->product_name[i] ) ) {
			labjack_ux->product_name[i] =
				toupper( labjack_ux->product_name[i] );
		}
	}

	if ( strcmp( labjack_ux->product_name, "U3" ) == 0 ) {
		labjack_ux->product_id = U3_PRODUCT_ID;
	} else
	if ( strcmp( labjack_ux->product_name, "U6" ) == 0 ) {
		labjack_ux->product_id = U6_PRODUCT_ID;
	} else
	if ( strcmp( labjack_ux->product_name, "UE9" ) == 0 ) {
		labjack_ux->product_id = UE9_PRODUCT_ID;
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"LabJack product '%s' is not supported by this driver.",
			labjack_ux->product_name );
	}

	labjack_ux->timeout_msec = 5000;	/* 5 seconds */

#if defined(OS_WIN32)
#error foo
#else /* NOT defined(OS_WIN32) */

	if ( labjack_ux->labjack_flags & MXF_LABJACK_UX_SHOW_INFO ) {
		mx_info( "LabJack library version = %f",
			LJUSB_GetLibraryVersion() );

		mx_info( "This computer has %u '%s' devices connected.",
			LJUSB_GetDevCount( labjack_ux->product_id ),
			labjack_ux->product_name );
	}

#endif /* NOT defined(OS_WIN32) */

#if defined(OS_WIN32)
#error bar
#else
	labjack_ux->handle = LJUSB_OpenDevice( labjack_ux->device_number,
						0, labjack_ux->product_id );

	if ( labjack_ux->handle == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The attempt by record '%s' to connect to device %lu "
		"(product '%s') failed.",
			record->name,
			labjack_ux->device_number,
			labjack_ux->product_name );
	}

#if MXI_LABJACK_UX_DEBUG
	MX_DEBUG(-2,("%s: handle = %p", fname, labjack_ux->handle));
#endif

#endif
	/* Read the configuration of the device. */

	switch( labjack_ux->product_id ) {
	case U3_PRODUCT_ID:
		/* Send a ConfigU3 command. */

		memset( command, 0, sizeof(command) );

		command[1] = 0xF8;	/* extended command */
		command[2] = 0x0A;	/* data length = 10 */
		command[3] = 0x08;	/* command number = 8 */

		/* Set write mask to read-only. */
		command[6] = 0x0;
		command[7] = 0x0;

		mx_status = mxi_labjack_ux_write( labjack_ux, command, 26, -1 );
		
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_labjack_ux_read( labjack_ux, response, 38, -1 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_LABJACK_UX_DEBUG
		mx_info( "'%s' firmware_version = %u",
			labjack_ux->record->name,
			256 * response[10] + response[9] );

		mx_info( "'%s' bootloader_version = %u",
			labjack_ux->record->name,
			256 * response[12] + response[11] );

		mx_info( "'%s' hardware_version = %u",
			labjack_ux->record->name,
			256 * response[14] + response[13] );

		mx_info( "'%s' serial_number = %u",
			labjack_ux->record->name,
			16384 * response[18] + 2048 * response[17]
			+ 256 * response[16] + response[15] );

		mx_info( "'%s' productID = %u",
			labjack_ux->record->name,
			256 * response[20] + response[19] );

		mx_info( "'%s' localID = %u",
			labjack_ux->record->name,
			response[21] );
#endif
		break;

	default:
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_labjack_ux_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_labjack_ux_close()";

	MX_LABJACK_UX *labjack_ux;
	mx_status_type mx_status;

	mx_status = mxi_labjack_ux_get_pointers( record, &labjack_ux, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if defined( OS_WIN32 )
	if ( labjack_ux->handle != INVALID_HANDLE ) {
		Close( labjack_ux->handle );
	}
#else
	if ( LJUSB_IsHandleValid( labjack_ux->handle ) ) {
		LJUSB_CloseDevice( labjack_ux->handle );
	}
#endif
	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_labjack_ux_read( MX_LABJACK_UX *labjack_ux,
			uint8_t *buffer,
			unsigned long num_bytes_to_read,
			long timeout_msec )
{
	static const char fname[] = "mxi_labjack_ux_read()";

	unsigned long returned_value;
	int saved_errno;

	if ( timeout_msec < 0 ) {
		timeout_msec = labjack_ux->timeout_msec;
	}

#if defined(OS_WIN32)

	returned_value = ePut( labjack_ux->handle,
				LJ_ioRAW_IN, 0, buffer, 0 );

#else /* NOT OS_WIN32 */

	returned_value = LJUSB_ReadTO( labjack_ux->handle, buffer,
				num_bytes_to_read, timeout_msec );

	if ( returned_value == 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"An error occurred while reading from LabJack device '%s'.  "
		"Errno = %d, error message = '%s'",
			labjack_ux->record->name,
			saved_errno, strerror(saved_errno) );
	}

#endif /* NOT OS_WIN32 */

#if MXI_LABJACK_UX_DEBUG
	fprintf( stderr, "%s: read %lu bytes from '%s': ",
		fname, returned_value, labjack_ux->record->name );
	{
		int i;

		for ( i = 0; i < returned_value; i++ ) {
			fprintf( stderr, "%#x ",
				(unsigned int) buffer[i] );
		}

		fprintf( stderr, "\n" );
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_labjack_u3_setup_checksum( MX_LABJACK_UX *labjack_ux,
				uint8_t *buffer,
				unsigned long num_bytes_to_write )
{
	static const char fname[] = "mxi_labjack_u3_setup_checksum()";

	uint8_t checksum8;
	uint16_t checksum16;

	uint8_t command_byte;
	uint16_t byte_value16, accumulator, quotient, remainder;
	unsigned long i, command_length;
	mx_bool_type is_extended_command;

	/* Is this an extended command? */

	command_byte = buffer[1];

	if ( ( command_byte & 0x78 ) == 0x78 ) {
		is_extended_command = TRUE;
	} else {
		is_extended_command = FALSE;
	}

	if ( is_extended_command == FALSE ) {
		/* Normal command format */

		command_length = 2 + ( command_byte & 0x7 );

		if ( command_length > num_bytes_to_write ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The command length (%lu) specified in the command "
			"to record '%s' is longer than the length of the "
			"buffer (%lu) passed to this function.",
				command_length,
				labjack_ux->record->name,
				num_bytes_to_write );
		}

		if ( command_length > 16 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The command length (%lu) for record '%s' is longer "
			"than the maximum value (16) for a normal command.",
				command_length,
				labjack_ux->record->name );
		}

		/* Compute the 8-bit normal command checksum. */

		accumulator = 0;

		for ( i = 1; i < command_length; i++ ) {
			byte_value16 = (uint16_t) buffer[i];

			accumulator += byte_value16;
		}

		quotient  = accumulator / 0x100;
		remainder = accumulator % 0x100;

		checksum8 = quotient + remainder;

		buffer[0] = (uint8_t) ( checksum8 & 0xff );
	} else {
		/* Extended command format */

		command_length = 2 + buffer[2];

		if ( command_length > num_bytes_to_write ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The command length (%lu) specified in the command "
			"to record '%s' is longer than the length of the "
			"buffer (%lu) passed to this function.",
				command_length,
				labjack_ux->record->name,
				num_bytes_to_write );
		}

		if ( command_length > 256 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The command length (%lu) for record '%s' is longer "
			"than the maximum value (256) for a normal command.",
				command_length,
				labjack_ux->record->name );
		}

		/* Compute 16-bit extended command checksum */

		accumulator = 0;

		for ( i = 6; i < command_length; i++ ) {
			byte_value16 = (uint16_t) buffer[i];

			accumulator += byte_value16;
		}

		checksum16 = accumulator;

		buffer[4] = (uint8_t) ( checksum16 & 0xff );
		buffer[5] = (uint8_t) ( (checksum16 >> 8) & 0xff );

		/* Compute the 8-bit extended command checksum. */

		accumulator = 0;

		for ( i = 1; i < 6; i++ ) {
			byte_value16 = (uint16_t) buffer[i];

			accumulator += byte_value16;
		}

		quotient  = accumulator / 0x100;
		remainder = accumulator % 0x100;

		checksum8 = quotient + remainder;

		buffer[0] = (uint8_t) ( checksum8 & 0xff );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_labjack_ux_write( MX_LABJACK_UX *labjack_ux,
			uint8_t *buffer,
			unsigned long num_bytes_to_write,
			long timeout_msec )
{
	static const char fname[] = "mxi_labjack_ux_write()";

	unsigned long returned_value;
	int saved_errno;
	mx_status_type mx_status;

	if ( timeout_msec < 0 ) {
		timeout_msec = labjack_ux->timeout_msec;
	}

	switch( labjack_ux->product_id ) {
	case U3_PRODUCT_ID:
		mx_status = mxi_labjack_u3_setup_checksum( labjack_ux, buffer,
							num_bytes_to_write );
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Support for LabJack product ID %lu used by record '%s' "
		"is not yet implemented.",
			labjack_ux->product_id, labjack_ux->record->name );
		break;
	}

#if MXI_LABJACK_UX_DEBUG
	fprintf( stderr, "%s: writing %lu bytes to '%s': ",
		fname, num_bytes_to_write, labjack_ux->record->name );
	{
		int i;

		for ( i = 0; i < num_bytes_to_write; i++ ) {
			fprintf( stderr, "%#x ",
				(unsigned int) buffer[i] );
		}

		fprintf( stderr, "\n" );
	}
#endif

#if defined(OS_WIN32)

	returned_value = eGet( labjack_ux->handle,
				LJ_ioRAW_OUT, 0, buffer, 0 );

#else /* NOT OS_WIN32 */

	returned_value = LJUSB_WriteTO( labjack_ux->handle, buffer,
				num_bytes_to_write, timeout_msec );

	if ( returned_value == 0 ) {
		saved_errno = errno;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"An error occurred while writing to LabJack device '%s'.  "
		"Errno = %d, error message = '%s'",
			labjack_ux->record->name,
			saved_errno, strerror(saved_errno) );
	}

#endif /* NOT OS_WIN32 */

	return MX_SUCCESSFUL_RESULT;
}

