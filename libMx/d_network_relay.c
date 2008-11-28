/*
 * Name:    d_network_relay.c
 *
 * Purpose: MX driver for network relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_relay.h"
#include "d_network_relay.h"

/* Initialize the network relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_relay_record_function_list = {
	NULL,
	mxd_network_relay_create_record_structures,
	mxd_network_relay_finish_record_initialization
};

MX_RELAY_FUNCTION_LIST mxd_network_relay_relay_function_list = {
	mxd_network_relay_relay_command,
	mxd_network_relay_get_relay_status,
	mxd_network_relay_pulse
};

MX_RECORD_FIELD_DEFAULTS mxd_network_relay_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_NETWORK_RELAY_STANDARD_FIELDS
};

long mxd_network_relay_num_record_fields
	= sizeof( mxd_network_relay_record_field_defaults )
		/ sizeof( mxd_network_relay_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_relay_rfield_def_ptr
			= &mxd_network_relay_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_network_relay_get_pointers( MX_RELAY *relay,
			MX_NETWORK_RELAY **network_relay,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_relay_get_pointers()";

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RELAY pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_relay == (MX_NETWORK_RELAY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_RELAY pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_relay = (MX_NETWORK_RELAY *)
				relay->record->record_type_struct;

	if ( *network_relay == (MX_NETWORK_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_RELAY pointer for relay record '%s' passed by '%s' is NULL",
			relay->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_network_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] = "mxd_network_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_NETWORK_RELAY *network_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        network_relay = (MX_NETWORK_RELAY *)
				malloc( sizeof(MX_NETWORK_RELAY) );

        if ( network_relay == (MX_NETWORK_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_NETWORK_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = network_relay;
        record->class_specific_function_list
			= &mxd_network_relay_relay_function_list;

        relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_relay_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_relay_finish_record_initialization()";

	MX_RELAY *relay;
	MX_NETWORK_RELAY *network_relay;
	mx_status_type mx_status;

	network_relay = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	relay = (MX_RELAY *) record->record_class_struct;

	mx_status = mxd_network_relay_get_pointers(
				relay, &network_relay, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_relay->pulse_duration_nf),
		network_relay->server_record,
		"%s.pulse_duration", network_relay->remote_record_name );

	mx_network_field_init( &(network_relay->pulse_on_value_nf),
		network_relay->server_record,
		"%s.pulse_on_value", network_relay->remote_record_name );

	mx_network_field_init( &(network_relay->pulse_off_value_nf),
		network_relay->server_record,
		"%s.pulse_off_value", network_relay->remote_record_name );

	mx_network_field_init( &(network_relay->relay_command_nf),
		network_relay->server_record,
		"%s.relay_command", network_relay->remote_record_name );

	mx_network_field_init( &(network_relay->relay_status_nf),
		network_relay->server_record,
		"%s.relay_status", network_relay->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_network_relay_relay_command()";

	MX_NETWORK_RELAY *network_relay;
	mx_status_type mx_status;

	network_relay = NULL;

	mx_status = mxd_network_relay_get_pointers(
					relay, &network_relay, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_relay->relay_command_nf),
				MXFT_LONG, &(relay->relay_command) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_relay_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_network_relay_get_relay_status()";

	MX_NETWORK_RELAY *network_relay;
	long relay_status;
	mx_status_type mx_status;

	network_relay = NULL;

	mx_status = mxd_network_relay_get_pointers(
					relay, &network_relay, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_relay->relay_status_nf),
				MXFT_LONG, &relay_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( relay_status == MXF_OPEN_RELAY ) {
		relay->relay_status = MXF_OPEN_RELAY;
	} else {
		relay->relay_status = MXF_CLOSE_RELAY;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_relay_pulse( MX_RELAY *relay )
{
	static const char fname[] = "mxd_network_relay_pulse()";

	MX_NETWORK_RELAY *network_relay;
	mx_status_type mx_status;

	network_relay = NULL;

	mx_status = mxd_network_relay_get_pointers(
					relay, &network_relay, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_relay->pulse_on_value_nf),
				MXFT_LONG, &(relay->pulse_on_value) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_relay->pulse_off_value_nf),
				MXFT_LONG, &(relay->pulse_off_value) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_relay->pulse_duration_nf),
				MXFT_DOUBLE, &(relay->pulse_duration) );

	return mx_status;
}

