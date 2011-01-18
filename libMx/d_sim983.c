/*
 * Name:    d_sim983.c
 *
 * Purpose: Driver for the Stanford Research Systems SIM983 scaling amplifier.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SIM983_DEBUG	TRUE

#define MXD_SIM983_ERROR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_amplifier.h"
#include "d_sim983.h"

MX_RECORD_FUNCTION_LIST mxd_sim983_record_function_list = {
	NULL,
	mxd_sim983_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_sim983_open
};

MX_AMPLIFIER_FUNCTION_LIST mxd_sim983_amplifier_function_list = {
	mxd_sim983_get_gain,
	mxd_sim983_set_gain,
	mxd_sim983_get_offset,
	mxd_sim983_set_offset,
	mxd_sim983_get_time_constant,
	mxd_sim983_set_time_constant
};

MX_RECORD_FIELD_DEFAULTS mxd_sim983_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_SIM983_STANDARD_FIELDS
};

long mxd_sim983_num_record_fields
		= sizeof( mxd_sim983_record_field_defaults )
		    / sizeof( mxd_sim983_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sim983_rfield_def_ptr
			= &mxd_sim983_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_sim983_get_pointers( MX_AMPLIFIER *amplifier,
			MX_SIM983 **sim983,
			const char *calling_fname )
{
	static const char fname[] = "mxd_sim983_get_pointers()";

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( sim983 == (MX_SIM983 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SIM983 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*sim983 = (MX_SIM983 *) amplifier->record->record_type_struct;

	if ( *sim983 == (MX_SIM983 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SIM983 pointer for record '%s' is NULL.",
			amplifier->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_sim983_command( MX_SIM983 *sim983,
		char *command,
		char *response,
		size_t max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxd_sim983_command()";

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
			fname, command, sim983->record->name));
	}

	mx_status = mx_rs232_putline( sim983->port_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( response != (char *) NULL ) {
		/* If a response is expected, read back the response. */

		mx_status = mx_rs232_getline( sim983->port_record,
					response, max_response_length,
					NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, sim983->record->name ));
		}
	}

	/* Check for errors in the previous command. */
#if 0

#if MXD_SIM983_ERROR_DEBUG
	MX_DEBUG(-2,("%s: sending '*ESR?' to '%s'",
		fname, sim983->record->name ));
#endif

	mx_status = mx_rs232_putline( sim983->port_record, "*ESR?", NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( sim983->port_record,
					esr_response, sizeof(esr_response),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SIM983_ERROR_DEBUG
	MX_DEBUG(-2,("%s: received '%s' from '%s'",
		fname, esr_response, sim983->record->name ));
#endif

	esr_byte = atol( esr_response );

	if ( esr_byte & 0x20 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Command error (CME) seen for command '%s' sent to '%s'.",
			command, sim983->record->name );
	} else
	if ( esr_byte & 0x10 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"Execution error (EXE) seen for command '%s' sent to '%s'.",
			command, sim983->record->name );
	} else
	if ( esr_byte & 0x08 ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "Device dependent error (DDE) seen for command '%s' sent to '%s'.",
			command, sim983->record->name );
	} else
	if ( esr_byte & 0x02 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Input data lost (INP) for command '%s' sent to '%s'.",
			command, sim983->record->name );
	} else
	if ( esr_byte & 0x04 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Output data lost (QYE) for command '%s' sent to '%s'.",
			command, sim983->record->name );
	}

#endif /* 0 */

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_sim983_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_sim983_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_SIM983 *sim983;

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPLIFIER structure.");
	}

	sim983 = (MX_SIM983 *)
				malloc( sizeof(MX_SIM983) );

	if ( sim983 == (MX_SIM983 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SIM983 structure." );
	}

	record->record_class_struct = amplifier;
	record->record_type_struct = sim983;

	record->class_specific_function_list
			= &mxd_sim983_amplifier_function_list;

	amplifier->record = record;
	sim983->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim983_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sim983_open()";

	MX_AMPLIFIER *amplifier;
	MX_SIM983 *sim983 = NULL;
	char response[100];
	char copy_of_response[100];
	int argc, num_items;
	char **argv;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	amplifier = (MX_AMPLIFIER *) record->record_class_struct;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Discard any outstanding characters. */

	mx_status = mx_rs232_discard_unwritten_output( sim983->port_record,
							MXD_SIM983_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( sim983->port_record,
							MXD_SIM983_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the communication interface by sending a break signal. */

#if MXD_SIM983_DEBUG
	MX_DEBUG(-2,("%s: sending a break signal to '%s'.",
				fname, record->name ));
#endif

	mx_status = mx_rs232_send_break( sim983->port_record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that we are connected to a SIM983 analog PID controller. */

	mx_status = mxd_sim983_command( sim983, "*IDN?",
					response, sizeof(response),
					MXD_SIM983_DEBUG );

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
	if ( strcmp( argv[1], "SIM983" ) != 0 ) {
		free( argv );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Device '%s' is not a SIM983 analog PID controller.  "
		"The response to '*IDN?' was '%s'.",
			record->name, response );
	}

	/* Get the version number. */

	num_items = sscanf( argv[3], "ver%lf", &(sim983->version) );

	if ( num_items != 1 ) {
		mx_status = mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Did not find the SIM983 version number in the token '%s' "
		"contained in the response '%s' to '*IDN?' by controller '%s'.",
			argv[3], response, record->name );

		free( argv );

		return mx_status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sim983_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sim983_get_gain()";

	MX_SIM983 *sim983;
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sim983_command( sim983, "GAIN?",
					response, sizeof(response),
					MXD_SIM983_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier->gain = atof( response );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim983_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sim983_set_gain()";

	MX_SIM983 *sim983;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "GAIN %lf", amplifier->gain );

	mx_status = mxd_sim983_command( sim983, command,
					NULL, 0, MXD_SIM983_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sim983_get_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sim983_get_offset()";

	MX_SIM983 *sim983;
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sim983_command( sim983, "OFST?",
					response, sizeof(response),
					MXD_SIM983_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier->offset = atof( response );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim983_set_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sim983_set_offset()";

	MX_SIM983 *sim983;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "OFST %lf", amplifier->offset );

	mx_status = mxd_sim983_command( sim983, command,
					NULL, 0, MXD_SIM983_DEBUG );

	return mx_status;
}

/* NOTE: The 'time constant' here is actually the Gain Bandwidth Product. */

MX_EXPORT mx_status_type
mxd_sim983_get_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sim983_get_time_constant()";

	MX_SIM983 *sim983;
	char response[100];
	mx_status_type mx_status;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sim983_command( sim983, "BWTH?",
					response, sizeof(response),
					MXD_SIM983_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier->time_constant = atof( response );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sim983_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_sim983_set_time_constant()";

	MX_SIM983 *sim983;
	char command[100];
	mx_status_type mx_status;

	mx_status = mxd_sim983_get_pointers( amplifier, &sim983, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"BWTH %lf", amplifier->time_constant );

	mx_status = mxd_sim983_command( sim983, command,
					NULL, 0, MXD_SIM983_DEBUG );

	return mx_status;
}

