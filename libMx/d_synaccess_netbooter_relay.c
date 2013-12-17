/*
 * Name:    d_synaccess_netbooter_relay.c
 *
 * Purpose: MX driver for using a Synaccess netBooter-controller power outlet
 *          as an MX relay record.
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

#define MXD_SA_NETBOOTER_RELAY_DEBUG 	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_relay.h"
#include "i_synaccess_netbooter.h"
#include "d_synaccess_netbooter_relay.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_sa_netbooter_relay_record_function_list = {
	NULL,
	mxd_sa_netbooter_relay_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_sa_netbooter_relay_open
};

MX_RELAY_FUNCTION_LIST mxd_sa_netbooter_relay_relay_function_list = {
	mxd_sa_netbooter_relay_relay_command,
	mxd_sa_netbooter_relay_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_sa_netbooter_relay_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_SYNACCESS_NETBOOTER_RELAY_STANDARD_FIELDS
};

long mxd_sa_netbooter_relay_num_record_fields
	= sizeof( mxd_sa_netbooter_relay_rf_defaults )
		/ sizeof( mxd_sa_netbooter_relay_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_sa_netbooter_relay_rfield_def_ptr
			= &mxd_sa_netbooter_relay_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_sa_netbooter_relay_get_pointers( MX_RELAY *relay,
		      MX_SYNACCESS_NETBOOTER_RELAY **synaccess_netbooter_relay,
		      MX_SYNACCESS_NETBOOTER **synaccess_netbooter,
		      const char *calling_fname )
{
	MX_SYNACCESS_NETBOOTER_RELAY *synaccess_netbooter_relay_ptr;
	MX_RECORD *synaccess_netbooter_record;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, calling_fname,
		"The MX_RELAY pointer passed is NULL." );
	}

	synaccess_netbooter_relay_ptr = (MX_SYNACCESS_NETBOOTER_RELAY *)
				relay->record->record_type_struct;

	if ( synaccess_netbooter_relay_ptr
			== (MX_SYNACCESS_NETBOOTER_RELAY *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
		"The MX_SYNACCESS_NETBOOTER_RELAY pointer "
		"for record '%s' is NULL.",
			relay->record->name );
	}

	if ( synaccess_netbooter_relay
			!= (MX_SYNACCESS_NETBOOTER_RELAY **) NULL )
	{
		*synaccess_netbooter_relay = synaccess_netbooter_relay_ptr;
	}

	if ( synaccess_netbooter != (MX_SYNACCESS_NETBOOTER **) NULL ) {

		synaccess_netbooter_record =
		    synaccess_netbooter_relay_ptr->synaccess_netbooter_record;

		if ( synaccess_netbooter_record == (MX_RECORD *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The synaccess_netbooter_record pointer used by "
			"relay record '%s' is NULL.",
				relay->record->name );
		}

		*synaccess_netbooter = (MX_SYNACCESS_NETBOOTER *)
			synaccess_netbooter_record->record_type_struct;

		if ( *synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
			return mx_error(
			MXE_CORRUPT_DATA_STRUCTURE, calling_fname,
			"The MX_SYNACCESS_NETBOOTER pointer for "
			"Synaccess Netbooter record '%s' used by "
			"relay record '%s' is NULL.",
				synaccess_netbooter_record->name,
				relay->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_sa_netbooter_relay_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_sa_netbooter_relay_create_record_structures()";

        MX_RELAY *relay;
        MX_SYNACCESS_NETBOOTER_RELAY *synaccess_netbooter_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_RELAY structure." );
        }

        synaccess_netbooter_relay = (MX_SYNACCESS_NETBOOTER_RELAY *)
				malloc( sizeof(MX_SYNACCESS_NETBOOTER_RELAY) );

        if ( synaccess_netbooter_relay
			== (MX_SYNACCESS_NETBOOTER_RELAY *) NULL )
	{
                return mx_error( MXE_OUT_OF_MEMORY, fname,
    "Cannot allocate memory for an MX_SYNACCESS_NETBOOTER_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = synaccess_netbooter_relay;
        record->class_specific_function_list
			= &mxd_sa_netbooter_relay_relay_function_list;

        relay->record = record;
	synaccess_netbooter_relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_sa_netbooter_relay_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_sa_netbooter_relay_open()";

	MX_RELAY *relay = NULL;
	MX_SYNACCESS_NETBOOTER_RELAY *synaccess_netbooter_relay = NULL;
	MX_SYNACCESS_NETBOOTER *synaccess_netbooter = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed is NULL." );
	}

	relay = (MX_RELAY *) record->record_class_struct;

	mx_status = mxd_sa_netbooter_relay_get_pointers( relay,
						&synaccess_netbooter_relay,
						&synaccess_netbooter, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_sa_netbooter_relay_get_relay_status( relay );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sa_netbooter_relay_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_sa_netbooter_relay_relay_command()";

	MX_SYNACCESS_NETBOOTER_RELAY *synaccess_netbooter_relay = NULL;
	MX_SYNACCESS_NETBOOTER *synaccess_netbooter = NULL;
	char command[80];
	char response[80];
	int port_is_on;
	mx_status_type mx_status;

	mx_status = mxd_sa_netbooter_relay_get_pointers( relay,
						&synaccess_netbooter_relay,
						&synaccess_netbooter, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		port_is_on = FALSE;
	} else {
		port_is_on = TRUE;
	}

	snprintf( command, sizeof(command), "$A3 %lu %d",
				synaccess_netbooter_relay->port_number,
				port_is_on );

	mx_status = mxi_synaccess_netbooter_command( synaccess_netbooter,
					command, response, sizeof(response),
					MXD_SA_NETBOOTER_RELAY_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_sa_netbooter_relay_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_sa_netbooter_relay_get_relay_status()";

	MX_SYNACCESS_NETBOOTER_RELAY *synaccess_netbooter_relay = NULL;
	MX_SYNACCESS_NETBOOTER *synaccess_netbooter = NULL;
	char response[80];
	size_t response_length;
	char *comma_ptr = NULL;
	int i, num_relays;
	char port_status;
	mx_status_type mx_status;

	mx_status = mxd_sa_netbooter_relay_get_pointers( relay,
						&synaccess_netbooter_relay,
						&synaccess_netbooter, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_synaccess_netbooter_command( synaccess_netbooter,
					"$A5", response, sizeof(response),
					MXD_SA_NETBOOTER_RELAY_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the response from the controller contains a comma (,)
	 * character, then the first comma follows after the field
	 * containing the relay status values.  If there _is_ no comma,
	 * then the response contains only relay status values.
	 */

	response_length = strlen( response );

	comma_ptr = strchr( response, ',' );

	if ( comma_ptr == NULL ) {
		num_relays = response_length;
	} else {
		num_relays = comma_ptr - response;
	}

	i = num_relays - synaccess_netbooter_relay->port_number;

	if ( i < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Port number %lu for Synaccess netBooter relay '%s' "
		"is outside the allowed range of values (1-%d).",
			synaccess_netbooter_relay->port_number,
			relay->record->name, num_relays );
	}

	port_status = response[i];

	if ( port_status == '0' ) {
		relay->relay_status = MXF_RELAY_IS_OPEN;
	} else {
		relay->relay_status = MXF_RELAY_IS_CLOSED;
	}

	return MX_SUCCESSFUL_RESULT;
}

