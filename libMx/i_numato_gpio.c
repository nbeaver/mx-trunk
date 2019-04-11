/*
 * Name:    i_numato_gpio.c
 *
 * Purpose: MX driver for the Keithley 2600 series of SourceMeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_KEITHLEY2600_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_process.h"
#include "mx_rs232.h"
#include "mx_gpib.h"
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
	MXI_KEITHLEY2600_STANDARD_FIELDS
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

	MX_KEITHLEY2600 *numato_gpio;

	/* Allocate memory for the necessary structures. */

	numato_gpio = (MX_KEITHLEY2600 *) malloc( sizeof(MX_KEITHLEY2600) );

	if ( numato_gpio == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_KEITHLEY2600 structure." );
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

	MX_KEITHLEY2600 *numato_gpio = NULL;
	MX_INTERFACE *port_interface = NULL;
	unsigned long flags;
	char response[80];
	char format[80];
	int num_items;
	size_t len;
	mx_status_type mx_status;

#if MXI_KEITHLEY2600_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	numato_gpio = (MX_KEITHLEY2600 *) record->record_type_struct;

	if ( numato_gpio == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_KEITHLEY2600 pointer for record '%s' is NULL.",
			record->name);
	}

	port_interface = &(numato_gpio->port_interface);

	flags = numato_gpio->numato_gpio_flags;

	/* Connect to the device. */

	switch( port_interface->record->mx_class ) {
	case MXI_RS232:
		mx_status = mx_rs232_discard_unread_input(
			port_interface->record, MXI_KEITHLEY2600_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	case MXI_GPIB:
		mx_status = mx_gpib_open_device( port_interface->record,
						port_interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	
		mx_status = mx_gpib_selective_device_clear(
			port_interface->record, port_interface->address );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Interface '%s' for Keithley 2600 record '%s' "
		"is not an RS-232 or GPIB record.",
			port_interface->record->name, record->name );
		break;
	}

	/* Verify that the Keithley 2600 is responding to commands. */

	mx_status = mxi_numato_gpio_command( numato_gpio, "*IDN?",
					response, sizeof(response), flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Parse the response. */

	snprintf( format, sizeof(format),
		"Keithley Instruments Inc., Model %%%ds %%%ds %%%ds",
			(int) sizeof(numato_gpio->model_name),
			(int) sizeof(numato_gpio->serial_number),
			(int) sizeof(numato_gpio->firmware_version) );

	num_items = sscanf( response, format,
			numato_gpio->model_name,
			numato_gpio->serial_number,
			numato_gpio->firmware_version );

	if ( num_items != 3 ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"The device expected for '%s' on port interface '%s' "
		"is not a Keithley 2600.  "
		"Its response to an '*IDN?' command was '%s'.",
		record->name, port_interface->record->name, response );
	}

	/* Zap trailing comma (,) characters if present. */

	len = strlen( numato_gpio->model_name );

	if ( (numato_gpio->model_name)[len-1] == ',' ) {
		(numato_gpio->model_name)[len-1] = '\0';
	}

	len = strlen( numato_gpio->serial_number );

	if ( (numato_gpio->serial_number)[len-1] == ',' ) {
		(numato_gpio->serial_number)[len-1] = '\0';
	}

	/*======== Configure global parameters. ========*/

	/* Select ASCII data format. */

	mx_status = mxi_numato_gpio_command( numato_gpio,
				"format.data = format.ASCII",
				NULL, 0,
				numato_gpio->numato_gpio_flags );

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
	MX_KEITHLEY2600 *numato_gpio;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	numato_gpio = (MX_KEITHLEY2600 *) record->record_type_struct;

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

MX_EXPORT mx_status_type
mxi_numato_gpio_command( MX_KEITHLEY2600 *numato_gpio,
				char *command,
				char *response,
				unsigned long max_response_length,
				unsigned long numato_gpio_flags )
{
	static const char fname[] = "mxi_numato_gpio_command()";

	MX_INTERFACE *port_interface = NULL;
	MX_RS232 *rs232 = NULL;
	int i, max_attempts;
	mx_bool_type debug, response_expected;
	mx_status_type mx_status;

	if ( numato_gpio == (MX_KEITHLEY2600 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY2600 pointer passed was NULL." );
	}

	port_interface = &(numato_gpio->port_interface);

	if ( numato_gpio_flags & MXF_KEITHLEY2600_DEBUG_RS232 ) {
		debug = TRUE;
	} else {
		debug = FALSE;
	}

	switch( port_interface->record->mx_class ) {
	case MXI_RS232:
		rs232 = (MX_RS232 *)
			port_interface->record->record_class_struct;

		break;
	case MXI_GPIB:
		break;
	default:
		mx_status = mx_error( MXE_TYPE_MISMATCH, fname,
		    "Interface record '%s' is not an RS-232 or GPIB record.",
			port_interface->record->name );
		break;
	}

	max_attempts = 5;

	for ( i = 0; i < max_attempts; i++ ) {

		if ( debug ) {
			fprintf( stderr, "Sending '%s' to '%s'.\n",
				command, numato_gpio->record->name );
		}

		switch( port_interface->record->mx_class ) {
		case MXI_RS232:
			mx_status = mx_rs232_putline( port_interface->record,
							command, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;
		case MXI_GPIB:
			mx_status = mx_gpib_putline( port_interface->record,
							port_interface->address,
							command, NULL, 0 );
			break;
		default:
			mx_status = mx_error( MXE_TYPE_MISMATCH, fname,
		    "Interface record '%s' is not an RS-232 or GPIB record.",
				port_interface->record->name );
			break;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	
		/* Infer whether or not a response is expected. */

		if ( ( response == NULL) || ( max_response_length == 0 ) ) {
			response_expected = FALSE;
		} else {
			response_expected = TRUE;
		}

		if ( response_expected == FALSE ) {
			return MX_SUCCESSFUL_RESULT;
		}

		switch( port_interface->record->mx_class ) {
		case MXI_RS232:
			mx_status = mx_rs232_getline_with_timeout(
						port_interface->record,
						response, max_response_length,
						NULL, 0, rs232->timeout );
			break;
		case MXI_GPIB:
			mx_status = mx_gpib_getline(
						port_interface->record,
						port_interface->address,
						response, max_response_length,
						NULL, 0 );
			break;
		default:
			mx_status = mx_error( MXE_TYPE_MISMATCH, fname,
		    "Interface record '%s' is not an RS-232 or GPIB record.",
				port_interface->record->name );
			break;
		}

		if ( mx_status.code == MXE_SUCCESS ) {

			if ( debug ) {
				fprintf( stderr, "Received '%s' from '%s'.\n",
					response, numato_gpio->record->name );
			}

			break;		/* Exit the for() loop. */
		} else
		if ( mx_status.code == MXE_TIMED_OUT ) {

		    switch( port_interface->record->mx_class ) {
		    case MXI_RS232:
#if 1
			(void) mx_resynchronize_record(
					port_interface->record );
#else
			(void) mx_rs232_putline( port_interface->record,
						"\r\n", NULL, 0x1 );
			mx_msleep(500);

			(void) mx_rs232_getline_with_timeout(
					port_interface->record,
					response, max_response_length,
					NULL, 0x1, rs232->timeout );
#endif
			break;
		    case MXI_GPIB:
			break;
		    default:
			mx_status = mx_error( MXE_TYPE_MISMATCH, fname,
		    "Interface record '%s' is not an RS-232 or GPIB record.",
				port_interface->record->name );
		    }
		} else {
			return mx_status;
		}

		fprintf( stderr, "Retrying command '%s' for '%s': retry = %d\n",
			command, numato_gpio->record->name, i+1 );
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Did not receive a response to the command '%s' sent "
		"to '%s' after %d attempts.",
			command, numato_gpio->record->name, i );
	}

	return MX_SUCCESSFUL_RESULT;
}

