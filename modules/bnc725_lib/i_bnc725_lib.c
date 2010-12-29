/*
 * Name:    i_bnc725_lib.c
 *
 * Purpose: MX driver for the vendor-provided Win32 C++ library for the
 *          BNC725 digital delay generator.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_BNC725_LIB_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "i_bnc725_lib.h"

MX_RECORD_FUNCTION_LIST mxi_bnc725_lib_record_function_list = {
	NULL,
	mxi_bnc725_lib_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_bnc725_lib_open,
	mxi_bnc725_lib_close
};

MX_RECORD_FIELD_DEFAULTS mxi_bnc725_lib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxi_bnc725_lib_num_record_fields
		= sizeof( mxi_bnc725_lib_record_field_defaults )
			/ sizeof( mxi_bnc725_lib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_bnc725_lib_rfield_def_ptr
			= &mxi_bnc725_lib_record_field_defaults[0];

static mx_status_type
mxi_bnc725_lib_get_pointers( MX_RECORD *record,
				MX_BNC725_LIB **bnc725_lib,
				const char *calling_fname )
{
	static const char fname[] = "mxi_bnc725_lib_get_pointers()";

	MX_BNC725_LIB *bnc725_lib_ptr;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	bnc725_lib_ptr = (MX_BNC725_LIB *) record->record_type_struct;

	if ( bnc725_lib_ptr == (MX_BNC725_LIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_BNC725_LIB pointer for record '%s' is NULL.",
			record->name );
	}

	if ( bnc725_lib != (MX_BNC725_LIB **) NULL ) {
		*bnc725_lib = bnc725_lib_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------*/

MX_EXPORT mx_status_type
mxi_bnc725_lib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_bnc725_lib_create_record_structures()";

	MX_BNC725_LIB *bnc725_lib;

	/* Allocate memory for the necessary structures. */

	bnc725_lib = (MX_BNC725_LIB *) malloc( sizeof(MX_BNC725_LIB) );

	if ( bnc725_lib == (MX_BNC725_LIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_BNC725_LIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = bnc725_lib;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	bnc725_lib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_lib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_bnc725_lib_open()";

	MX_BNC725_LIB *bnc725_lib;
	mx_status_type mx_status;

#if MXI_BNC725_LIB_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_bnc725_lib_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_bnc725_lib_close()";

	MX_BNC725_LIB *bnc725_lib;
	mx_status_type mx_status;

	mx_status = mxi_bnc725_lib_get_pointers( record, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

