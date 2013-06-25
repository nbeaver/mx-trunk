/*
 * Name:    i_avt_vimba.c
 *
 * Purpose: MX interface driver for the Vimba C API used by
 *          cameras from Allied Vision Technologies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_AVT_VIMBA_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"

#include "i_avt_vimba.h"

MX_RECORD_FUNCTION_LIST mxi_avt_vimba_record_function_list = {
	NULL,
	mxi_avt_vimba_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_avt_vimba_open
};

MX_RECORD_FIELD_DEFAULTS mxi_avt_vimba_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_AVT_VIMBA_STANDARD_FIELDS
};

long mxi_avt_vimba_num_record_fields
		= sizeof( mxi_avt_vimba_record_field_defaults )
			/ sizeof( mxi_avt_vimba_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_avt_vimba_rfield_def_ptr
			= &mxi_avt_vimba_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_avt_vimba_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_avt_vimba_create_record_structures()";

	MX_AVT_VIMBA *avt_vimba;

	/* Allocate memory for the necessary structures. */

	avt_vimba = (MX_AVT_VIMBA *) malloc( sizeof(MX_AVT_VIMBA) );

	if ( avt_vimba == (MX_AVT_VIMBA *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AVT_VIMBA structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = avt_vimba;

	record->record_function_list = &mxi_avt_vimba_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	avt_vimba->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_avt_vimba_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_avt_vimba_open()";

	MX_AVT_VIMBA *avt_vimba;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	avt_vimba = (MX_AVT_VIMBA *) record->record_type_struct;

	if ( avt_vimba == (MX_AVT_VIMBA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AVT_VIMBA pointer for record '%s' is NULL.", record->name);
	}

#if MXI_AVT_VIMBA_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

