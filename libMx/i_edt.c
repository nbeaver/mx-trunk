/*
 * Name:    i_edt.c
 *
 * Purpose: MX interface driver for EDT cameras.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_EDT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mxconfig.h"

#if HAVE_EDT

#include "mx_util.h"
#include "mx_record.h"
#include "i_edt.h"

#include "edtinc.h"

MX_RECORD_FUNCTION_LIST mxi_edt_record_function_list = {
	NULL,
	mxi_edt_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_edt_open
};

MX_RECORD_FIELD_DEFAULTS mxi_edt_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_EDT_STANDARD_FIELDS
};

long mxi_edt_num_record_fields
		= sizeof( mxi_edt_record_field_defaults )
			/ sizeof( mxi_edt_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_edt_rfield_def_ptr
			= &mxi_edt_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_edt_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_edt_create_record_structures()";

	MX_EDT *edt;

	/* Allocate memory for the necessary structures. */

	edt = (MX_EDT *) malloc( sizeof(MX_EDT) );

	if ( edt == (MX_EDT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EDT structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = edt;

	record->record_function_list = &mxi_edt_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	edt->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_edt_open()";

	MX_EDT *edt;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	edt = (MX_EDT *) record->record_type_struct;

	if ( edt == (MX_EDT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_EDT pointer for record '%s' is NULL.", record->name);
	}

#if MXI_EDT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EDT */

