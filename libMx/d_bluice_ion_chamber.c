/*
 * Name:    d_bluice_ion_chamber.c
 *
 * Purpose: MX analog input driver for Blu-Ice ion chambers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_ION_CHAMBER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"

#include "mx_bluice.h"
#include "d_bluice_ion_chamber.h"

/* Initialize the analog_input driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_bluice_ion_chamber_record_function_list = {
	NULL,
	mxd_bluice_ion_chamber_create_record_structures
};

MX_ANALOG_INPUT_FUNCTION_LIST
mxd_bluice_ion_chamber_analog_input_function_list = {
	mxd_bluice_ion_chamber_read
};

/* MX bluice analog_input data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_bluice_ion_chamber_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_BLUICE_ION_CHAMBER_STANDARD_FIELDS
};

long mxd_bluice_ion_chamber_num_record_fields
		= sizeof( mxd_bluice_ion_chamber_rf_defaults )
		  / sizeof( mxd_bluice_ion_chamber_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_ion_chamber_rfield_def_ptr
			= &mxd_bluice_ion_chamber_rf_defaults[0];

/*=======================================================================*/

/* WARNING: There is no guarantee that the foreign device pointer will
 * already be initialized when mxd_bluice_ion_chamber_get_pointers() is
 * invoked, so we have to test for this.
 */

static mx_status_type
mxd_bluice_ion_chamber_get_pointers( MX_ANALOG_INPUT *analog_input,
			MX_BLUICE_ION_CHAMBER **bluice_ion_chamber,
			MX_BLUICE_SERVER **bluice_server,
			MX_BLUICE_FOREIGN_DEVICE **foreign_ion_chamber,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_ion_chamber_get_pointers()";

	MX_RECORD *bluice_ion_chamber_record;
	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber_ptr;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server_ptr;
	MX_BLUICE_FOREIGN_DEVICE *foreign_ion_chamber_ptr;
	mx_status_type mx_status;
	long mx_status_code;

	/* In this section, we do standard MX ..._get_pointers() logic. */

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_ion_chamber_record = analog_input->record;

	if ( bluice_ion_chamber_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer %p "
		"passed was NULL.", analog_input );
	}

	bluice_ion_chamber_ptr = bluice_ion_chamber_record->record_type_struct;

	if ( bluice_ion_chamber_ptr == (MX_BLUICE_ION_CHAMBER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_ION_CHAMBER pointer for record '%s' is NULL.",
			bluice_ion_chamber_record->name );
	}

	if ( bluice_ion_chamber != (MX_BLUICE_ION_CHAMBER **) NULL ) {
		*bluice_ion_chamber = bluice_ion_chamber_ptr;
	}

	bluice_server_record = bluice_ion_chamber_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", bluice_ion_chamber_record->name );
	}

	bluice_server_ptr = bluice_server_record->record_class_struct;

	if ( bluice_server_ptr == (MX_BLUICE_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_SERVER pointer for Blu-Ice server "
		"record '%s' used by record '%s' is NULL.",
			bluice_server_record->name,
			bluice_ion_chamber_record->name );
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = bluice_server_ptr;
	}

	/* In this section, we check to see if the pointer to the Blu-Ice
	 * foreign device structure has been set up yet.
	 */

	if ( foreign_ion_chamber != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_ion_chamber = NULL;
	}

	foreign_ion_chamber_ptr = bluice_ion_chamber_ptr->foreign_device;

	if ( foreign_ion_chamber_ptr == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
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

#if BLUICE_ION_CHAMBER_DEBUG
		MX_DEBUG(-2,("%s: About to wait for device pointer "
			"initialization of ion chamber '%s' for function '%s'.",
			fname, bluice_ion_chamber_record->name, calling_fname));
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
					bluice_ion_chamber_ptr->bluice_name,
					MXT_BLUICE_FOREIGN_ION_CHAMBER,
					&(bluice_server_ptr->ion_chamber_array),
					&(bluice_server_ptr->num_ion_chambers),
					&foreign_ion_chamber_ptr,
					timeout );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_mutex_unlock(bluice_server_ptr->foreign_data_mutex);

			return mx_status;
		}

		foreign_ion_chamber_ptr->u.ion_chamber.mx_analog_input
						= analog_input;

		bluice_ion_chamber_ptr->foreign_device
						= foreign_ion_chamber_ptr;

		mx_mutex_unlock( bluice_server_ptr->foreign_data_mutex );

#if BLUICE_ION_CHAMBER_DEBUG
		MX_DEBUG(-2,("%s: Successfully waited for device pointer "
			"initialization of ion chamber '%s'.",
			fname, bluice_ion_chamber_record->name));
#endif
	}

	if ( foreign_ion_chamber != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_ion_chamber = foreign_ion_chamber_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_bluice_ion_chamber_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_bluice_ion_chamber_create_record_structures()";

	MX_ANALOG_INPUT *ainput;
	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;

	/* Allocate memory for the necessary structures. */

	ainput = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( ainput == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	bluice_ion_chamber = (MX_BLUICE_ION_CHAMBER *)
				malloc( sizeof(MX_BLUICE_ION_CHAMBER) );

	if ( bluice_ion_chamber == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_ION_CHAMBER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = ainput;
	record->record_type_struct = bluice_ion_chamber;
	record->class_specific_function_list
			= &mxd_bluice_ion_chamber_analog_input_function_list;

	ainput->record = record;
	bluice_ion_chamber->record = record;

	bluice_ion_chamber->foreign_device = NULL;

	/* Raw analog input values are stored as doubles. */

	ainput->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_ion_chamber_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_bluice_ion_chamber_read()";

	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_ion_chamber;
	mx_status_type mx_status;
	long mx_status_code;

	mx_status = mxd_bluice_ion_chamber_get_pointers( ainput,
				&bluice_ion_chamber, &bluice_server,
				&foreign_ion_chamber, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	ainput->raw_value.double_value =
			foreign_ion_chamber->u.ion_chamber.value;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );


#if BLUICE_ION_CHAMBER_DEBUG
	MX_DEBUG(-2,("%s: Analog input '%s' raw value = %g",
		fname, ainput->record->name, ainput->raw_value.double_value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

