/*
 * Name:    d_sim980.c
 *
 * Purpose: Driver for the Stanford Research Systems SIM980 summing ainput.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIM980_DEBUG	FALSE

#define MXD_SIM980_ERROR_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_rs232.h"
#include "d_sim980.h"

/* Initialize the ainput driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sim980_record_function_list = {
	NULL,
	mxd_sim980_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_sim980_open,
	NULL,
	NULL,
	NULL,
	mxd_sim980_special_processing_setup
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_sim980_analog_input_function_list =
{
	mxd_sim980_read
};

/* SIM980 analog input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_sim980_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_LONG_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_SIM980_STANDARD_FIELDS
};

long mxd_sim980_num_record_fields
		= sizeof( mxd_sim980_rf_defaults )
		  / sizeof( mxd_sim980_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sim980_rfield_def_ptr
			= &mxd_sim980_rf_defaults[0];

/* === */

static mx_status_type
mxd_sim980_get_pointers( MX_ANALOG_INPUT *ainput,
			MX_SIM980 **sim980,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sim980_get_pointers()";

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sim980 == (MX_SIM980 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIM980 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sim980 = (MX_SIM980 *) ainput->record->record_type_struct;

	if ( *sim980 == (MX_SIM980 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIM980 pointer for record '%s' is NULL.",
			ainput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sim980_command( MX_SIM980 *sim980,
		char *command,
		char *response,
		size_t max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxd_sim980_command()";

#if 0
	char esr_response[10];
	unsigned char esr_byte;
#endif
	mx_status_type mx_status;

	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The command buffer pointer passed was NULL." );
	}

	/* Send the command. */

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, sim980->record->name));
	}

	mx_status = mx_rs232_putline( sim980->port_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != (char *) NULL ) {
		/* If a response is expected, read back the response. */

		mx_status = mx_rs232_getline( sim980->port_record,
					response, max_response_length,
					NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, sim980->record->name ));
		}
	}

	/* Check for errors in the previous command. */

#if 0

#if MXD_SIM980_ERROR_DEBUG
	MX_DEBUG(-2,("%s: sending '*ESR?' to '%s'",
		fname, sim980->record->name ));
#endif

	mx_status = mx_rs232_putline( sim980->port_record, "*ESR?", NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( sim980->port_record,
					esr_response, sizeof(esr_response),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIM980_ERROR_DEBUG
	MX_DEBUG(-2,("%s: received '%s' from '%s'",
		fname, esr_response, sim980->record->name ));
#endif

	esr_byte = atol( esr_response );

	if ( esr_byte & 0x20 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Command error (CME) seen for command '%s' sent to '%s'.",
			command, sim980->record->name );
	} else
	if ( esr_byte & 0x10 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Execution error (EXE) seen for command '%s' sent to '%s'.",
			command, sim980->record->name );
	} else
	if ( esr_byte & 0x08 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Device dependent error (DDE) seen for command '%s' sent to '%s'.",
			command, sim980->record->name );
	} else
	if ( esr_byte & 0x02 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Input data lost (INP) for command '%s' sent to '%s'.",
			command, sim980->record->name );
	} else
	if ( esr_byte & 0x04 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Output data lost (QYE) for command '%s' sent to '%s'.",
			command, sim980->record->name );
	}

#endif /* 0 */

	return MX_SUCCESSFUL_RESULT;
}

/* === */

static mx_status_type mxd_sim980_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

/* === */

MX_EXPORT mx_status_type
mxd_sim980_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_sim980_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_SIM980 *sim980 = NULL;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	sim980 = (MX_SIM980 *) malloc( sizeof(MX_SIM980) );

	if ( sim980 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SIM980 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = sim980;
	record->class_specific_function_list
			= &mxd_sim980_analog_input_function_list;

	ainput->record = record;
	sim980->record = record;

	ainput->subclass = MXT_AIN_LONG;

	sim980->channel_number = 1;
	sim980->averaging_time = 1000;	/* in milliseconds */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim980_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sim980_open()";

	MX_ANALOG_INPUT *ainput;
	MX_SIM980 *sim980 = NULL;
	char response[100];
	char copy_of_response[100];
	int argc, num_items;
	char **argv;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	ainput = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_sim980_get_pointers( ainput, &sim980, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any outstanding characters. */

	mx_status = mx_rs232_discard_unwritten_output( sim980->port_record,
							MXD_SIM980_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( sim980->port_record,
							MXD_SIM980_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the communication interface by sending a break signal. */

#if MXD_SIM980_DEBUG
	MX_DEBUG(-2,("%s: sending a break signal to '%s'.",
				fname, record->name ));
#endif

	mx_status = mx_rs232_send_break( sim980->port_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that we are connected to a SIM980 analog PID controller. */

	mx_status = mxd_sim980_command( sim980, "*IDN?",
					response, sizeof(response),
					MXD_SIM980_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( copy_of_response, response, sizeof(copy_of_response) );

	mx_string_split( copy_of_response, ",", &argc, &argv );

	if ( argc != 4 ) {
		free( argv );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find 4 tokens in the response '%s' to "
		"the *IDN? command sent to '%s'.",
			response, record->name );
	}
	if ( strcmp( argv[0], "Stanford_Research_Systems" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Controller '%s' is not a Stanford Research Systems device.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}
	if ( strcmp( argv[1], "SIM980" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Device '%s' is not a SIM980 analog PID controller.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Get the version number. */

	num_items = sscanf( argv[3], "ver%lf", &(sim980->version) );

	if ( num_items != 1 ) {
		mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find the SIM980 version number in the token '%s' "
		"contained in the response '%s' to '*IDN?' by controller '%s'.",
			argv[3], response, record->name );

		free( argv );

		return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sim980_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxd_sim980_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_SIM980_CHANNEL_NUMBER:
		case MXLV_SIM980_CHANNEL_CONTROL:
			record_field->process_function
					    = mxd_sim980_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim980_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_sim980_read()";

	MX_SIM980 *sim980 = NULL;
	char command[100];
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_sim980_get_pointers( ainput, &sim980, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"READ? %lu", sim980->averaging_time );

	mx_status = mxd_sim980_command( sim980, command,
					response, sizeof(response),
					MXD_SIM980_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ainput->raw_value.long_value = atol( response );

	return MX_SUCCESSFUL_RESULT;
}

/*=== */

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxd_sim980_process_function( void *record_ptr,
				void *record_field_ptr,
				void *socket_handler_ptr,
				int operation )
{
	static const char fname[] = "mxd_sim980_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_SIM980 *sim980;
	char command[100];
	char response[100];
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	sim980 = (MX_SIM980 *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_SIM980_CHANNEL_CONTROL:
			snprintf( command, sizeof(command),
				"CHAN? %lu", sim980->channel_number );

			mx_status = mxd_sim980_command( sim980, command,
						response, sizeof(response),
						MXD_SIM980_DEBUG );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			sim980->channel_control = atol( response );
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
		case MXLV_SIM980_CHANNEL_NUMBER:
			if ( (sim980->channel_number < 1)
			  || (sim980->channel_number > 4) )
			{
				mx_status =
				    mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
					"The requested channel number %lu "
					"for SIM980 device '%s' is outside "
					"the allowed range of 1 to 4.",
					sim980->channel_number, record->name );

				sim980->channel_number = 1;

				return mx_status;
			}
			break;

		case MXLV_SIM980_CHANNEL_CONTROL:
			if ( sim980->channel_control > 1 ) {
				sim980->channel_control = 1;
			} else
			if ( sim980->channel_control < -1 ) {
				sim980->channel_control = -1;
			}

			snprintf( command, sizeof(command),
				"CHAN %lu,%ld",
				sim980->channel_number,
				sim980->channel_control );

			mx_status = mxd_sim980_command( sim980, command,
						NULL, 0, MXD_SIM980_DEBUG );

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

