/*
 * Name:    i_hsc1.c
 *
 * Purpose: MX driver for X-ray Instrumentation Associates HSC-1
 *          Huber slit controllers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_HSC1_INTERFACE_DEBUG	FALSE

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
#include "i_hsc1.h"

MX_RECORD_FUNCTION_LIST mxi_hsc1_record_function_list = {
	mxi_hsc1_initialize_type,
	mxi_hsc1_create_record_structures,
	mxi_hsc1_finish_record_initialization,
	NULL,
	NULL,
	mxi_hsc1_open,
	mxi_hsc1_close,
	mxi_hsc1_resynchronize,
};

MX_GENERIC_FUNCTION_LIST mxi_hsc1_generic_function_list = {
	mxi_hsc1_getchar,
	mxi_hsc1_putchar,
	mxi_hsc1_read,
	mxi_hsc1_write,
	mxi_hsc1_num_input_bytes_available,
	mxi_hsc1_discard_unread_input,
	mxi_hsc1_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_hsc1_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_HSC1_INTERFACE_STANDARD_FIELDS
};

long mxi_hsc1_num_record_fields
		= sizeof( mxi_hsc1_record_field_defaults )
			/ sizeof( mxi_hsc1_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_hsc1_rfield_def_ptr
			= &mxi_hsc1_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_hsc1_get_pointers( MX_RECORD *hsc1_interface_record,
				MX_HSC1_INTERFACE **hsc1_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxi_hsc1_get_pointers()";

	if ( hsc1_interface_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( hsc1_interface == (MX_HSC1_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_HSC1_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*hsc1_interface = (MX_HSC1_INTERFACE *)
				(hsc1_interface_record->record_type_struct);

	if ( *hsc1_interface == (MX_HSC1_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HSC1_INTERFACE pointer for record '%s' is NULL.",
			hsc1_interface_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_hsc1_initialize_type( long type )
{
	static const char fname[] = "mxi_hsc1_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults, *field;
	long num_record_fields;
	long num_modules_field_index;
	long num_modules_varargs_cookie;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.", type );
	}

	record_field_defaults = *(driver->record_field_defaults_ptr);
	num_record_fields = *(driver->num_record_fields);

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_modules", &num_modules_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
		num_modules_field_index, 0, &num_modules_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"module_id", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_modules_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_hsc1_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_hsc1_create_record_structures()";

	MX_GENERIC *generic;
	MX_HSC1_INTERFACE *hsc1_interface;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	hsc1_interface = (MX_HSC1_INTERFACE *)
				malloc( sizeof(MX_HSC1_INTERFACE) );

	if ( hsc1_interface == (MX_HSC1_INTERFACE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HSC1_INTERFACE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = hsc1_interface;
	record->class_specific_function_list
				= &mxi_hsc1_generic_function_list;

	generic->record = record;
	hsc1_interface->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_hsc1_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_hsc1_finish_record_initialization()";

	MX_HSC1_INTERFACE *hsc1_interface;
	int i;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	hsc1_interface = (MX_HSC1_INTERFACE *)record->record_type_struct;

	if ( hsc1_interface == (MX_HSC1_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_HSC1_INTERFACE pointer for record '%s' is NULL.",
			record->name );
	}

	if ( hsc1_interface->num_modules <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of HSC-1 modules specified (%ld) is <= 0.",
			hsc1_interface->num_modules );
	}

	hsc1_interface->module_is_busy
		  = (int *) malloc(hsc1_interface->num_modules * sizeof(int));

	if ( hsc1_interface->module_is_busy == (int *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate %ld 'module_is_busy' flags.",
				hsc1_interface->num_modules );
	}

	/* Mark all the HSC-1 modules as not busy. */

	for ( i = 0; i < hsc1_interface->num_modules; i++ ) {
		hsc1_interface->module_is_busy[i] = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_hsc1_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_hsc1_open()";

	MX_HSC1_INTERFACE *hsc1_interface;
	MX_RS232 *rs232;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( record, &hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( hsc1_interface->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"RS-232 record pointer for HSC-1 record '%s' is NULL.",
			record->name );
	}

	rs232 = (MX_RS232 *)
			(hsc1_interface->rs232_record->record_class_struct);

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_RS232 pointer for record '%s' is NULL.",
			hsc1_interface->rs232_record->name );
	}

	if ( rs232->speed != 9600 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The HSC-1 controller requires a port speed of 9600 baud.  Instead saw %ld.",
			rs232->speed );
	}
	if ( rs232->word_size != 8 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The HSC-1 controller requires 8 bit characters.  Instead saw %ld",
			rs232->word_size );
	}
	if ( rs232->parity != 'N' ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The HSC-1 controller requires no parity.  Instead saw '%c'.",
			rs232->parity );
	}
	if ( rs232->stop_bits != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The HSC-1 controller requires 1 stop bit.  Instead saw %ld.",
			rs232->stop_bits );
	}
	if (rs232->flow_control != 'N') {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The HSC-1 controller requires no flow control.  "
	"Instead ssaw '%c'.", rs232->flow_control );
	}
	if ( rs232->read_terminators != 0x0d0a ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The HSC-1 controller requires the RS-232 read terminators "
		"to be a CR LF sequence ( 0x0d0a ).  Instead saw %#lx.",
			rs232->read_terminators );
	}
	if ( rs232->write_terminators != 0xd ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The HSC-1 controller requires the RS-232 write terminator "
		"to be a CR character ( 0x0d ).  Instead saw %#lx.",
			rs232->write_terminators );
	}

	mx_status = mxi_hsc1_resynchronize( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_hsc1_close()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( record, &hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( hsc1_interface->rs232_record,
					MXI_HSC1_INTERFACE_DEBUG);

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_hsc1_resynchronize()";

	MX_HSC1_INTERFACE *hsc1_interface;
	long i;
	mx_status_type mx_status;

	hsc1_interface = (MX_HSC1_INTERFACE *) record->record_type_struct;

	if ( hsc1_interface == (MX_HSC1_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_HSC1 pointer for motor is NULL." );
	}

	/* Discard any unwritten RS-232 output. */

	mx_status = mx_rs232_discard_unwritten_output(
						hsc1_interface->rs232_record,
						MXI_HSC1_INTERFACE_DEBUG );

	if ( mx_status.code != MXE_SUCCESS && mx_status.code != MXE_UNSUPPORTED)
		return mx_status;

	/* Send a CR character so that any incomplete command line
	 * will be terminated.
	 */

	mx_status = mx_rs232_putchar( hsc1_interface->rs232_record,
					0xd, MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any unread RS-232 input. */

	mx_status = mx_rs232_discard_unread_input( hsc1_interface->rs232_record,
						MXI_HSC1_INTERFACE_DEBUG );

	/* Mark all the HSC-1 modules as not busy. */

	for ( i = 0; i < hsc1_interface->num_modules; i++ ) {
		hsc1_interface->module_is_busy[i] = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_hsc1_getchar()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getchar( hsc1_interface->rs232_record, c, flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_hsc1_putchar()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putchar( hsc1_interface->rs232_record, c, flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_hsc1_read()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_read( hsc1_interface->rs232_record,
				buffer, count, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_hsc1_write()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_write( hsc1_interface->rs232_record,
					buffer, count, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available )
{
	static const char fname[] = "mxi_hsc1_num_input_bytes_available()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_num_input_bytes_available(
		hsc1_interface->rs232_record, num_input_bytes_available );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_hsc1_discard_unread_input()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( hsc1_interface->rs232_record,
							debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_hsc1_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_hsc1_discard_unwritten_output()";

	MX_HSC1_INTERFACE *hsc1_interface;
	mx_status_type mx_status;

	hsc1_interface = NULL;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	mx_status = mxi_hsc1_get_pointers( generic->record,
					&hsc1_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unwritten_output(
					hsc1_interface->rs232_record,
					debug_flag );

	return mx_status;
}

/* === Functions specific to this driver. === */

static long
mxi_hsc1_get_module_number( MX_HSC1_INTERFACE *hsc1_interface,
				char *buffer_ptr )
{
	long i, module_number;

	module_number = -1L;

	for ( i = 0; i < hsc1_interface->num_modules; i++ ) {

		if ( strncmp( buffer_ptr, hsc1_interface->module_id[i],
				MX_MAX_HSC1_MODULE_ID_LENGTH ) == 0 )
		{
			module_number = i;
			break;			/* exit the for() loop */
		}
	}
	return module_number;
}

static mx_status_type
mxi_hsc1_handle_error_code( const char *calling_fname,
			char *rs232_record_name,
			char *module_id,
			char *response )
{
	int num_items, error_code, mx_error_code;

	num_items = sscanf( response, "%d", &error_code );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, calling_fname,
			"Did not find HSC-1 status code in error message '%s'",
			response );
	}

	switch( error_code ) {
	case 0:
	case 1:
	case 2:
		mx_error_code = MXE_INTERFACE_IO_ERROR;
		break;
	case 3:
	case 4:
	case 5:
	case 8:
	case 12:
	case 13:
		mx_error_code = MXE_ILLEGAL_ARGUMENT;
		break;
	case 6:
	case 11:
		mx_error_code = MXE_WOULD_EXCEED_LIMIT;
		break;
	case 7:
		mx_error_code = MXE_PERMISSION_DENIED;
		break;
	case 9:
		return MX_SUCCESSFUL_RESULT;
	case 10:
		mx_error_code = MXE_DEVICE_ACTION_FAILED;
		break;
	default:
		mx_error_code = MXE_INTERFACE_IO_ERROR;
		break;
	}

	return mx_error( mx_error_code, calling_fname,
		"Module '%s' responded with error '%s'",
		module_id, response );
}

MX_EXPORT mx_status_type
mxi_hsc1_command( MX_HSC1_INTERFACE *hsc1_interface,
		unsigned long module_number,
		char *command,
		char *response,
		int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_hsc1_command()";

	char local_buffer[200];
	char *buffer_ptr, *last_char_ptr, *next_field_ptr, *hsc1_status;
	char *module_id;
	size_t module_id_length, status_length, remaining_buffer_length;
	unsigned long sleep_ms;
	long other_module_number, mx_error_code;
	int i, max_attempts;
	int module_is_busy, ignore_busy_status;
	unsigned long num_input_bytes_available;
	mx_status_type mx_status;

	static unsigned long command_number = 0L;

	if ( hsc1_interface == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_HSC1_INTERFACE pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}
	if ( response != NULL ) {
		strcpy( response, "" );
	}

	command_number++;

	if ( module_number >= hsc1_interface->num_modules )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Module number %lu for HSC-1 interface '%s' is outside "
		"the allowed range of 0 to %ld", module_number,
			hsc1_interface->record->name,
			(hsc1_interface->num_modules - 1) );
	}

	if ( hsc1_interface->module_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"hsc1_interface->module_id pointer for HSC-1 interface '%s' is NULL.",
			hsc1_interface->record->name );
	}

	module_id = ( hsc1_interface->module_id )[ module_number ];

	if ( module_id == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"module_id address for HSC-1 interface '%s' module %lu is NULL.",
			hsc1_interface->record->name, module_number );
	}

	module_id_length = strlen(module_id);

	if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
		MX_DEBUG(-2,("%s: '%s' checking to see if module is busy.",
				fname, module_id));
	}

	/* If the module is marked as busy, see if it has generated
	 * end of position report yet.
	 */

	module_is_busy = hsc1_interface->module_is_busy[ module_number ];

	ignore_busy_status = debug_flag & MXF_HSC1_IGNORE_BUSY_STATUS;

	if ( module_is_busy && !ignore_busy_status ) {

		/* Is there any pending input on the serial port? */

		mx_status = mx_rs232_num_input_bytes_available(
				hsc1_interface->rs232_record,
				&num_input_bytes_available );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_input_bytes_available == 0 ) {
			if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
				mx_error_code = (MXE_NOT_READY | MXE_QUIET);
			} else {
				mx_error_code = MXE_NOT_READY;
			}

			return mx_error( mx_error_code, fname,
			    "One or more of the HSC-1 motors is still busy." );
		}

		while ( num_input_bytes_available > 0 ) {

			/* Try to read in a line from a module. */

			mx_status = mx_rs232_getline( hsc1_interface->rs232_record,
					local_buffer, sizeof(local_buffer),
					NULL, 0);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			buffer_ptr = local_buffer;

			if ( *buffer_ptr != '%' ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"The first character returned by the HSC-1 was not a '%%' character.  "
					"Response seen = '%s'", local_buffer );
			}

			buffer_ptr++;

			/* Is this ID from the module we were waiting for? */

			if ( strncmp( buffer_ptr,
				module_id, module_id_length ) == 0 ) {

				/* If it was, then mark this module as not
				 * busy and break out of the while loop.
				 */

				hsc1_interface->module_is_busy[module_number]
								= FALSE;

				break;
			}

			/* Mark the module we got a message from as not busy,
			 * and go back to look for input from the module
			 * we really are looking for.
			 */

			other_module_number = mxi_hsc1_get_module_number(
						hsc1_interface, buffer_ptr );

			if ( other_module_number < 0 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"1: The module ID found in the message '%s' "
				"is not marked as being connected to "
				"the interface '%s'", local_buffer,
					hsc1_interface->record->name );
			}

			hsc1_interface->module_is_busy[other_module_number]
								= FALSE;

			/* Are there more lines to be read from
			 * the serial port?
			 */

			mx_status = mx_rs232_num_input_bytes_available(
					hsc1_interface->rs232_record,
					&num_input_bytes_available );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( num_input_bytes_available == 0 ) {
				if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
					mx_error_code =
						(MXE_NOT_READY | MXE_QUIET);
				} else {
					mx_error_code = MXE_NOT_READY;
				}

				return mx_error( mx_error_code, fname,
				"One or more HSC-1 motors is still busy." );
			}
		}
	}

	/* Now we can send the command to the HSC-1. */

	sprintf( local_buffer, "!%s %s", module_id, command );

	if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
		MX_DEBUG(-2,("%s: '%s' sending command '%s'",
				fname, module_id, command));
	}

	mx_status = mx_rs232_putline( hsc1_interface->rs232_record,
					local_buffer, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are not going to wait for the response status, return now. */

	if ( debug_flag & MXF_HSC1_IGNORE_RESPONSE ) {

		if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
			MX_DEBUG(-2,("%s: '%s' ignoring any response.",
				fname, module_id ));
		}
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the response. */

	i = 0;

	max_attempts = 10;
	sleep_ms = 1;

	for (;;) {

		for ( i=0; i < max_attempts; i++ ) {
			if ( i > 0 ) {
				MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
				hsc1_interface->record->name, command ));
			}

			strcpy( local_buffer, "" );

			mx_status = mx_rs232_getline(
				hsc1_interface->rs232_record, local_buffer,
				sizeof(local_buffer), NULL, 0 );

			if ( mx_status.code == MXE_SUCCESS ) {
				break;		/* Exit the for() loop. */

			} else if ( mx_status.code != MXE_NOT_READY ) {
				MX_DEBUG(-2,("*** Exiting with status = %ld",
							mx_status.code));
				return mx_status;
			}
			mx_msleep(sleep_ms);
		}

		if ( i >= max_attempts ) {
			mx_status = mx_rs232_discard_unread_input(
					hsc1_interface->rs232_record,
					debug_flag );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread characters in buffer for record '%s'",
					hsc1_interface->record->name );
			}

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"No response seen to '%s' command", command );
		}

		/* See if the response had the correct module ID in it. */
		
		if ( local_buffer[0] != '%' ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"The first character returned by the HSC-1 was not a '%%' character.  "
"Response seen = '%s'", local_buffer );
		}

		buffer_ptr = local_buffer + 1;

		if (strncmp( buffer_ptr, module_id, module_id_length ) == 0) {

			/* If the module IDs match, exit the for() loop. */

			break;
		}

		/* Mark the module we got a message from as not busy,
		 * and go back to look for input from the module
		 * we really are looking for.
		 */

		other_module_number = mxi_hsc1_get_module_number(
						hsc1_interface, buffer_ptr );

		if ( other_module_number < 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"2: The module ID found in the message '%s' "
				"is not marked as being connected to "
				"the interface '%s'", local_buffer,
					hsc1_interface->record->name );
		}

		hsc1_interface->module_is_busy[other_module_number] = FALSE;
	}

	buffer_ptr += (module_id_length + 1);

	/* Delete the trailing semicolon from the response. */

	remaining_buffer_length = strlen(buffer_ptr);

	last_char_ptr = buffer_ptr + remaining_buffer_length - 1;

	if ( *last_char_ptr != ';' ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Did not see ';' character in response from HSC-1.  Response seen = '%s'",
				local_buffer );
	}

	*last_char_ptr = '\0';

	/* Copy the part of the response buffer after the status
	 * code to the caller's buffer.
	 */

	hsc1_status = buffer_ptr;

	next_field_ptr = strchr( buffer_ptr, ' ' );

	if ( next_field_ptr == NULL ) {
		if ( response != NULL ) {
			strcpy( response, "" );
		}
	} else {
		/* Null terminate the status code. */

		*next_field_ptr = '\0';

		next_field_ptr++;

		/* Copy the body of the response if needed. */

		if ( response != NULL ) {
			strlcpy( response, next_field_ptr,
					response_buffer_length );
		}
	}

	if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
		if ( response == NULL ) {
			MX_DEBUG(-2,(
			"%s: '%s' discarded response.",fname, module_id ));
		} else {
			MX_DEBUG(-2,(
			"%s: '%s' received response '%s'",
				fname, module_id, response ));
		}
	}

	/* Check the status code in the response and return
	 * an appropriate MX status to the caller.
	 */

	status_length = strlen( hsc1_status );

	if (strncmp(hsc1_status, "OK", status_length) == 0) {

		return MX_SUCCESSFUL_RESULT;

	} else if (strncmp(hsc1_status, "BUSY", status_length)== 0) {

		if ( debug_flag & MXI_HSC1_INTERFACE_DEBUG ) {
			mx_error_code = (MXE_NOT_READY | MXE_QUIET);
		} else {
			mx_error_code = MXE_NOT_READY;
		}

		return mx_error( mx_error_code, fname,
				"HSC-1 controller is busy." );

	} else if (strncmp(hsc1_status, "ERROR", status_length)== 0) {

		return mxi_hsc1_handle_error_code( fname,
					hsc1_interface->rs232_record->name,
					module_id, next_field_ptr );

	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unexpected status code '%s' seen in HSC-1 response.  "
			"The remainder of the message was '%s'",
				hsc1_status, next_field_ptr );
	}
}

