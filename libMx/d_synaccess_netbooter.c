/*
 * Name:    d_synaccess_netbooter.c
 *
 * Purpose: MX driver for generic relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2008-2009, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SYNACCESS_NETBOOTER_DEBUG 	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_thread.h"
#include "mx_relay.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_marccd_server_socket.h"
#include "d_marccd.h"
#include "d_synaccess_netbooter.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_synaccess_netbooter_record_function_list = {
	NULL,
	mxd_synaccess_netbooter_create_record_structures
};

MX_RELAY_FUNCTION_LIST mxd_synaccess_netbooter_rly_function_list = {
	mxd_synaccess_netbooter_relay_command,
	mxd_synaccess_netbooter_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_synaccess_netbooter_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_SYNACCESS_NETBOOTER_STANDARD_FIELDS
};

long mxd_synaccess_netbooter_num_record_fields
	= sizeof( mxd_synaccess_netbooter_rf_defaults )
		/ sizeof( mxd_synaccess_netbooter_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_synaccess_netbooter_rfield_def_ptr
			= &mxd_synaccess_netbooter_rf_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_synaccess_netbooter_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_synaccess_netbooter_create_record_structures()";

        MX_RELAY *relay;
        MX_SYNACCESS_NETBOOTER *synaccess_netbooter;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_RELAY structure." );
        }

        synaccess_netbooter = (MX_SYNACCESS_NETBOOTER *)
				malloc( sizeof(MX_SYNACCESS_NETBOOTER) );

        if ( synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for an MX_SYNACCESS_NETBOOTER structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = synaccess_netbooter;
        record->class_specific_function_list
			= &mxd_synaccess_netbooter_rly_function_list;

        relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_synaccess_netbooter_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_synaccess_netbooter_relay_command()";

	MX_SYNACCESS_NETBOOTER *synaccess_netbooter;
	MX_MARCCD *marccd = NULL;
	MX_MARCCD_SERVER_SOCKET *mss = NULL;
	MX_RECORD *marccd_record;
	int marccd_flag;
	char command[40];
	mx_status_type mx_status;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RELAY pointer passed was NULL." );
	}
	if ( relay->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_RELAY %p is NULL.", relay );
	}

	synaccess_netbooter =
		(MX_SYNACCESS_NETBOOTER *) relay->record->record_type_struct;

	if ( synaccess_netbooter == (MX_SYNACCESS_NETBOOTER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SYNACCESS_NETBOOTER pointer for record '%s' is NULL.",
			relay->record->name );
	}

	marccd_record = synaccess_netbooter->marccd_record;

	if ( marccd_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The marccd_record pointer for MarCCD shutter '%s' is NULL.",
			relay->record->name );
	}

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		marccd_flag = 1;
	} else {
		marccd_flag = 0;
	}

	snprintf( command, sizeof(command), "shutter,%d", marccd_flag );

	switch( marccd_record->mx_type ) {
	case MXT_AD_MARCCD:
		marccd = marccd_record->record_type_struct;

		mx_status = mxd_marccd_command( marccd, command,
						MXF_MARCCD_CMD_SHUTTER,
						MXD_SYNACCESS_NETBOOTER_DEBUG );
		break;
	case MXT_AD_MARCCD_SERVER_SOCKET:
		mss = marccd_record->record_type_struct;

		mx_status = mxd_marccd_server_socket_command( mss, command,
					NULL, 0, MXD_SYNACCESS_NETBOOTER_DEBUG );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"marccd_record '%s' for MarCCD shutter '%s' is not of type "
		"'marccd' or 'marccd_server_socket'.  Instead, it is of "
		"type '%s'.", marccd_record->name, relay->record->name,
			mx_get_driver_name( marccd_record ) );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_synaccess_netbooter_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_synaccess_netbooter_get_relay_status()";

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RELAY pointer passed was NULL." );
	}

	/* I currently do not know how to get the shutter status, so I just
	 * return back the last value I sent to MarCCD.
	 */

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		relay->relay_status = MXF_RELAY_IS_OPEN;
	} else {
		relay->relay_status = MXF_RELAY_IS_CLOSED;
	}

	return MX_SUCCESSFUL_RESULT;
}

