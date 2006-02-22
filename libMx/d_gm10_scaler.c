/*
 * Name:    d_gm10_scaler.c
 *
 * Purpose: MX scaler driver for Black Cat Systems GM-10, GM-45, GM-50, and
 *          GM-90 radiation detectors.
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

#define MXD_GM10_SCALER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_rs232.h"
#include "mx_scaler.h"
#include "d_gm10_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_gm10_scaler_record_function_list = {
	NULL,
	mxd_gm10_scaler_create_record_structures
};

MX_SCALER_FUNCTION_LIST mxd_gm10_scaler_scaler_function_list = {
	NULL,
	NULL,
	mxd_gm10_scaler_read,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_scaler_default_get_parameter_handler,
	mx_scaler_default_set_parameter_handler
};

/* MX gm10 scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_gm10_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_GM10_SCALER_STANDARD_FIELDS
};

long mxd_gm10_scaler_num_record_fields
		= sizeof( mxd_gm10_scaler_record_field_defaults )
		  / sizeof( mxd_gm10_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_gm10_scaler_rfield_def_ptr
			= &mxd_gm10_scaler_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_gm10_scaler_get_pointers( MX_SCALER *scaler,
			MX_GM10_SCALER **gm10_scaler,
			const char *calling_fname )
{
	static const char fname[] = "mxd_gm10_scaler_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( gm10_scaler == (MX_GM10_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GM10_SCALER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*gm10_scaler = (MX_GM10_SCALER *) scaler->record->record_type_struct;

	if ( (*gm10_scaler) == (MX_GM10_SCALER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_GM10_SCALER pointer for scaler record '%s' passed by '%s' is NULL",
			scaler->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_gm10_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_gm10_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_GM10_SCALER *gm10_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	gm10_scaler = (MX_GM10_SCALER *) malloc( sizeof(MX_GM10_SCALER) );

	if ( gm10_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GM10_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = gm10_scaler;
	record->class_specific_function_list
			= &mxd_gm10_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_gm10_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_gm10_scaler_read()";

	MX_GM10_SCALER *gm10_scaler;
	mx_status_type mx_status;

	mx_status = mxd_gm10_scaler_get_pointers(
				scaler, &gm10_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The 'gm10_timer' driver takes care of updating the current value
	 * of the scaler, so we do not actually need to do anything here.
	 */

#if MXD_GM10_SCALER_DEBUG
	MX_DEBUG(-2,("%s: Scaler '%s' value = %ld",
		fname, scaler->record->name, scaler->value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

