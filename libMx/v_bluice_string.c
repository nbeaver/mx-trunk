/*
 * Name:    v_bluice_string.c
 *
 * Purpose: Support for Blu-Ice string variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define BLUICE_STRING_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"

#include "mx_bluice.h"
#include "n_bluice_dcss.h"
#include "n_bluice_dhs.h"
#include "v_bluice_string.h"

MX_RECORD_FUNCTION_LIST mxv_bluice_string_record_function_list = {
	mx_variable_initialize_driver,
	mxv_bluice_string_create_record_structures,
	mxv_bluice_string_finish_record_initialization,
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
			mx_bool_type skip_foreign_device_check,
			const char *calling_fname )
{
	static const char fname[] = "mxv_bluice_string_get_pointers()";

	MX_RECORD *variable_record;
	MX_BLUICE_STRING *bluice_string_ptr;
	MX_RECORD *bluice_server_record;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	variable_record = variable->record;

	if ( variable_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_VARIABLE pointer %p is NULL.",
			variable );
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

	if ( variable_record->mx_type == MXV_BLUICE_SELF_STRING ) {
		/* If this is a "self" string, then we do not have 
		 * a Blu-Ice server.
		 */

		return MX_SUCCESSFUL_RESULT;
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

	if ( skip_foreign_device_check ) {
		return MX_SUCCESSFUL_RESULT;
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
mxv_bluice_string_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_bluice_string_finish_record_initialization()";

	MX_VARIABLE *variable;
	MX_BLUICE_STRING *bluice_string;
	MX_BLUICE_SERVER *bluice_server;
	MX_BLUICE_FOREIGN_DEVICE *fdev;
	MX_BLUICE_DHS_SERVER *bluice_dhs_server;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	bluice_string = record->record_type_struct;

	if ( bluice_string == (MX_BLUICE_STRING *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BLUICE_STRING pointer for record '%s' is NULL.",
			record->name );
	}

	/* A "self" string has nothing to initialize. */

	if ( record->mx_type == MXV_BLUICE_SELF_STRING ) {
		bluice_string->bluice_server_record = NULL;

		/* However, we do verify that the Blu-Ice server name
		 * is set to 'self'.
		 */

		if ( strcmp( bluice_string->bluice_server_name, "self" ) != 0 )
		{
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"bluice_self_string record '%s' does not "
			"list 'self' as its Blu-Ice server name.  "
			"Instead, it says that the server name is '%s'.",
			record->name, bluice_string->bluice_server_name );
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* DCSS and DHS strings need to find the Blu-Ice server record. */

	bluice_string->bluice_server_record =
		mx_get_record( record, bluice_string->bluice_server_name );

	if ( bluice_string->bluice_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"Blu-Ice string record '%s' uses Blu-Ice server record '%s' "
		"which was not found in the current MX database.",
			record->name, bluice_string->bluice_server_name );
	}

	/* The foreign device pointer for DCSS strings will be initialized
	 * later, when DCSS sends us the necessary configuration information.
	 */

	if ( record->mx_type == MXV_BLUICE_DCSS_STRING ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* The foreign device pointer for DHS strings will be initialized
	 * _now_, since _we_ are the source of the configuration information.
	 */

	variable = record->record_superclass_struct;

	mx_status = mxv_bluice_string_get_pointers( variable, NULL,
					&bluice_server, NULL, TRUE, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_bluice_setup_device_pointer(
					bluice_server,
					bluice_string->bluice_name,
					&(bluice_server->string_array),
					&(bluice_server->num_strings),
					&fdev );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bluice_string->foreign_device = fdev;

	fdev->foreign_type = MXT_BLUICE_FOREIGN_STRING;

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

	fdev->u.string.string_buffer = calloc(1, MXU_BLUICE_STRING_LENGTH);

	if ( fdev->u.string.string_buffer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d character string "
		"for Blu-Ice string variable '%s'.",
			MXU_BLUICE_STRING_LENGTH, record->name );
	}

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

	/* A "self" string has nothing to open. */

	if ( record->mx_type == MXV_BLUICE_SELF_STRING ) {
		return MX_SUCCESSFUL_RESULT;
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_bluice_string_get_pointers( variable, &bluice_string,
				&bluice_server, NULL, FALSE, fname );

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

	mx_status = mxv_bluice_string_get_pointers( variable, &bluice_string,
						NULL, NULL, FALSE, fname);

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

	mx_status = mxv_bluice_string_get_pointers( variable, &bluice_string,
				&bluice_server, &foreign_string, FALSE, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* A "self" string has nothing to copy. */

	if ( variable->record->mx_type == MXV_BLUICE_SELF_STRING ) {
		return MX_SUCCESSFUL_RESULT;
	}

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
		foreign_string->u.string.string_buffer,
		dimension_array[0] );

	mx_mutex_unlock( bluice_server->foreign_data_mutex );

	return mx_status;
}

