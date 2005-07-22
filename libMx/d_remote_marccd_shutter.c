/*
 * Name:    d_remote_marccd_shutter.c
 *
 * Purpose: MX driver for generic relay support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_REMOTE_MARCCD_SHUTTER_DEBUG 	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_record.h"
#include "mx_driver.h"
#include "mx_socket.h"
#include "mx_ccd.h"
#include "mx_relay.h"
#include "d_remote_marccd.h"
#include "d_remote_marccd_shutter.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_remote_marccd_shutter_record_function_list = {
	NULL,
	mxd_remote_marccd_shutter_create_record_structures
};

MX_RELAY_FUNCTION_LIST mxd_remote_marccd_shutter_rly_function_list = {
	mxd_remote_marccd_shutter_relay_command,
	mxd_remote_marccd_shutter_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_remote_marccd_shutter_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_REMOTE_MARCCD_SHUTTER_STANDARD_FIELDS
};

long mxd_remote_marccd_shutter_num_record_fields
	= sizeof( mxd_remote_marccd_shutter_rf_defaults )
		/ sizeof( mxd_remote_marccd_shutter_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_remote_marccd_shutter_rfield_def_ptr
			= &mxd_remote_marccd_shutter_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_remote_marccd_shutter_get_pointers( MX_RELAY *relay,
			      MX_REMOTE_MARCCD_SHUTTER **remote_marccd_shutter,
			      MX_CCD **ccd,
			      MX_REMOTE_MARCCD **remote_marccd,
			      const char *fname )
{
	MX_RECORD *remote_marccd_record;

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}
	if ( remote_marccd_shutter == (MX_REMOTE_MARCCD_SHUTTER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_REMOTE_MARCCD_SHUTTER pointer passed is NULL." );
	}
	if ( ccd == (MX_CCD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_CCD pointer passed is NULL." );
	}
	if ( remote_marccd == (MX_REMOTE_MARCCD **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_REMOTE_MARCCD pointer passed is NULL." );
	}

	*remote_marccd_shutter = (MX_REMOTE_MARCCD_SHUTTER *)
				relay->record->record_type_struct;

	if ( *remote_marccd_shutter == (MX_REMOTE_MARCCD_SHUTTER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_REMOTE_MARCCD_SHUTTER pointer for record '%s' is NULL.",
			relay->record->name );
	}

	remote_marccd_record = (*remote_marccd_shutter)->marccd_record;

	if ( remote_marccd_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'marccd_record' pointer for record '%s' is NULL.",
			relay->record->name );
	}

	*ccd = (MX_CCD *) relay->record->record_class_struct;

	if ( *ccd == (MX_CCD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CCD pointer for record '%s' is NULL.",
			relay->record->name );
	}

	*remote_marccd = (MX_REMOTE_MARCCD *)
				relay->record->record_type_struct;

	if ( *remote_marccd == (MX_REMOTE_MARCCD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_REMOTE_MARCCD pointer for record '%s' is NULL.",
			relay->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_shutter_create_record_structures( MX_RECORD *record )
{
        const char fname[] =
		"mxd_remote_marccd_shutter_create_record_structures()";

        MX_RELAY *relay;
        MX_REMOTE_MARCCD_SHUTTER *remote_marccd_shutter;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        remote_marccd_shutter = (MX_REMOTE_MARCCD_SHUTTER *)
				malloc( sizeof(MX_REMOTE_MARCCD_SHUTTER) );

        if ( remote_marccd_shutter == (MX_REMOTE_MARCCD_SHUTTER *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_REMOTE_MARCCD_SHUTTER structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = remote_marccd_shutter;
        record->class_specific_function_list
			= &mxd_remote_marccd_shutter_rly_function_list;

        relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_shutter_relay_command( MX_RELAY *relay )
{
	const char fname[] = "mxd_remote_marccd_shutter_relay_command()";

	MX_REMOTE_MARCCD_SHUTTER *remote_marccd_shutter;
	MX_CCD *ccd;
	MX_REMOTE_MARCCD *remote_marccd;
	int marccd_flag;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_shutter_get_pointers( relay,
						&remote_marccd_shutter,
						&ccd, &remote_marccd, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		marccd_flag = 1;
	} else {
		marccd_flag = 0;
	}

	mx_status = mxd_remote_marccd_command( ccd, remote_marccd, command,
					MXD_REMOTE_MARCCD_SHUTTER_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_remote_marccd_shutter_get_relay_status( MX_RELAY *relay )
{
	const char fname[] = "mxd_remote_marccd_shutter_get_relay_status()";

	MX_REMOTE_MARCCD_SHUTTER *remote_marccd_shutter;
	MX_CCD *ccd;
	MX_REMOTE_MARCCD *remote_marccd;
	mx_status_type mx_status;

	mx_status = mxd_remote_marccd_shutter_get_pointers( relay,
						&remote_marccd_shutter,
						&ccd, &remote_marccd, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* I currently do not know how to get the shutter status, so I just
	 * return back the last value I sent to MarCCD.
	 */

	if ( relay->relay_command == MXF_OPEN_RELAY ) {
		relay->relay_status = MXF_RELAY_IS_OPEN;
	} else {
		relay->relay_status = MXF_RELAY_IS_CLOSED;
	}
#endif

	return mx_status;
}

