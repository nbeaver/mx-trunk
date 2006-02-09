/*
 * Name:    i_micro488ex.c
 *
 * Purpose: MX driver for the IOtech Micro488EX RS232 to GPIB interface
 *          used in System Controller mode.
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

#define MICRO488EX_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_gpib.h"
#include "mx_rs232.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "i_micro488ex.h"

MX_RECORD_FUNCTION_LIST mxi_micro488ex_record_function_list = {
	NULL,
	mxi_micro488ex_create_record_structures,
	mxi_micro488ex_finish_record_initialization,
	NULL,
	mxi_micro488ex_print_interface_structure,
	NULL,
	NULL,
	mxi_micro488ex_open
};

MX_GPIB_FUNCTION_LIST mxi_micro488ex_gpib_function_list = {
	mxi_micro488ex_open_device,
	mxi_micro488ex_close_device,
	mxi_micro488ex_read,
	mxi_micro488ex_write,
	mxi_micro488ex_interface_clear,
	mxi_micro488ex_device_clear,
	mxi_micro488ex_selective_device_clear,
	mxi_micro488ex_local_lockout,
	mxi_micro488ex_remote_enable,
	mxi_micro488ex_go_to_local,
	mxi_micro488ex_trigger,
	mxi_micro488ex_wait_for_service_request,
	mxi_micro488ex_serial_poll,
	mxi_micro488ex_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_micro488ex_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_MICRO488EX_STANDARD_FIELDS
};

mx_length_type mxi_micro488ex_num_record_fields
		= sizeof( mxi_micro488ex_record_field_defaults )
			/ sizeof( mxi_micro488ex_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_micro488ex_rfield_def_ptr
			= &mxi_micro488ex_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_micro488ex_get_pointers( MX_GPIB *gpib,
				MX_MICRO488EX **micro488ex,
				const char *calling_fname )
{
	static const char fname[] = "mxi_micro488ex_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( micro488ex == (MX_MICRO488EX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MICRO488EX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*micro488ex = (MX_MICRO488EX *) (gpib->record->record_type_struct);

	if ( *micro488ex == (MX_MICRO488EX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MICRO488EX pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	if ( (*micro488ex)->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The RS-232 MX_RECORD pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_micro488ex_check_for_error( MX_MICRO488EX *micro488ex, char *command )
{
	static const char fname[] = "mxi_micro488ex_check_for_error()";

	static struct {
		long micro488ex_error_code;
		long mx_error_code;
		char micro488ex_error_message[40];
	} micro488ex_error_table[] = {
		{ 0,	MXE_SUCCESS,		"OK" },
		{ 1,	MXE_ILLEGAL_ARGUMENT,	"Invalid address" },
		{ 2,	MXE_INTERFACE_IO_ERROR,	"Invalid command" },
		{ 3,	MXE_NOT_VALID_FOR_CURRENT_STATE,  "Wrong mode" },
		{ 4,	MXE_UNKNOWN_ERROR,	"Unassigned - Reserved" },
		{ 5,	MXE_UNKNOWN_ERROR,	"Unassigned - Reserved" },
		{ 6,	MXE_INTERFACE_IO_ERROR,	"No macro" },
		{ 7,	MXE_OUT_OF_MEMORY,	"Macro overflow" },
		{ 8,	MXE_LIMIT_WAS_EXCEEDED,	"Command overflow" },
		{ 9,	MXE_LIMIT_WAS_EXCEEDED,	"Address overflow" },
		{ 10,	MXE_OUT_OF_MEMORY,	"Message overflow" },
		{ 11,	MXE_NOT_VALID_FOR_CURRENT_STATE,  "Not a talker" },
		{ 12,	MXE_NOT_VALID_FOR_CURRENT_STATE,  "Not a listener" },
		{ 13,	MXE_HARDWARE_FAULT,	"Bus error" },
		{ 14,	MXE_TIMED_OUT,		"Timeout - Write" },
		{ 15,	MXE_TIMED_OUT,		"Timeout - Read" },
		{ 16,	MXE_OUT_OF_MEMORY,	"Out of memory" },
		{ 17,	MXE_NOT_VALID_FOR_CURRENT_STATE,  "Macro recursion" },
		{ 18,	MXE_INITIALIZATION_ERROR,  "NVRAM failure" },
		{ 19,	MXE_NOT_VALID_FOR_CURRENT_STATE,  "Logging error" },
		{ 20,	MXE_NOT_VALID_FOR_CURRENT_STATE,  "Timer in use" }
	};

	static int num_error_table_entries =
				sizeof( micro488ex_error_table )
				/ sizeof( micro488ex_error_table[0] );

	char status_command[40];
	char status_response[80];
	int num_items, error_code;
	mx_status_type mx_status;

	strcpy( command, "STATUS 2" );

	mx_status = mx_rs232_putline( micro488ex->rs232_record,
					status_command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( micro488ex->rs232_record,
				status_response, sizeof(status_response),
				NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MICRO488EX_DEBUG 
	MX_DEBUG(-2,
	    ("%s: status = '%s'", fname, status_response));
#endif

	num_items = sscanf( status_response, "%d", &error_code );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Did not find a Micro 488/EX status code in response to a '%s' command "
	"sent to Micro 488/EX controller '%s'.  Response = '%s'.",
		status_command, micro488ex->record->name, status_response );
	}

	/* We are done if no error occurred. */

	if ( error_code == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Is this an unrecognized error code? */

	if ( error_code >= num_error_table_entries ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Micro 488/EX controller '%s' returned an unrecognized error code %d "
	"in response to command '%s'.  Status response = '%s'.",
			micro488ex->record->name, error_code,
			command, status_response );
	}

	/* Return the error message. */

	return mx_error( micro488ex_error_table[error_code].mx_error_code,
		fname, "Micro 488/EX controller '%s' returned error code %d "
		"in response to the command '%s'.",
		micro488ex->record->name, error_code, command );
}

static mx_status_type
mxi_micro488ex_command( MX_MICRO488EX *micro488ex,
			char *command,
			char *response,
			size_t response_buffer_length,
			unsigned long transfer_flags )
{
	static const char fname[] = "mxi_micro488ex_command()";

	int debug;
	unsigned long flags;
	mx_status_type mx_status;

	if ( transfer_flags & MXF_232_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_232_DEBUG );
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
			fname, command, micro488ex->record->name ));
	}

	mx_status = mx_rs232_putline( micro488ex->rs232_record,
					command, NULL, transfer_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != NULL ) {
		mx_status = mx_rs232_getline( micro488ex->rs232_record,
					response, response_buffer_length,
					NULL, transfer_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, response, micro488ex->record->name ));
		}
	}

	flags = micro488ex->micro488ex_flags;

	if ( ( flags & MXF_MICRO488EX_DISABLE_ERROR_CHECKING ) == 0 ) {
		mx_status = mxi_micro488ex_check_for_error( micro488ex,
								command );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_micro488ex_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_micro488ex_create_record_structures()";

	MX_GPIB *gpib;
	MX_MICRO488EX *micro488ex;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GPIB structure." );
	}

	micro488ex = (MX_MICRO488EX *) malloc( sizeof(MX_MICRO488EX) );

	if ( micro488ex == (MX_MICRO488EX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MICRO488EX structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = micro488ex;
	record->class_specific_function_list
				= &mxi_micro488ex_gpib_function_list;

	gpib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_micro488ex_finish_record_initialization( MX_RECORD *record )
{
	return mx_gpib_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxi_micro488ex_print_interface_structure( FILE *file, MX_RECORD *record )
{
	MX_GPIB *gpib;
	MX_MICRO488EX *micro488ex;
	char read_eos_char, write_eos_char;

	gpib = (MX_GPIB *) (record->record_class_struct);

	micro488ex = (MX_MICRO488EX *) (record->record_type_struct);

	fprintf(file, "GPIB parameters for interface '%s':\n", record->name);
	fprintf(file, "  GPIB type              = MICRO488EX.\n\n");

	fprintf(file, "  name                   = %s\n", record->name);
	fprintf(file, "  rs232 port name        = %s\n",
					micro488ex->rs232_record->name);
	fprintf(file, "  default I/O timeout    = %g\n",
					gpib->default_io_timeout);
	fprintf(file, "  default EOI mode       = %d\n",
					gpib->default_eoi_mode);

	read_eos_char = (char) ( gpib->default_read_terminator & 0xff );

	if ( isprint( (int) read_eos_char) ) {
		fprintf(file,
		      "  default read EOS char  = 0x%02x (%c)\n",
			read_eos_char, read_eos_char);
	} else {
		fprintf(file,
		      "  default read EOS char  = 0x%02x\n", read_eos_char);
	}

	write_eos_char = (char) ( gpib->default_write_terminator & 0xff );

	if ( isprint( (int) write_eos_char) ) {
		fprintf(file,
		      "  default write EOS char = 0x%02x (%c)\n",
			write_eos_char, write_eos_char);
	} else {
		fprintf(file,
		      "  default write EOS char = 0x%02x\n", write_eos_char);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_micro488ex_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_micro488ex_open()";

	MX_GPIB *gpib;
	MX_MICRO488EX *micro488ex;
	MX_RS232 *rs232;
	char command[40];
	char response[80];
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232 = (MX_RS232 *) micro488ex->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for RS-232 record '%s' is NULL.",
			micro488ex->rs232_record->name );
	}

	flags = micro488ex->micro488ex_flags;

	/* Enable or disable automatic reporting of errors as requested. */

	if ( flags & MXF_MICRO488EX_DISABLE_ERROR_CHECKING ) {
		strcpy( command, "ERROR OFF" );
	} else {
		strcpy( command, "ERROR NUMBER" );
	}

	mx_status = mx_rs232_putline( micro488ex->rs232_record,
					command, NULL, MICRO488EX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	(void) mx_rs232_discard_unread_input( micro488ex->rs232_record,
						MICRO488EX_DEBUG );

	/* If requested, restore the module to its factory default config.
	 *
	 * This feature was originally added to make it possible to temporarily
	 * use a Micro 488/EX unit for which the battery in a DS1216 battery
	 * backed memory had run down.
	 */

	if ( flags & MXF_MICRO488EX_RESTORE_TO_FACTORY_DEFAULTS ) {

		mx_status = mxi_micro488ex_command( micro488ex, "FACTORY",
					NULL, 0, MICRO488EX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_micro488ex_command( micro488ex, "SAVE",
					NULL, 0, MICRO488EX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Ask the Micro488/EX for its version number. */

	mx_status = mxi_micro488ex_command( micro488ex, "HELLO",
					response, sizeof(response),
					MICRO488EX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gpib->read_buffer_length = 0;
	gpib->write_buffer_length = 0;

	mx_status = mx_gpib_setup_communication_buffers( record );

	return mx_status;
}

/* ========== Device specific calls ========== */

MX_EXPORT mx_status_type
mxi_micro488ex_open_device( MX_GPIB *gpib, int32_t address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_micro488ex_close_device( MX_GPIB *gpib, int32_t address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_micro488ex_read( MX_GPIB *gpib,
		int32_t address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		mx_hex_type transfer_flags )
{
	static const char fname[] = "mxi_micro488ex_read()";

	MX_MICRO488EX *micro488ex;
	int debug;
	unsigned int micro488ex_flags;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_flags & MXF_232_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_232_DEBUG );
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
			fname, command, micro488ex->record->name ));
	}

	sprintf( command, "ENTER %02d", address );

	mx_status = mx_rs232_putline( micro488ex->rs232_record,
					command, NULL, transfer_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( micro488ex->rs232_record,
					buffer, max_bytes_to_read,
					bytes_read, transfer_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;


	if ( debug ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, buffer, micro488ex->record->name ));
	}

	micro488ex_flags = micro488ex->micro488ex_flags;

	if (( micro488ex_flags & MXF_MICRO488EX_DISABLE_ERROR_CHECKING ) == 0) {
		mx_status = mxi_micro488ex_check_for_error( micro488ex,
								command );
	}

	return mx_status;
}

#define MX_PREFIX_BUFFER_LENGTH   10

MX_EXPORT mx_status_type
mxi_micro488ex_write( MX_GPIB *gpib,
		int32_t address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		mx_hex_type transfer_flags )
{
	static const char fname[] = "mxi_micro488ex_write()";

	MX_MICRO488EX *micro488ex;
	char prefix[ MX_PREFIX_BUFFER_LENGTH + 1 ];
	int debug;
	unsigned long micro488ex_flags;
	size_t device_bytes_written;
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_flags & MXF_232_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_232_DEBUG );
	} else {
		debug = FALSE;
	}

	/* Send the Micro488EX command prefix. */

	sprintf( prefix, "OUTPUT %02d;", address );

	if ( debug ) {
		MX_DEBUG(-2,("%s: sending '%s%s' to '%s'.",
			fname, prefix, buffer, micro488ex->record->name ));
	}

	mx_status = mx_rs232_write( micro488ex->rs232_record,
				prefix, MX_PREFIX_BUFFER_LENGTH,
				NULL, transfer_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( micro488ex->rs232_record,
				buffer, &device_bytes_written, transfer_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != NULL ) {
		*bytes_written = device_bytes_written
					+ MX_PREFIX_BUFFER_LENGTH;
	}

	micro488ex_flags = micro488ex->micro488ex_flags;

	if (( micro488ex_flags & MXF_MICRO488EX_DISABLE_ERROR_CHECKING ) == 0) {
		mx_status = mxi_micro488ex_check_for_error( micro488ex,
								buffer );
	}

	return mx_status;
}

#undef MX_PREFIX_BUFFER_LENGTH

MX_EXPORT mx_status_type
mxi_micro488ex_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_micro488ex_interface_clear()";

	MX_MICRO488EX *micro488ex;
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_micro488ex_command( micro488ex, "ABORT",
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_device_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_micro488ex_device_clear()";

	MX_MICRO488EX *micro488ex;
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_micro488ex_command( micro488ex, "CLEAR",
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_selective_device_clear( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_micro488ex_selective_device_clear()";

	MX_MICRO488EX *micro488ex;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "CLEAR %02d", address );

	mx_status = mxi_micro488ex_command( micro488ex, command,
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_micro488ex_local_lockout()";

	MX_MICRO488EX *micro488ex;
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_micro488ex_command( micro488ex, "LOCAL LOCKOUT",
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_remote_enable( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_micro488ex_remote_enable()";

	MX_MICRO488EX *micro488ex;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( address < 0 ) {
		strcpy( command, "REMOTE" );
	} else {
		sprintf( command, "REMOTE %02d", address );
	}

	mx_status = mxi_micro488ex_command( micro488ex, command,
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_go_to_local( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_micro488ex_go_to_local()";

	MX_MICRO488EX *micro488ex;
	char command[20];
	mx_status_type mx_status;

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( address < 0 ) {
		strcpy( command, "LOCAL" );
	} else {
		sprintf( command, "LOCAL %02d", address );
	}

	mx_status = mxi_micro488ex_command( micro488ex, command,
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_trigger( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_micro488ex_trigger_device()";

	MX_MICRO488EX *micro488ex;
	char command[20];
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the trigger command. */

	if ( address < 0 ) {
		strcpy( command, "TR" );
	} else {
		sprintf( command, "TRIGGER %02d", address );
	}

	mx_status = mxi_micro488ex_command( micro488ex, command,
					NULL, 0, MICRO488EX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_micro488ex_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] = "mxi_micro488ex_wait_for_service_request()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Not yet implemented." );
}

MX_EXPORT mx_status_type
mxi_micro488ex_serial_poll( MX_GPIB *gpib, int32_t address,
				uint8_t *serial_poll_byte)
{
	static const char fname[] = "mxi_micro488ex_serial_poll()";

	MX_MICRO488EX *micro488ex;
	char command[20];
	char response[20];
	unsigned short short_value;
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_micro488ex_get_pointers( gpib, &micro488ex, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the serial poll byte. */

	sprintf( command, "SPOLL %02d", address );

	mx_status = mxi_micro488ex_command( micro488ex, command,
					response, sizeof(response),
					MICRO488EX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%hu", &short_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Serial poll byte not seen in response '%s' to command '%s'",
			response, command );
	}

	*serial_poll_byte = (unsigned char) ( short_value & 0xff );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_micro488ex_serial_poll_disable( MX_GPIB *gpib )
{
	/* Apparently this function is not needed for this interface. */

	return MX_SUCCESSFUL_RESULT;
}

