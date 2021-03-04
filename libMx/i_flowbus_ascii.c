/*
 * Name:    i_flowbus_ascii.c
 *
 * Purpose: Bronkhorst FLOW-BUS ASCII protocol interface driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_FLOWBUS_ASCII_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_flowbus_ascii.h"

MX_RECORD_FUNCTION_LIST mxi_flowbus_ascii_record_function_list = {
	NULL,
	mxi_flowbus_ascii_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_flowbus_ascii_open
};

MX_RECORD_FIELD_DEFAULTS mxi_flowbus_ascii_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_FLOWBUS_ASCII_STANDARD_FIELDS
};

long mxi_flowbus_ascii_num_record_fields
		= sizeof( mxi_flowbus_ascii_record_field_defaults )
			/ sizeof( mxi_flowbus_ascii_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_flowbus_ascii_rfield_def_ptr
			= &mxi_flowbus_ascii_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_flowbus_ascii_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_flowbus_ascii_create_record_structures()";

	MX_FLOWBUS_ASCII *flowbus_ascii;

	/* Allocate memory for the necessary structures. */

	flowbus_ascii = (MX_FLOWBUS_ASCII *) malloc( sizeof(MX_FLOWBUS_ASCII) );

	if ( flowbus_ascii == (MX_FLOWBUS_ASCII *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_FLOWBUS_ASCII structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = flowbus_ascii;

	record->record_function_list = &mxi_flowbus_ascii_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	flowbus_ascii->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_flowbus_ascii_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_flowbus_ascii_open()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	return MX_SUCCESSFUL_RESULT;
}

