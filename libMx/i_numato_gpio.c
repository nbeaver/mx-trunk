/*
 * Name:    i_numato_gpio.c
 *
 * Purpose: MX interface driver for Numato Lab GPIO devices.
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

#define MXI_NUMATO_GPIO_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "i_numato_gpio.h"

MX_RECORD_FUNCTION_LIST mxi_numato_gpio_record_function_list = {
	NULL,
	mxi_numato_gpio_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_numato_gpio_open,
	NULL,
	NULL,
	NULL,
	mxi_numato_gpio_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_numato_gpio_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NUMATO_GPIO_STANDARD_FIELDS
};

long mxi_numato_gpio_num_record_fields
		= sizeof( mxi_numato_gpio_record_field_defaults )
			/ sizeof( mxi_numato_gpio_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_numato_gpio_rfield_def_ptr
			= &mxi_numato_gpio_record_field_defaults[0];

/*---*/

static mx_status_type mxi_numato_gpio_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/*---*/

MX_EXPORT mx_status_type
mxi_numato_gpio_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_numato_gpio_create_record_structures()";

	MX_NUMATO_GPIO *numato_gpio;

	/* Allocate memory for the necessary structures. */

	numato_gpio = (MX_NUMATO_GPIO *) malloc( sizeof(MX_NUMATO_GPIO) );

	if ( numato_gpio == (MX_NUMATO_GPIO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_NUMATO_GPIO structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = numato_gpio;

	record->record_function_list = &mxi_numato_gpio_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	numato_gpio->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_numato_gpio_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_numato_gpio_open()";

	MX_NUMATO_GPIO *numato_gpio = NULL;
	unsigned long flags;
	mx_bool_type debug_rs232;
	mx_status_type mx_status;

#if MXI_NUMATO_GPIO_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	numato_gpio = (MX_NUMATO_GPIO *) record->record_type_struct;

	if ( numato_gpio == (MX_NUMATO_GPIO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NUMATO_GPIO pointer for record '%s' is NULL.",
			record->name);
	}

	flags = numato_gpio->numato_gpio_flags;

	if ( flags & MXF_NUMATO_GPIO_DEBUG_RS232 ) {
		debug_rs232 = TRUE;
	} else {
		debug_rs232 = FALSE;
	}

	/* Send a <CR> to make sure that any partial commands or junk data
	 * have been discarded.
	 */

	mx_status = mx_rs232_putchar( numato_gpio->rs232_record,
						0x0d, debug_rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(1000);

	/* Discard any leftover bytes in the serial port. */

	mx_status = mx_rs232_discard_unwritten_output(
				numato_gpio->rs232_record, debug_rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( numato_gpio->rs232_record,
								 debug_rs232 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the version number of the Numato firmware. */

	mx_status = mxi_numato_gpio_command( numato_gpio, "ver",
					numato_gpio->version,
					sizeof(numato_gpio->version) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read the id number of the Numato device. */

	mx_status = mxi_numato_gpio_command( numato_gpio, "id",
					numato_gpio->id,
					sizeof(numato_gpio->id) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_numato_gpio_special_processing_setup( MX_RECORD *record )
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
					= mxi_numato_gpio_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*==================================================================*/

static mx_status_type
mxi_numato_gpio_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxi_numato_gpio_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_NUMATO_GPIO *numato_gpio;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	numato_gpio = (MX_NUMATO_GPIO *) record->record_type_struct;

	MXW_UNUSED( numato_gpio );

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

/*==================================================================*/

/* Communication between our computer and the Numato controller seems
 * to be somewhat brittle.  It seems to work the best if you send 
 * characters to it with one putchar() per character and read it
 * with one getchar() per character.  Things like putline() or getline()
 * seem to fail.
 */

MX_EXPORT mx_status_type
mxi_numato_gpio_command( MX_NUMATO_GPIO *numato_gpio,
				char *command,
				char *response,
				unsigned long max_response_length )
{
	static const char fname[] = "mxi_numato_gpio_command()";

	MX_RECORD *rs232_record = NULL;
	MX_RS232 *rs232 = NULL;
	char c;
	char local_buffer[80];
	size_t i, length;
	unsigned long flags;
	mx_bool_type debug, debug_verbose;
	mx_status_type mx_status;

	if ( numato_gpio == (MX_NUMATO_GPIO *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NUMATO_GPIO pointer passed was NULL." );
	}

	rs232_record = numato_gpio->rs232_record;

	if ( rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The rs232_record pointer for Numato GPIO interface '%s' "
		"is NULL.", numato_gpio->record->name );
	}

	rs232 = (MX_RS232 *) rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RS232 pointer for RS232 record '%s' used "
		"by Numato GPIO interface '%s' is NULL.",
			rs232_record->name, numato_gpio->record->name );
	}

	flags = numato_gpio->numato_gpio_flags;

	if ( flags & MXF_NUMATO_GPIO_DEBUG_RS232 ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	if ( flags & MXF_NUMATO_GPIO_DEBUG_RS232_VERBOSE ) {
		debug_verbose = TRUE;
	} else {
		debug_verbose = FALSE;
	}

	/* The Numato GPIO device echoes back everything that we send to it.
	 * So we will need to readout and throw away that text.  Line
	 * terminators are handled separately.
	 */

	length = strlen( command );

	if ( length > sizeof(local_buffer) ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The length (%ld) of the command '%s' (plus <CR>) sent to "
		"Numato GPIO device '%s' is longer than the length (%ld) of "
		"the local buffer used for reading out echoed responses.  "
		"You will need to either shorten the command or else "
		"increase the size of 'local_buffer' in the source code.  "
		"Numato commands are short, so you should never see "
		"this error message.",
			(long) length, command,
			numato_gpio->record->name,
			(long) sizeof(local_buffer) );
	}

	/* Now send the command. */

	if ( debug ) {
		fprintf( stderr, "Sending '%s' to '%s'.\n",
			command, numato_gpio->record->name );
	}

	/* Send the command one character at a time and receive
	 * the response one character at a time.
	 */

	for ( i = 0; i < length; i++ ) {
		c = command[i];

		if ( debug_verbose ) {
			MX_DEBUG(-2,("%s: sending %#x", fname, c));
		}

		mx_status = mx_rs232_putchar( rs232_record, c, debug );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_getchar_with_timeout( rs232_record, &c,
						debug, rs232->timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_verbose ) {
			MX_DEBUG(-2,("%s: received %#x", fname, c));
		}
	}

	/* Send the line terminator character <CR> to the Numato. */

	c = 0x0d;

	if ( debug_verbose ) {
		MX_DEBUG(-2,("%s: sending %#x", fname, c));
	}

	mx_status = mx_rs232_putchar( rs232_record, c, debug );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any 0x0a or 0x0d characters. */

	while( TRUE ) {
		mx_status = mx_rs232_getchar_with_timeout( rs232_record, &c,
						debug, rs232->timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_verbose ) {
			MX_DEBUG(-2,("%s: received %#x", fname, c));
		}

		/* Break out if we see a non-line terminator character. */

		if ( (c != 0x0a) && (c != 0x0d) ) {
			break;		/* Exit this while() loop. */
		}
	}

	/* If the most recently received character is a 0x3e '>' character,
	 * then the Numato has sent us an empty response to the command
	 * and we should return now.
	 */

	if ( c == 0x3e ) {
		if ( (response != (char *) NULL)
		  && (max_response_length > 0) )
		{
			response[0] = '\0';
		}

		if ( debug ) {
			fprintf( stderr, "Received '' from '%s'.\n",
					numato_gpio->record->name );
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise the most recently received character is the first
	 * character of the expected response.
	 */

	response[0] = c;

	/* Read in the rest of the expected response. */

	for ( i = 1; i < max_response_length; i++ ) {
		mx_status = mx_rs232_getchar_with_timeout( rs232_record, &c,
						debug, rs232->timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_verbose ) {
			MX_DEBUG(-2,("%s: received %#x", fname, c));
		}

		/* Break out if we see a line terminator character. */

		if ( (c == 0x0a) || (c == 0x0d) ) {
			break;		/* Exit this while() loop. */
		}

		response[i] = c;
	}

	response[i] = '\0';

	/* Discard any 0x0a or 0x0d characters, until we see 0x3e '>'. */

	while( TRUE ) {
		mx_status = mx_rs232_getchar_with_timeout( rs232_record, &c,
							debug, rs232->timeout );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_verbose ) {
			MX_DEBUG(-2,("%s: received %#x", fname, c));
		}

		/* Break out if we see a non-line terminator character. */

		if ( (c != 0x0a) && (c != 0x0d) ) {
			break;		/* Exit this while() loop. */
		}
	}

	if ( c != 0x3e ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' from Numato controller '%s' did not end "
		"with a '>' 0x3e character.  Instead, it ended with %#x.",
			response, numato_gpio->record->name, c );
	}

	if ( debug ) {
		fprintf( stderr, "Received '%s' from '%s'.\n",
			response, numato_gpio->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

