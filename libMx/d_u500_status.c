/*
 * Name:    d_u500_status.c
 *
 * Purpose: MX digital input driver for reading status words from
 *          U500 motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_U500

#include "mx_util.h"
#include "mx_record.h"
#include "mx_digital_input.h"
#include "i_u500.h"
#include "d_u500_status.h"

/* Aerotech includes */

#include "Aerotype.h"
#include "Build.h"
#include "Quick.h"
#include "Wapi.h"

MX_RECORD_FUNCTION_LIST mxd_u500_status_record_function_list = {
	NULL,
	mxd_u500_status_create_record_structures
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_u500_status_digital_input_function_list = {
	mxd_u500_status_read
};

MX_RECORD_FIELD_DEFAULTS mxd_u500_status_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS,
	MXD_U500_STATUS_STANDARD_FIELDS
};

long mxd_u500_status_num_record_fields
		= sizeof( mxd_u500_status_record_field_defaults )
			/ sizeof( mxd_u500_status_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_u500_status_rfield_def_ptr
			= &mxd_u500_status_record_field_defaults[0];

static mx_status_type
mxd_u500_status_get_pointers( MX_DIGITAL_INPUT *dinput,
			MX_U500_STATUS **u500_status,
			MX_U500 **u500,
			const char *calling_fname )
{
	static const char fname[] = "mxd_u500_status_get_pointers()";

	MX_RECORD *u500_status_record, *u500_record;
	MX_U500_STATUS *u500_status_ptr;

	if ( dinput == (MX_DIGITAL_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DIGITAL_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	u500_status_record = dinput->record;

	if ( u500_status_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for MX_DIGITAL_INPUT pointer %p "
		"passed by '%s' was NULL.", dinput, calling_fname );
	}

	u500_status_ptr = (MX_U500_STATUS *) dinput->record->record_type_struct;

	if ( u500_status_ptr == (MX_U500_STATUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_U500_STATUS pointer for record '%s' passed by '%s' is NULL.",
			u500_status_record->name, calling_fname );
	}

	if ( u500_status != (MX_U500_STATUS **) NULL ) {
		*u500_status = u500_status_ptr;
	}

	if ( u500 == (MX_U500 **) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	u500_record = u500_status_ptr->u500_record;

	if ( u500_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_U500 pointer for U500 status "
		"record '%s' passed by '%s' is NULL.",
			u500_status_record->name, calling_fname );
	}

	*u500 = (MX_U500 *) u500_record->record_type_struct;

	if ( *u500 == (MX_U500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_U500 pointer for U500 record '%s' used by "
		"U500 status record '%s' and passed by '%s' is NULL.",
			u500_record->name,
			u500_status_record->name,
			calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_status_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_u500_status_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_U500_STATUS *u500_status;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        u500_status = (MX_U500_STATUS *) malloc( sizeof(MX_U500_STATUS) );

        if ( u500_status == (MX_U500_STATUS *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_U500_STATUS structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = u500_status;
        record->class_specific_function_list
			= &mxd_u500_status_digital_input_function_list;

        digital_input->record = record;
	u500_status->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_u500_status_read( MX_DIGITAL_INPUT *dinput )
{
	static const char fname[] = "mxd_u500_status_read()";

	MX_U500_STATUS *u500_status = NULL;
	MX_U500 *u500 = NULL;
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

	mx_status = mxd_u500_status_get_pointers( dinput,
			&u500_status, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_u500_select_board( u500, u500_status->board_number );

	if ( mx_status.code == MXE_SUCCESS )
		return mx_status;

	wapi_status = WAPIAerCheckStatus();

	if ( wapi_status != 0 ) {
		return mxi_u500_error( wapi_status, fname,
			"Attempt to check the U500 status for "
			"controller '%s' by record '%s' failed.",
			u500->record->name, dinput->record->name );
	}

	dinput->value = WAPIAerReadStatus( (SHORT) u500_status->status_type );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_U500 */

