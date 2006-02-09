/*
 * Name:    i_vp9000.c
 *
 * Purpose: MX interface driver for Velmex VP9000 series motor controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_VP9000_DEBUG	FALSE

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
#include "i_vp9000.h"

MX_RECORD_FUNCTION_LIST mxi_vp9000_record_function_list = {
	mxi_vp9000_initialize_type,
	mxi_vp9000_create_record_structures,
	mxi_vp9000_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_vp9000_open,
	mxi_vp9000_close,
	NULL,
	mxi_vp9000_resynchronize,
};

MX_GENERIC_FUNCTION_LIST mxi_vp9000_generic_function_list = {
	mxi_vp9000_getchar,
	mxi_vp9000_putchar,
	mxi_vp9000_read,
	mxi_vp9000_write,
	mxi_vp9000_num_input_bytes_available,
	mxi_vp9000_discard_unread_input,
	mxi_vp9000_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_vp9000_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_VP9000_STANDARD_FIELDS
};

mx_length_type mxi_vp9000_num_record_fields
		= sizeof( mxi_vp9000_record_field_defaults )
			/ sizeof( mxi_vp9000_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_vp9000_rfield_def_ptr
			= &mxi_vp9000_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_vp9000_get_record_pointers( MX_RECORD *vp9000_record,
				MX_GENERIC **generic,
				const char *calling_fname )
{
	static const char fname[] = "mxi_vp9000_get_record_pointers()";

	if ( vp9000_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( (vp9000_record->mx_superclass != MXR_INTERFACE)
	  || (vp9000_record->mx_class != MXI_GENERIC)
	  || (vp9000_record->mx_type != MXI_GEN_VP9000) ) {

		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' is not a Velmex VP9000 interface record.",
			vp9000_record->name );
	}

	if ( generic == (MX_GENERIC **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GENERIC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*generic = (MX_GENERIC *) (vp9000_record->record_class_struct);

	if ( *generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_GENERIC pointer for record '%s' is NULL.",
			vp9000_record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_vp9000_get_pointers( MX_GENERIC *generic,
				MX_VP9000 **vp9000,
				const char *calling_fname )
{
	static const char fname[] = "mxi_vp9000_get_pointers()";

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GENERIC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( vp9000 == (MX_VP9000 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_VP9000 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*vp9000 = (MX_VP9000 *) (generic->record->record_type_struct);

	if ( *vp9000 == (MX_VP9000 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_VP9000 pointer for record '%s' is NULL.",
			generic->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*=== Public functions ===*/

MX_EXPORT mx_status_type
mxi_vp9000_initialize_type( long type )
{
	static const char fname[] = "mxi_vp9000_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	mx_length_type num_record_fields;
	mx_length_type referenced_field_index;
	mx_length_type num_controllers_varargs_cookie;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found in driver list.", type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

	if ( record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	record_field_defaults = *record_field_defaults_ptr;

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'*record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (mx_length_type *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	num_record_fields = *(driver->num_record_fields);

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_controllers", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index,
				0, &num_controllers_varargs_cookie);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"num_motors", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_controllers_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_vp9000_create_record_structures()";

	MX_GENERIC *generic;
	MX_VP9000 *vp9000;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	vp9000 = (MX_VP9000 *) malloc( sizeof(MX_VP9000) );

	if ( vp9000 == (MX_VP9000 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_VP9000 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = vp9000;
	record->class_specific_function_list
				= &mxi_vp9000_generic_function_list;

	generic->record = record;
	vp9000->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_vp9000_finish_record_initialization()";

	MX_VP9000 *vp9000;

	vp9000 = (MX_VP9000 *) record->record_type_struct;

	if ( vp9000 == (MX_VP9000 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_VP9000 pointer for record '%s' is NULL.",
			record->name );
	}

	vp9000->active_controller = 0;
	vp9000->active_motor = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_vp9000_open()";

	MX_GENERIC *generic;
	MX_VP9000 *vp9000;
	MX_RECORD *rs232_record;
	MX_RS232 *rs232;
	int i, configuration_is_correct;
	char c;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("mxi_vp9000_open() invoked."));

	mx_status = mxi_vp9000_get_record_pointers( record, &generic, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Velmex VP9000 expects its serial port to always be set to
	 * 7 bits, even parity, 2 stop bits.  Check to see if the underlying
	 * RS-232 port is configured this way.
	 */

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232_record = vp9000->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The RS-232 MX_RECORD pointer for record '%s' is NULL.",
			record->name );
	}

	rs232 = (MX_RS232 *) rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for RS-232 record '%s' is NULL.",
			rs232_record->name );
	}

	configuration_is_correct = TRUE;

	if ( rs232->speed < 300 )
		configuration_is_correct = FALSE;

	if ( rs232->speed > 9600 )
		configuration_is_correct = FALSE;

	if ( rs232->word_size != 7 )
		configuration_is_correct = FALSE;

	if ( rs232->parity != 'E' )
		configuration_is_correct = FALSE;

	if ( rs232->stop_bits != 2 )
		configuration_is_correct = FALSE;

	if ( configuration_is_correct == FALSE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Settings for RS-232 port '%s' do not match the requirements of VP9000 "
"interface '%s' which needs 7 bit, even parity, 2 stop bits, 300 to 9600 bps.",
			rs232_record->name, record->name );
	}

	if ( vp9000->num_controllers > 1 ) {
		MX_DEBUG(-2,("%s: Please note that this driver has not yet "
			"been tested with more than one controller connected.",
			fname));
	}

	/* Throw away any pending input and output on the 
	 * VP9000 RS-232 port.
	 */

	mx_status = mx_generic_discard_unwritten_output(
						generic, MXI_VP9000_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return mx_status;
		break;
	}

	mx_status = mx_generic_discard_unread_input( generic, MXI_VP9000_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( vp9000->num_controllers == 1 ) {

		/* Put the VP9000 into the online state using the 'G' command.
		 * This tells the VP9000 to not echo characters and to add a
		 * carriage return at the end of each response.
		 */

		mx_status = mxi_vp9000_putline(vp9000, 1, "G", MXI_VP9000_DEBUG);
	} else {
		/* Put all the VP9000s online using the '&' command. */

		mx_status = mxi_vp9000_putline(vp9000, 1, "&", MXI_VP9000_DEBUG);
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 1; i <= vp9000->num_controllers; i++ ) {

		/* Verify that all the controllers are there by asking them
		 * for their status.
		 */

		mx_status = mxi_vp9000_putline(vp9000, 1, "V", MXI_VP9000_DEBUG);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_vp9000_getc(vp9000, i, &c, MXI_VP9000_DEBUG);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch ( c ) {
		case 'R':	/* Ready for commands. */

			/* There should be a carriage return to discard. */

			mx_status = mxi_vp9000_getc(vp9000, i, &c,
							MXI_VP9000_DEBUG);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( c != MX_CR ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unexpected character 0x%x '%c' followed 'R' response "
			"for VP9000 interface '%s'.",
					c, c, vp9000->record->name );
			}
			break;
		case 'B':
			return mx_error( MXE_NOT_READY, fname,
	"VP9000 interface '%s' says that a motor is currently moving.",
				vp9000->record->name );
			break;
		case 'J':
			return mx_error( MXE_NOT_READY, fname,
"VP9000 interface '%s' says that it is in Jog/Slew mode even though it was "
"just told to go to On-Line mode with a 'G' command.  This may mean that it "
"is using VP9000 firmware older than version 2.8", vp9000->record->name );
			break;
		default:
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"VP9000 interface '%s', controller %d returned invalid "
				"status in response to a V command.  "
				"Response = 0x%x '%c'",
				generic->record->name, i, c, c );
			break;
		}

		/* Tell the controllers to notify us if a limit switch
		 * is hit.
		 */

		mx_status = mxi_vp9000_putline(vp9000, 1, "O1", MXI_VP9000_DEBUG);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_vp9000_close()";

	MX_GENERIC *generic;
	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_vp9000_get_record_pointers( record, &generic, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the VP9000 to leave On-Line mode and return to Jog/slew mode. */

	mx_status = mxi_vp9000_putline(vp9000, 1, "Q", MXI_VP9000_DEBUG);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Throw away any pending input and output on the VP9000 RS-232 port. */

	mx_status = mx_generic_discard_unwritten_output(
						generic, MXI_VP9000_DEBUG );

	switch( mx_status.code ) {
	case MXE_SUCCESS:
	case MXE_UNSUPPORTED:
		break;		/* Continue on. */
	default:
		return mx_status;
		break;
	}

	mx_status = mx_generic_discard_unread_input( generic, MXI_VP9000_DEBUG );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_resynchronize( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mxi_vp9000_close( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_vp9000_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_vp9000_getchar()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getchar( vp9000->rs232_record, c, flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_vp9000_putchar()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putchar( vp9000->rs232_record, c, flags );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_vp9000_read()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_read( vp9000->rs232_record,
				buffer, count, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_vp9000_write()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_write( vp9000->rs232_record,
					buffer, count, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_num_input_bytes_available( MX_GENERIC *generic,
				uint32_t *num_input_bytes_available )
{
	static const char fname[] = "mxi_vp9000_num_input_bytes_available()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_num_input_bytes_available(
			vp9000->rs232_record, num_input_bytes_available );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_vp9000_discard_unread_input()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(
					vp9000->rs232_record, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_vp9000_discard_unwritten_output()";

	MX_VP9000 *vp9000;
	mx_status_type mx_status;

	mx_status = mxi_vp9000_get_pointers( generic, &vp9000, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unwritten_output(
					vp9000->rs232_record, debug_flag );

	return mx_status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_vp9000_command( MX_VP9000 *vp9000, int controller_number,
		char *command, char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_vp9000_command()";

	MX_GENERIC *generic;
	unsigned long sleep_ms;
	int i, max_attempts;
	mx_status_type mx_status;

	MX_DEBUG(2,("%s invoked.", fname));

	if ( vp9000 == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_VP9000 pointer passed was NULL." );
	}

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	generic = (MX_GENERIC *) vp9000->record->record_class_struct;

	/* Send the command string. */

	mx_status = mxi_vp9000_putline( vp9000, controller_number,
					command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if one is expected. */

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	if ( response != NULL ) {
		for ( i=0; i < max_attempts; i++ ) {
			if ( i > 0 ) {
				MX_DEBUG(-2,
			("%s: No response yet to '%s' command.  Retrying...",
				vp9000->record->name, command ));
			}

			mx_status = mxi_vp9000_getline( vp9000, controller_number,
					response, response_buffer_length,
					debug_flag );

			if ( mx_status.code == MXE_SUCCESS ) {
				break;		/* Exit the for() loop. */

			} else if ( mx_status.code != MXE_NOT_READY ) {
				MX_DEBUG(-2,
		("*** Exiting with MX status = %ld for VP9000 interface '%s'",
		 			mx_status.code, vp9000->record->name));
				return mx_status;
			}
			mx_msleep(sleep_ms);
		}

		if ( i >= max_attempts ) {
			mx_status = mxi_vp9000_discard_unread_input(
					generic, debug_flag );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Failed at attempt to discard unread characters in buffer "
		"for VP9000 interface '%s'", vp9000->record->name );
			}

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"No response seen for '%s' command sent to VP9000 interface '%s'.",
				command, vp9000->record->name );
		}
	}

	MX_DEBUG(2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_getline( MX_VP9000 *vp9000, int controller_number,
		char *buffer, long buffer_length, int debug_flag )
{
	static const char fname[] = "mxi_vp9000_getline()";

	char c;
	int i;
	mx_status_type mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2, ("mxi_vp9000_getline() invoked."));
	}

	for ( i = 0; i < (buffer_length - 1) ; i++ ) {
		mx_status = mxi_vp9000_getc( vp9000, controller_number,
						&c, MXF_GENERIC_WAIT );

		if ( mx_status.code != MXE_SUCCESS ) {
			/* Make the buffer contents a valid C string
			 * before returning, so that we can at least
			 * see what appeared before the error.
			 */

			buffer[i] = '\0';

			if ( debug_flag ) {
				MX_DEBUG(-2, ("%s failed.\nbuffer = '%s'",
					fname, buffer));

				MX_DEBUG(-2,
				("Failed with MX status = %ld, c = 0x%x '%c'",
				mx_status.code, c, c));
			}

			return mx_status;
		}

		if ( debug_flag ) {
			MX_DEBUG(-2,
		("mxi_vp9000_getline: received c = 0x%x '%c'", c, c));
		}

		/* If we received an '^' character, this means that a 
		 * previously running program has finished.  This program
		 * relies on using the 'V' command to determine the 
		 * controller status, so discard this and the following
		 * carriage return.
		 */

		buffer[i] = c;

		/* In mxi_vp9000_open(), we programed the controller to put
		 * carriage return characters at the end of its responses.
		 */

		if ( c == MX_CR ) {
			break;		/* exit the for() loop */
		}
	}

	/* Check to see if the last character was a carriage return
	 * and if it is then overwrite it with a NULL.
	 */

	if ( buffer[i] != MX_CR ) {
		mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Warning: Input buffer overrun for VP9000 interface '%s'.",
			vp9000->record->name );

		buffer[i] = '\0';
	} else {
		mx_status = MX_SUCCESSFUL_RESULT;

		buffer[i] = '\0';
	}

#if MXI_VP9000_DEBUG
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_vp9000_getline: buffer = '%s'", buffer) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_putline( MX_VP9000 *vp9000, int controller_number,
				char *buffer, int debug_flag )
{
	static const char fname[] = "mxi_vp9000_putline()";

	char *ptr;
	char c;
	int i;
	mx_status_type mx_status;

#if MXI_VP9000_DEBUG
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_vp9000_putline: buffer = '%s'",buffer));
	}

	if ((vp9000->active_controller != 0) || (vp9000->active_motor != 0)) {
		return mx_error( MXE_NOT_READY, fname,
"VP9000 interface '%s', controller %d, motor %d is currently moving.  "
"New commands cannot be sent until this motor stops moving.",
		vp9000->record->name, vp9000->active_controller,
		vp9000->active_motor );
	}

	/* Send the required number of opening curly braces. */

	for ( i = 1; i < controller_number; i++ ) {
		mx_status = mxi_vp9000_putc( vp9000, controller_number,
						'{', MXF_GENERIC_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("mxi_vp9000_putline: sent opening curly brace."));
		}
	}

	/* Send the body of the message. */

	ptr = &buffer[0];

	while ( *ptr != '\0' ) {
		c = *ptr;

		mx_status = mxi_vp9000_putc( vp9000, controller_number,
						c, MXF_GENERIC_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("mxi_vp9000_putline: sent 0x%x '%c'", c, c));
		}

		ptr++;
	}

	/* Send the required number of closing curly braces. */

	for ( i = 1; i < controller_number; i++ ) {
		mx_status = mxi_vp9000_putc( vp9000, controller_number,
						 '}', MXF_GENERIC_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,
			("mxi_vp9000_putline: sent closing curly brace."));
		}
	}

	/* Send a CR after the end of the command. */

	c = MX_CR;

	mx_status = mxi_vp9000_putc( vp9000, controller_number,
					c, MXF_GENERIC_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("mxi_vp9000_putline(eos): sent 0x%x '%c'", c, c));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_vp9000_getc( MX_VP9000 *vp9000, int controller_number,
			char *c, int debug_flag )
{
	MX_GENERIC *generic;
	mx_status_type mx_status;

	generic = (MX_GENERIC *) vp9000->record->record_class_struct;

	mx_status = mx_generic_getchar( generic, c, MXF_GENERIC_WAIT );

#if MXI_VP9000_DEBUG
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_vp9000_getc: received 0x%x '%c'", *c, *c));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_vp9000_putc( MX_VP9000 *vp9000, int controller_number,
			char c, int debug_flag )
{
	MX_GENERIC *generic;
	mx_status_type mx_status;

	generic = (MX_GENERIC *) vp9000->record->record_class_struct;

	mx_status = mx_generic_putchar( generic, c, MXF_GENERIC_WAIT );

#if MXI_VP9000_DEBUG
	if ( 1 ) {
#else
	if ( debug_flag ) {
#endif
		MX_DEBUG(-2, ("mxi_vp9000_putc: sent 0x%x '%c'", c, c));
	}

	return mx_status;
}

