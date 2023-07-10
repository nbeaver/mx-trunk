/*
 * Name:    d_handel_mapping_pixel_next.c
 *
 * Purpose: Handel driver that sends 'mapping_pixel_next' messages.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_mutex.h"
#include "mx_thread.h"
#include "mx_mca.h"
#include "mx_digital_output.h"

#include <handel.h>
#include <handel_constants.h>
#include <handel_errors.h>
#include <handel_generic.h>

#include "i_handel.h"
#include "d_handel_mapping_pixel_next.h"

MX_RECORD_FUNCTION_LIST mxd_handel_mpn_record_function_list = {
	NULL,
	mxd_handel_mpn_create_record_structures
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST mxd_handel_mpn_digital_output_function_list = {
	mxd_handel_mpn_read,
	mxd_handel_mpn_write
};

MX_RECORD_FIELD_DEFAULTS mxd_handel_mpn_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_HANDEL_MAPPING_PIXEL_NEXT_STANDARD_FIELDS
};

long mxd_handel_mpn_num_record_fields
		= sizeof( mxd_handel_mpn_record_field_defaults )
		/ sizeof( mxd_handel_mpn_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_mpn_rfield_def_ptr
			= &mxd_handel_mpn_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_handel_mpn_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_handel_mpn_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_HANDEL_MAPPING_PIXEL_NEXT *mapping_pixel_next;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *)
			malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an MX_DIGITAL_OUTPUT structure." );
        }

        mapping_pixel_next = (MX_HANDEL_MAPPING_PIXEL_NEXT *)
				malloc( sizeof(MX_HANDEL_MAPPING_PIXEL_NEXT) );

        if ( mapping_pixel_next == (MX_HANDEL_MAPPING_PIXEL_NEXT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Cannot allocate memory for an "
			"MX_HANDEL_MAPPING_PIXEL_NEXT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = mapping_pixel_next;
        record->class_specific_function_list
			= &mxd_handel_mpn_digital_output_function_list;

        digital_output->record = record;
	mapping_pixel_next->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mpn_read( MX_DIGITAL_OUTPUT *doutput )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_mpn_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_handel_mpn_write()";

	MX_HANDEL_MAPPING_PIXEL_NEXT *handel_mpn = NULL;
	MX_HANDEL *handel = NULL;
	int xia_status, ignored;
	mx_status_type mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

	/* Check that all of the pointers we need are set up correctly. */

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_DIGITAL_OUTPUT %p is NULL.",
			doutput );
	}

	handel_mpn = (MX_HANDEL_MAPPING_PIXEL_NEXT *)
			doutput->record->record_type_struct;

	if ( handel_mpn == (MX_HANDEL_MAPPING_PIXEL_NEXT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_MAPPING_PIXEL_NEXT pointer for "
		"digital output '%s' is NULL.", doutput->record->name );
	}

	if ( handel_mpn->handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for Handel 'mapping_pixel_next' "
		"driver is NULL." );
	}

	handel = (MX_HANDEL *) handel_mpn->handel_record->record_type_struct;

	if ( handel == (MX_HANDEL *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL pointer for Handel record '%s' used by "
		"the '%s' record is NULL.",
			handel_mpn->handel_record->name,
			doutput->record->name );
	}

	MX_DEBUG(-2,("%s: writing value %ld (%lu)", fname,
				doutput->value, doutput->value ));

	ignored = 0;

	MX_XIA_SYNC( xiaBoardOperation(
		0, "mapping_pixel_next", (void *) &ignored ) );

	if ( xia_status != XIA_SUCCESS ) {
		mx_status = mx_error( MXE_DEVICE_ACTION_FAILED,
		"Writing to 'mapping_pixel_next' for XIA "
		"Handel controller '%s' failed for MX record '%s'.  "
		"Error code = %d, '%s'.",
			handel->record->name,
			xia_status, mxi_handel_strerror(xia_status) );
	}

	return mx_status;
}

