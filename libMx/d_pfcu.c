/*
 * Name:    d_pfcu.c
 *
 * Purpose: MX driver for PF4 and PF2S2 filters and shutters controlled by
 *          an XIA PFCU RS-232 controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PFCU_DEBUG 	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_relay.h"
#include "mx_rs232.h"
#include "i_pfcu.h"
#include "d_pfcu.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_pfcu_record_function_list = {
	NULL,
	mxd_pfcu_create_record_structures
};

MX_RELAY_FUNCTION_LIST mxd_pfcu_relay_function_list = {
	mxd_pfcu_relay_command,
	mxd_pfcu_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_pfcu_filter_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_PFCU_FILTER_STANDARD_FIELDS
};

mx_length_type mxd_pfcu_filter_num_record_fields
	= sizeof( mxd_pfcu_filter_rf_defaults )
		/ sizeof( mxd_pfcu_filter_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_filter_rfield_def_ptr
			= &mxd_pfcu_filter_rf_defaults[0];

MX_RECORD_FIELD_DEFAULTS mxd_pfcu_shutter_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_PFCU_SHUTTER_STANDARD_FIELDS
};

mx_length_type mxd_pfcu_shutter_num_record_fields
	= sizeof( mxd_pfcu_shutter_rf_defaults )
		/ sizeof( mxd_pfcu_shutter_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pfcu_shutter_rfield_def_ptr
			= &mxd_pfcu_shutter_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_pfcu_get_pointers( MX_RELAY *relay,
			      MX_PFCU_RELAY **pfcu_relay,
			      MX_PFCU **pfcu,
			      const char *fname )
{
	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RELAY pointer passed is NULL." );
	}
	if ( pfcu_relay == (MX_PFCU_RELAY **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PFCU_RELAY pointer passed is NULL." );
	}
	if ( pfcu == (MX_PFCU **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PFCU pointer passed is NULL." );
	}

	*pfcu_relay = (MX_PFCU_RELAY *) relay->record->record_type_struct;

	if ( *pfcu_relay == (MX_PFCU_RELAY *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU_RELAY pointer for record '%s' is NULL.",
			relay->record->name );
	}

	*pfcu = (MX_PFCU *) (*pfcu_relay)->pfcu_record->record_type_struct;

	if ( *pfcu == (MX_PFCU *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PFCU pointer for PFCU record '%s' used by "
		"PFCU relay record '%s' is NULL.",
			(*pfcu_relay)->pfcu_record->name, relay->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_pfcu_create_record_structures()";

        MX_RELAY *relay;
        MX_PFCU_RELAY *pfcu_relay;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_RELAY structure." );
        }

        pfcu_relay = (MX_PFCU_RELAY *) malloc( sizeof(MX_PFCU_RELAY) );

        if ( pfcu_relay == (MX_PFCU_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_PFCU_RELAY structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = pfcu_relay;
        record->class_specific_function_list = &mxd_pfcu_relay_function_list;

        relay->record = record;
	pfcu_relay->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pfcu_relay_command( MX_RELAY *relay )
{
	const char fname[] = "mxd_pfcu_relay_command()";

	MX_PFCU_RELAY *pfcu_relay;
	MX_PFCU *pfcu;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_pfcu_get_pointers( relay, &pfcu_relay, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( relay->record->mx_type ) {
	case MXT_RLY_PFCU_FILTER:
		if ( relay->relay_command == MXF_OPEN_RELAY ) {
			sprintf( command, "R%d", pfcu_relay->filter_number );
		} else {
			sprintf( command, "I%d", pfcu_relay->filter_number );
		}
		break;
	case MXT_RLY_PFCU_SHUTTER:
		if ( relay->relay_command == MXF_OPEN_RELAY ) {
			strcpy( command, "O" );
		} else {
			/* If an exposure is in progress, a shutter close
			 * command can abort it.
			 */

			if ( pfcu->exposure_in_progress ) {
				mx_status = mx_rs232_discard_unread_input(
							pfcu->rs232_record,
							MXD_PFCU_DEBUG );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				pfcu->exposure_in_progress = FALSE;
			}
			strcpy( command, "C" );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"MX driver type '%s' for record '%s' is not supported "
			"for this function.",
			mx_get_driver_name( relay->record ),
			relay->record->name );
		break;
	}

	mx_status = mxi_pfcu_command( pfcu, pfcu_relay->module_number,
					command, NULL, 0, MXD_PFCU_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pfcu_get_relay_status( MX_RELAY *relay )
{
	const char fname[] = "mxd_pfcu_get_relay_status()";

	MX_PFCU_RELAY *pfcu_relay;
	MX_PFCU *pfcu;
	char response[40];
	int i;
	mx_status_type mx_status;

	mx_status = mxd_pfcu_get_pointers( relay, &pfcu_relay, &pfcu, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_pfcu_command( pfcu, pfcu_relay->module_number,
					"F", response, sizeof(response),
					MXD_PFCU_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( relay->record->mx_type ) {
	case MXT_RLY_PFCU_FILTER:
		i = pfcu_relay->filter_number - 1;

		switch( response[i] ) {
		case '0':
			relay->relay_status = MXF_RELAY_IS_OPEN;
			break;
		case '1':
			relay->relay_status = MXF_RELAY_IS_CLOSED;
			break;
		case '2':
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"PFCU filter '%s' has detected an open circuit.",
				relay->record->name );
			break;
		case '3':
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"PFCU filter '%s' has detected a short circuit.",
				relay->record->name );
			break;
		default:
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
    "PFCU filter '%s' has reported an illegal status '%c' in response '%s'.",
				relay->record->name, response[i], response );
			break;
		}
		break;

	case MXT_RLY_PFCU_SHUTTER:
		/* See if position 3 has a legal status. */

		switch( response[2] ) {
		case '0':
		case '1':
			/* This is a legal status.  Keep going. */
			break;
		case '2':
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "PFCU shutter '%s' has detected an open circuit for position 3.",
				relay->record->name );
			break;
		case '3':
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "PFCU shutter '%s' has detected a short circuit for position 3.",
				relay->record->name );
			break;
		default:
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"PFCU filter '%s' has reported an illegal status '%c' "
			"for position 3 in response '%s'.",
				relay->record->name, response[2], response );
			break;
		}

		/* See if position 4 has a legal status. */

		switch( response[3] ) {
		case '0':
		case '1':
			/* This is a legal status.  Keep going. */
			break;
		case '2':
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "PFCU shutter '%s' has detected an open circuit for position 4.",
				relay->record->name );
			break;
		case '3':
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
	    "PFCU shutter '%s' has detected a short circuit for position 4.",
				relay->record->name );
			break;
		default:
			relay->relay_status = MXF_RELAY_ILLEGAL_STATUS;

			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"PFCU filter '%s' has reported an illegal status '%c' "
			"for position 4 in response '%s'.",
				relay->record->name, response[3], response );
			break;
		}

		/* If we get here, then both position 3 and position 4 have
		 * a legal status.
		 */

		if ( (response[2] == '1') && (response[3] == '0') ) {
			relay->relay_status = MXF_RELAY_IS_OPEN;
		} else {
			relay->relay_status = MXF_RELAY_IS_CLOSED;
		}
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"MX driver type '%s' for record '%s' is not supported "
			"for this function.",
			mx_get_driver_name( relay->record ),
			relay->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

