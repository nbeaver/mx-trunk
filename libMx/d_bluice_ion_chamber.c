/*
 * Name:    d_bluice_ion_chamber.c
 *
 * Purpose: MX scaler driver for Blu-Ice ion chambers used as scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BLUICE_ION_CHAMBER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_scaler.h"

#include "mx_bluice.h"
#include "d_bluice_ion_chamber.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_bluice_ion_chamber_record_function_list = {
	NULL,
	mxd_bluice_ion_chamber_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_bluice_ion_chamber_open
};

MX_SCALER_FUNCTION_LIST mxd_bluice_ion_chamber_scaler_function_list = {
	mxd_bluice_ion_chamber_clear,
	NULL,
	mxd_bluice_ion_chamber_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_scaler_default_get_parameter_handler,
	mx_scaler_default_set_parameter_handler
};

/* MX bluice scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_bluice_ion_chamber_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_BLUICE_ION_CHAMBER_STANDARD_FIELDS
};

long mxd_bluice_ion_chamber_num_record_fields
		= sizeof( mxd_bluice_ion_chamber_record_field_defaults )
		  / sizeof( mxd_bluice_ion_chamber_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bluice_ion_chamber_rfield_def_ptr
			= &mxd_bluice_ion_chamber_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_bluice_ion_chamber_get_pointers( MX_SCALER *scaler,
			MX_BLUICE_ION_CHAMBER **bluice_ion_chamber,
			MX_BLUICE_SERVER **bluice_server,
			MX_BLUICE_FOREIGN_ION_CHAMBER **foreign_ion_chamber,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bluice_ion_chamber_get_pointers()";

	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber_ptr;
	MX_RECORD *bluice_server_record;

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_ion_chamber == (MX_BLUICE_ION_CHAMBER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_ION_CHAMBER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_ion_chamber_ptr = (MX_BLUICE_ION_CHAMBER *)
				scaler->record->record_type_struct;

	if ( bluice_ion_chamber_ptr == (MX_BLUICE_ION_CHAMBER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_ION_CHAMBER pointer for record '%s' is NULL.",
			scaler->record->name );
	}

	if ( bluice_ion_chamber != (MX_BLUICE_ION_CHAMBER **) NULL ) {
		*bluice_ion_chamber = bluice_ion_chamber_ptr;
	}

	bluice_server_record = bluice_ion_chamber_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", scaler->record->name );
	}

	switch( bluice_server_record->mx_type ) {
	case MXN_BLUICE_DCSS_SERVER:
	case MXN_BLUICE_DHS_SERVER:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Blu-Ice server record '%s' should be either of type "
		"'bluice_dcss_server' or 'bluice_dhs_server'.  Instead, it is "
		"of type '%s'.",
			bluice_server_record->name,
			mx_get_driver_name( bluice_server_record ) );
		break;
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

		if ( (*bluice_server) == (MX_BLUICE_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_SERVER pointer for Blu-Ice server "
			"record '%s' used by record '%s' is NULL.",
				bluice_server_record->name,
				scaler->record->name );
		}
	}

	if ( foreign_ion_chamber != (MX_BLUICE_FOREIGN_ION_CHAMBER **) NULL ) {
		*foreign_ion_chamber =
				bluice_ion_chamber_ptr->foreign_ion_chamber;

		if ( (*foreign_ion_chamber) ==
				(MX_BLUICE_FOREIGN_ION_CHAMBER *) NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The foreign_ion_chamber pointer for scaler '%s' "
			"has not been initialized.", scaler->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_bluice_ion_chamber_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_bluice_ion_chamber_create_record_structures()";

	MX_SCALER *scaler;
	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	bluice_ion_chamber = (MX_BLUICE_ION_CHAMBER *) malloc( sizeof(MX_BLUICE_ION_CHAMBER) );

	if ( bluice_ion_chamber == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_BLUICE_ION_CHAMBER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = bluice_ion_chamber;
	record->class_specific_function_list
			= &mxd_bluice_ion_chamber_scaler_function_list;

	scaler->record = record;

	bluice_ion_chamber->foreign_ion_chamber = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_ion_chamber_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bluice_ion_chamber_open()";

	MX_SCALER *scaler;
	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *device_ptr;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	scaler = (MX_SCALER *) record->record_class_struct;

	mx_status = mxd_bluice_ion_chamber_get_pointers( scaler,
			&bluice_ion_chamber, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	    ("%s: About to wait for device pointer initialization.", fname));

	mx_status = mx_bluice_wait_for_device_pointer_initialization(
						bluice_server,
						bluice_ion_chamber->bluice_name,
	    (MX_BLUICE_FOREIGN_DEVICE ***) &(bluice_server->ion_chamber_array),
					&(bluice_server->num_ion_chambers),
						&device_ptr,
						5.0 );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_ion_chamber->foreign_ion_chamber =
			(MX_BLUICE_FOREIGN_ION_CHAMBER *) device_ptr;

	bluice_ion_chamber->foreign_ion_chamber->mx_scaler = scaler;

	MX_DEBUG(-2,
	("%s: Scaler '%s', bluice_ion_chamber->foreign_ion_chamber = %p",
		fname, record->name, bluice_ion_chamber->foreign_ion_chamber));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_ion_chamber_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_bluice_ion_chamber_clear()";

	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_ION_CHAMBER *foreign_ion_chamber;
	mx_status_type mx_status;

	mx_status = mxd_bluice_ion_chamber_get_pointers( scaler,
				&bluice_ion_chamber, &bluice_server,
				&foreign_ion_chamber, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_mutex_lock( bluice_server->foreign_data_mutex );

	foreign_ion_chamber->value = 0;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	scaler->value = 0;

#if MXD_BLUICE_ION_CHAMBER_DEBUG
	MX_DEBUG(-2,("%s: Scaler '%s' cleared.",
		fname, scaler->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bluice_ion_chamber_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_bluice_ion_chamber_read()";

	MX_BLUICE_ION_CHAMBER *bluice_ion_chamber;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_ION_CHAMBER *foreign_ion_chamber;
	mx_status_type mx_status;

	mx_status = mxd_bluice_ion_chamber_get_pointers( scaler,
				&bluice_ion_chamber, &bluice_server,
				&foreign_ion_chamber, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME!!! Blu-Ice is actually reporting floating point values for
	 * the ion chambers, so we need to change the 'bluice_ion_chamber'
	 * driver from a scaler driver to an analog input driver.
	 */

	mx_mutex_lock( bluice_server->foreign_data_mutex );

	scaler->raw_value = foreign_ion_chamber->value;

	mx_mutex_unlock( bluice_server->foreign_data_mutex );


#if MXD_BLUICE_ION_CHAMBER_DEBUG
	MX_DEBUG(-2,("%s: Scaler '%s' raw value = %ld",
		fname, scaler->record->name, scaler->raw_value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

