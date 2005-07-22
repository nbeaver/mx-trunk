/*
 * Name:    d_spec_scaler.c
 *
 * Purpose: MX scaler driver to control Spec scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SPEC_SCALER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_spec.h"
#include "mx_scaler.h"
#include "d_spec_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_spec_scaler_record_function_list = {
	NULL,
	mxd_spec_scaler_create_record_structures
};

MX_SCALER_FUNCTION_LIST mxd_spec_scaler_scaler_function_list = {
	NULL,
	NULL,
	mxd_spec_scaler_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_scaler_default_get_parameter_handler,
	mx_scaler_default_set_parameter_handler
};

/* MX spec scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_spec_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_SPEC_SCALER_STANDARD_FIELDS
};

long mxd_spec_scaler_num_record_fields
		= sizeof( mxd_spec_scaler_record_field_defaults )
		  / sizeof( mxd_spec_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_spec_scaler_rfield_def_ptr
			= &mxd_spec_scaler_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_spec_scaler_get_pointers( MX_SCALER *scaler,
			MX_SPEC_SCALER **spec_scaler,
			const char *calling_fname )
{
	static const char fname[] = "mxd_spec_scaler_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The scaler pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( scaler->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for scaler pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( scaler->record->mx_type != MXT_SCL_SPEC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The scaler '%s' passed by '%s' is not a spec scaler.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			scaler->record->name, calling_fname,
			scaler->record->mx_superclass,
			scaler->record->mx_class,
			scaler->record->mx_type );
	}

	if ( spec_scaler != (MX_SPEC_SCALER **) NULL ) {

		*spec_scaler = (MX_SPEC_SCALER *)
				(scaler->record->record_type_struct);

		if ( *spec_scaler == (MX_SPEC_SCALER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SPEC_SCALER pointer for scaler record '%s' passed by '%s' is NULL",
				scaler->record->name, calling_fname );
		}
	}

	if ( (*spec_scaler)->spec_server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX server record pointer for MX spec scaler '%s' is NULL.",
			scaler->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_spec_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_spec_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_SPEC_SCALER *spec_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	spec_scaler = (MX_SPEC_SCALER *)
				malloc( sizeof(MX_SPEC_SCALER) );

	if ( spec_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SPEC_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = spec_scaler;
	record->class_specific_function_list
			= &mxd_spec_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_spec_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_spec_scaler_read()";

	MX_SPEC_SCALER *spec_scaler;
	char property_name[SV_NAME_LEN];
	long value;
	mx_status_type mx_status;

	mx_status = mxd_spec_scaler_get_pointers(
				scaler, &spec_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( property_name, "scaler/%s/value",
					spec_scaler->remote_scaler_name );

	mx_status = mx_spec_get_number( spec_scaler->spec_server_record,
				property_name, MXFT_LONG, &value );

	scaler->raw_value = value;

#if MXD_SPEC_SCALER_DEBUG
	MX_DEBUG(-2,("%s: '%s' = %ld", fname, property_name, value));
#endif

	return mx_status;
}

