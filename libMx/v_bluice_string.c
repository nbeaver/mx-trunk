/*
 * Name:    v_bluice_string.c
 *
 * Purpose: Support for Blu-Ice string variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_STRING_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"

#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "v_bluice_string.h"

MX_RECORD_FUNCTION_LIST mxv_bluice_string_record_function_list = {
	mx_variable_initialize_type,
	mxv_bluice_string_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxv_bluice_string_open
};

MX_VARIABLE_FUNCTION_LIST mxv_bluice_string_variable_function_list = {
	mxv_bluice_string_send_variable,
	mxv_bluice_string_receive_variable
};

MX_RECORD_FIELD_DEFAULTS mxv_bluice_string_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BLUICE_STRING_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_bluice_string_num_record_fields
	= sizeof( mxv_bluice_string_field_defaults )
	/ sizeof( mxv_bluice_string_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_bluice_string_rfield_def_ptr
		= &mxv_bluice_string_field_defaults[0];

/*---*/

static mx_status_type
mxv_bluice_string_get_pointers( MX_VARIABLE *variable,
			MX_BLUICE_STRING **bluice_string,
			MX_BLUICE_SERVER **bluice_server,
			MX_BLUICE_FOREIGN_DEVICE **foreign_string,
			const char *calling_fname )
{
	static const char fname[] = "mxv_bluice_string_get_pointers()";

	MX_BLUICE_STRING *bluice_string_ptr;
	MX_RECORD *bluice_server_record;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( bluice_string == (MX_BLUICE_STRING **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_BLUICE_STRING pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bluice_string_ptr = (MX_BLUICE_STRING *)
				variable->record->record_type_struct;

	if ( bluice_string_ptr == (MX_BLUICE_STRING *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_STRING pointer for record '%s' is NULL.",
			variable->record->name );
	}

	if ( bluice_string != (MX_BLUICE_STRING **) NULL ) {
		*bluice_string = bluice_string_ptr;
	}

	bluice_server_record = bluice_string_ptr->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' "
		"is NULL.", variable->record->name );
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
	}

	if ( bluice_server != (MX_BLUICE_SERVER **) NULL ) {
		*bluice_server = (MX_BLUICE_SERVER *)
				bluice_server_record->record_class_struct;

		if ( (*bluice_server) == (MX_BLUICE_SERVER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BLUICE_SERVER pointer for Blu-Ice server "
			"record '%s' used by record '%s' is NULL.",
				bluice_server_record->name,
				variable->record->name );
		}
	}

	if ( foreign_string != (MX_BLUICE_FOREIGN_DEVICE **) NULL ) {
		*foreign_string = bluice_string_ptr->foreign_device;

		if ( (*foreign_string) == (MX_BLUICE_FOREIGN_DEVICE *) NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The foreign_string pointer for string '%s' "
			"has not been initialized.", variable->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_bluice_string_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxv_bluice_string_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_BLUICE_STRING *bluice_string;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	bluice_string = (MX_BLUICE_STRING *)
				malloc( sizeof(MX_BLUICE_STRING) );

	if ( bluice_string == (MX_BLUICE_STRING *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_BLUICE_STRING structure.");
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = bluice_string;

	record->superclass_specific_function_list =
				&mxv_bluice_string_variable_function_list;
	record->class_specific_function_list = NULL;

	bluice_string->foreign_device = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_string_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_bluice_string_open()";

	MX_VARIABLE *variable;
	MX_BLUICE_STRING *bluice_string;
	MX_RECORD *bluice_server_record;
	MX_BLUICE_SERVER *bluice_server;
	long num_dimensions, field_type;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_bluice_string_get_pointers( variable,
				&bluice_string, &bluice_server, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_server_record = bluice_string->bluice_server_record;

	if ( bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'bluice_server_record' pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mx_get_variable_parameters( record, &num_dimensions, NULL,
						&field_type, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( field_type != MXFT_STRING ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX field type for string variable '%s' is "
		"not MXFT_STRING (%d).  Instead, it is %ld.  "
		"This should not be able to happen.",
			record->name, MXFT_STRING, field_type );
	}

	if ( num_dimensions != 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Blu-Ice string record '%s' should be a 1-dimensional string.  "
		"Instead, it is a %ld-dimensional array.",
			record->name, num_dimensions );
	}

#if BLUICE_STRING_DEBUG
	MX_DEBUG(-2,
	    ("%s: About to wait for device pointer initialization.", fname));
#endif

	mx_status = mx_bluice_wait_for_device_pointer_initialization(
					bluice_server,
					bluice_string->bluice_name,
					MXT_BLUICE_FOREIGN_STRING,
					&(bluice_server->string_array),
					&(bluice_server->num_strings),
					&(bluice_string->foreign_device),
					5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if BLUICE_STRING_DEBUG
	MX_DEBUG(-2,
	("%s: Successfully waited for device pointer initialization.", fname));
#endif

	bluice_string->foreign_device->u.string.mx_string_variable = variable;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_bluice_string_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_string_send_variable()";

	MX_BLUICE_STRING *bluice_string;
	mx_status_type mx_status;

	mx_status = mxv_bluice_string_get_pointers( variable,
					&bluice_string, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Writing to a Blu-Ice string is not yet implemented." );
}

MX_EXPORT mx_status_type
mxv_bluice_string_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_bluice_string_receive_variable()";

	MX_BLUICE_STRING *bluice_string;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *foreign_string;
	long *dimension_array;
	void *value_ptr;
	mx_status_type mx_status;
	long mx_status_code;

	mx_status = mxv_bluice_string_get_pointers( variable,
			&bluice_string, &bluice_server, &foreign_string, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_parameters( variable->record, NULL,
						&dimension_array, NULL,
						&value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_code = mx_mutex_lock( bluice_server->foreign_data_mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"An attempt to lock the foreign data mutex for Blu-Ice "
		"server '%s' failed.", bluice_server->record->name );
	}

	strlcpy( value_ptr,
		foreign_string->u.string.string_contents,
		dimension_array[0] );

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return mx_status;
}

