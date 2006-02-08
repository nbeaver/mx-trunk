/*
 * Name:    d_soft_sca.c
 *
 * Purpose: MX single channel analyzer driver for software-emulated SCAs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_SCA_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_sca.h"
#include "d_soft_sca.h"

/* Initialize the SCA driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_sca_record_function_list = {
	NULL,
	mxd_soft_sca_create_record_structures,
	mxd_soft_sca_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_sca_open,
};

MX_SCA_FUNCTION_LIST mxd_soft_sca_sca_function_list = {
	mxd_soft_sca_get_parameter,
	mxd_soft_sca_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_sca_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCA_STANDARD_FIELDS,
	MXD_SOFT_SCA_STANDARD_FIELDS
};

mx_length_type mxd_soft_sca_num_record_fields
		= sizeof( mxd_soft_sca_record_field_defaults )
		  / sizeof( mxd_soft_sca_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_sca_rfield_def_ptr
			= &mxd_soft_sca_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_soft_sca_get_pointers( MX_SCA *sca,
			MX_SOFT_SCA **soft_sca,
			const char *calling_fname )
{
	const char fname[] = "mxd_soft_sca_get_pointers()";

	if ( sca == (MX_SCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sca pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( sca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for sca pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( sca->record->mx_type != MXT_SCA_SOFTWARE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The sca '%s' passed by '%s' is not a soft sca.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			sca->record->name, calling_fname,
			sca->record->mx_superclass,
			sca->record->mx_class,
			sca->record->mx_type );
	}

	if ( soft_sca != (MX_SOFT_SCA **) NULL ) {

		*soft_sca = (MX_SOFT_SCA *)
				(sca->record->record_type_struct);

		if ( *soft_sca == (MX_SOFT_SCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SOFT_SCA pointer for sca record '%s' passed by '%s' is NULL",
				sca->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_soft_sca_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_soft_sca_create_record_structures()";

	MX_SCA *sca;
	MX_SOFT_SCA *soft_sca;

	/* Allocate memory for the necessary structures. */

	sca = (MX_SCA *) malloc( sizeof(MX_SCA) );

	if ( sca == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCA structure." );
	}

	soft_sca = (MX_SOFT_SCA *) malloc( sizeof(MX_SOFT_SCA) );

	if ( soft_sca == (MX_SOFT_SCA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_SCA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = sca;
	record->record_type_struct = soft_sca;
	record->class_specific_function_list = &mxd_soft_sca_sca_function_list;

	sca->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_sca_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_soft_sca_finish_record_initialization()";

	MX_SCA *sca;
	MX_SOFT_SCA *soft_sca;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	sca = (MX_SCA *) record->record_class_struct;

	mx_status = mxd_soft_sca_get_pointers( sca, &soft_sca, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_sca_open( MX_RECORD *record )
{
	const char fname[] = "mxd_soft_sca_open()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_sca_get_parameter( MX_SCA *sca )
{
	return mx_sca_default_get_parameter_handler( sca );
}

MX_EXPORT mx_status_type
mxd_soft_sca_set_parameter( MX_SCA *sca )
{
	return mx_sca_default_set_parameter_handler( sca );
}

