/*
 * Name:    i_newport_xps.c
 *
 * Purpose: MX driver for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NEWPORT_XPS_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_newport_xps.h"

MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list = {
	NULL,
	mxi_newport_xps_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_newport_xps_open
};

MX_RECORD_FIELD_DEFAULTS mxi_newport_xps_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_NEWPORT_XPS_STANDARD_FIELDS
};

long mxi_newport_xps_num_record_fields
		= sizeof( mxi_newport_xps_record_field_defaults )
			/ sizeof( mxi_newport_xps_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_newport_xps_rfield_def_ptr
			= &mxi_newport_xps_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_newport_xps_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_newport_xps_create_record_structures()";

	MX_NEWPORT_XPS *newport_xps = NULL;

	/* Allocate memory for the necessary structures. */

	newport_xps = (MX_NEWPORT_XPS *) malloc( sizeof(MX_NEWPORT_XPS) );

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NEWPORT_XPS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = newport_xps;

	record->record_function_list = &mxi_newport_xps_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	newport_xps->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_newport_xps_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_newport_xps_open()";

	MX_NEWPORT_XPS *newport_xps = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	newport_xps = (MX_NEWPORT_XPS *) record->record_type_struct;

	if ( newport_xps == (MX_NEWPORT_XPS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_NEWPORT_XPS pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

