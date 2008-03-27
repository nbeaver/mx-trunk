/*
 * Name:    d_bluice_shutter.c
 *
 * Purpose: MX driver for Blu-Ice controlled shutters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_SHUTTER_DEBUG 	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_relay.h"

#include "mx_bluice.h"
#include "n_bluice_dhs.h"
#include "d_bluice_shutter.h"

/* Initialize the generic relay driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_bluice_shutter_record_function_list = {
	NULL,
	mxd_bluice_shutter_create_record_structures,
	mxd_bluice_shutter_finish_record_initialization
};

MX_RELAY_FUNCTION_LIST mxd_bluice_shutter_relay_function_list = {
	mxd_bluice_shutter_relay_command,
	mxd_bluice_shutter_get_relay_status
};

MX_RECORD_FIELD_DEFAULTS mxd_bluice_shutter_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RELAY_STANDARD_FIELDS,
	MXD_BLUICE_SHUTTER_STANDARD_FIELDS
};

long mxd_bluice_shutter_num_record_fields
	= sizeof( mxd_bluice_shutter_rf_defaults )
		/ sizeof( mxd_bluice_shutter_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_shutter_rfield_def_ptr
			= &mxd_bluice_shutter_rf_defaults[0];

/*=======================================================================*/

/* WARNING: There is no guarantee that the foreign device pointer will
 * already be initialized when mxd_bluice_shutter_get_pointers() is
 * invoked, so we have to test for this.
 */

static mx_status_type
mxd_bluice_shutter_get_pointers( MX_RELAY *relay,
			MX_BLUICE_SHUTTER **bluice_shutter,
			MX_BLUICE_SERVER **bluice_server,
			MX_BLUICE_FOREIGN_DEVICE **foreign_shutter,
			mx_bool_type skip_foreign_device_check,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_shutter_get_pointers()";

	MX_RECORD *bluice_shutter_record;
	MX_BLUICE_SHUTTER *bluice_shutter_ptr;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server_ptr;
	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter_ptr;
	mx_status_type mx_status;
	long mx_status_code;

	/* In this section, we do standard MX ..._get_pointers() logic. */

	if ( relay == (MX_RELAY *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RELAY pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_shutter_record = relay->record;

	if ( bluice_shutter_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_RELAY pointer %p "
		"passed was NULL.", relay );
	}

	bluice_shutter_ptr = bluice_shutter_record->record_type_struct;

	if ( bluice_shutter_ptr == (MX_BLUICE_SHUTTER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SHUTTER pointer for record '%s' is NULL.",
			bluice_shutter_record->name );
	}

	if ( bluice_shutter != (MX_BLUICE_SHUTTER **) NULL ) {
		*bluice_shutter = bluice_shutter_ptr;
	}

	bluice_server_record = bluice_shutter_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", bluice_shutter_record->name );
	}

	bluice_server_ptr = bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for Blu-Ice server "
		"record '%s' used by record '%s' is NULL.",
			bluice_server_record->name,
			bluice_shutter_record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	if ( skip_foreign_device_check ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* In this section, we check to see if the pointer to the Blu-Ice
	 * foreign device structure has been set up yet.
	 */

	if ( foreign_shutter != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_shutter = NULL;
	}

	foreign_shutter_ptr = bluice_shutter_ptr->foreign_device;

	if ( foreign_shutter_ptr == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
		double timeout;

		/* If not, wait a while for the pointer to be set up. */

		switch( bluice_server_record->mx_type ) {
		case MXN_BLUICE_DCSS_SERVER:
			timeout = 5.0;
			break;
		case MXN_BLUICE_DHS_SERVER:
			timeout = 0.1;
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Blu-Ice server record '%s' should be either of type "
			"'bluice_dcss_server' or 'bluice_dhs_server'.  "
			"Instead, it is of type '%s'.",
				bluice_server_record->name,
				mx_get_driver_name( bluice_server_record ) );
		}

#if BLUICE_SHUTTER_DEBUG
		MX_DEBUG(-2,("%s: About to wait for device pointer "
			"initialization of shutter '%s' for function '%s'.",
			fname, bluice_shutter_record->name, calling_fname));
#endif
		mx_status_code = mx_mutex_lock(
				bluice_server_ptr->foreign_data_mutex );

		if ( mx_status_code != MXE_SUCCESS ) {
			return mx_error( mx_status_code, fname,
			"An attempt to lock the foreign data mutex for "
			"Blu-ice server '%s' failed.",
				bluice_server_record->name );
		}

		mx_status = mx_bluice_wait_for_device_pointer_initialization(
					bluice_server_ptr,
					bluice_shutter_ptr->bluice_name,
					MXT_BLUICE_FOREIGN_SHUTTER,
					&(bluice_server_ptr->shutter_array),
					&(bluice_server_ptr->num_shutters),
					&foreign_shutter_ptr,
					timeout );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_mutex_unlock(bluice_server_ptr->foreign_data_mutex);

			return mx_status;
		}

		foreign_shutter_ptr->u.shutter.mx_relay = relay;

		bluice_shutter_ptr->foreign_device = foreign_shutter_ptr;

		mx_mutex_unlock( bluice_server_ptr->foreign_data_mutex );

#if BLUICE_SHUTTER_DEBUG
		MX_DEBUG(-2,("%s: Successfully waited for device pointer "
			"initialization of shutter '%s'.",
			fname, bluice_shutter_record->name));
#endif
	}

	if ( foreign_shutter != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_shutter = foreign_shutter_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_bluice_shutter_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_bluice_shutter_create_record_structures()";

        MX_RELAY *relay;
        MX_BLUICE_SHUTTER *bluice_shutter;

        /* Allocate memory for the necessary structures. */

        relay = (MX_RELAY *) malloc(sizeof(MX_RELAY));

        if ( relay == (MX_RELAY *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Unable to allocate memory for an MX_RELAY structure." );
        }

        bluice_shutter = (MX_BLUICE_SHUTTER *)
				malloc( sizeof(MX_BLUICE_SHUTTER) );

        if ( bluice_shutter == (MX_BLUICE_SHUTTER *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_BLUICE_SHUTTER structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = relay;
        record->record_type_struct = bluice_shutter;
        record->class_specific_function_list =
				&mxd_bluice_shutter_relay_function_list;

        relay->record = record;
	bluice_shutter->record = record;

	bluice_shutter->foreign_device = NULL;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_shutter_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_bluice_shutter_finish_record_initialization()";

	MX_RELAY *relay;
	MX_BLUICE_SHUTTER *bluice_shutter;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *fdev;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	/* The foreign device pointer for DCSS controlled shutters
	 * will be initialized later, when DCSS sends us the necessary
	 * configuration information.
	 */

	if ( record->mx_type == MXT_RLY_BLUICE_DCSS_SHUTTER ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* The foreign device pointer for DHS controlled shutters
	 * will be initialized _now_, since _we_ are the source of the
	 * configuration information.
	 */

	relay = record->record_class_struct;

	mx_status = mxd_bluice_shutter_get_pointers( relay,
						&bluice_shutter,
						&bluice_server,
						NULL, TRUE, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					bluice_shutter->bluice_name,
					&(bluice_server->shutter_array),
					&(bluice_server->num_shutters),
					sizeof(MX_BLUICE_FOREIGN_DEVICE *),
					sizeof(MX_BLUICE_FOREIGN_DEVICE),
					&fdev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_shutter->foreign_device = fdev;

	fdev->foreign_type = MXT_BLUICE_FOREIGN_ION_CHAMBER;

	bluice_dhs_server = bluice_server->record->record_type_struct;

	if ( bluice_dhs_server == (MX_BLUICE_DHS_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_DHS_SERVER pointer for "
		"Blu-Ice DHS server '%s' is NULL.",
			bluice_server->record->name );
	}

	strlcpy( fdev->dhs_server_name,
		bluice_dhs_server->dhs_name,
		MXU_BLUICE_DHS_NAME_LENGTH );

	fdev->u.shutter.shutter_status = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_shutter_relay_command( MX_RELAY *relay )
{
	static const char fname[] = "mxd_bluice_shutter_relay_command()";

	MX_BLUICE_SHUTTER *bluice_shutter;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bluice_shutter_get_pointers( relay, &bluice_shutter,
			&bluice_server, &foreign_shutter, FALSE, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( bluice_server->record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
		switch( relay->relay_command ) {
		case MXF_OPEN_RELAY:
			snprintf( command, sizeof(command),
					"gtos_set_shutter_state %s open",
					bluice_shutter->bluice_name );
			break;
		case MXF_CLOSE_RELAY:
			snprintf( command, sizeof(command),
					"gtos_set_shutter_state %s closed",
					bluice_shutter->bluice_name );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal relay command %ld requested for Blu-Ice "
			"shutter '%s'.  The legal values are 0 and 1.",
				relay->relay_command, relay->record->name );
		}
		break;
	case MXN_BLUICE_DHS_SERVER:
		switch( relay->relay_command ) {
		case MXF_OPEN_RELAY:
			snprintf( command, sizeof(command),
					"stoh_set_shutter_state %s open",
					bluice_shutter->bluice_name );
			break;
		case MXF_CLOSE_RELAY:
			snprintf( command, sizeof(command),
					"stoh_set_shutter_state %s closed",
					bluice_shutter->bluice_name );
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal relay command %ld requested for Blu-Ice "
			"shutter '%s'.  The legal values are 0 and 1.",
				relay->relay_command, relay->record->name );
		}
	}

	mx_status = mx_bluice_check_for_master( bluice_server );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_send_message( bluice_server->record,
						command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bluice_shutter_get_relay_status( MX_RELAY *relay )
{
	static const char fname[] = "mxd_bluice_shutter_get_relay_status()";

	MX_BLUICE_SHUTTER *bluice_shutter;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_shutter;
	mx_status_type mx_status;
	long mx_status_code;

	mx_status = mxd_bluice_shutter_get_pointers( relay, &bluice_shutter,
			&bluice_server, &foreign_shutter, FALSE, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}


	relay->relay_status = foreign_shutter->u.shutter.shutter_status;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return mx_status;
}

