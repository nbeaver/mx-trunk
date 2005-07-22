/*
 * Name:    d_soft_dinput.c
 *
 * Purpose: MX soft digital input device driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_digital_input.h"
#include "d_soft_dinput.h"

MX_RECORD_FUNCTION_LIST mxd_soft_dinput_record_function_list = {
	mxd_soft_dinput_initialize_type,
	mxd_soft_dinput_create_record_structures,
	mxd_soft_dinput_finish_record_initialization,
	mxd_soft_dinput_delete_record,
	NULL,
	mxd_soft_dinput_read_parms_from_hardware,
	mxd_soft_dinput_write_parms_to_hardware,
	mxd_soft_dinput_open,
	mxd_soft_dinput_close
};

MX_DIGITAL_INPUT_FUNCTION_LIST mxd_soft_dinput_digital_input_function_list = {
	mxd_soft_dinput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_dinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_INPUT_STANDARD_FIELDS
};

long mxd_soft_dinput_num_record_fields
		= sizeof( mxd_soft_dinput_record_field_defaults )
			/ sizeof( mxd_soft_dinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_dinput_rfield_def_ptr
			= &mxd_soft_dinput_record_field_defaults[0];

/* ===== */

MX_EXPORT mx_status_type
mxd_soft_dinput_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_create_record_structures( MX_RECORD *record )
{
        const char fname[] = "mxd_soft_dinput_create_record_structures()";

        MX_DIGITAL_INPUT *digital_input;
        MX_SOFT_DINPUT *soft_dinput;

        /* Allocate memory for the necessary structures. */

        digital_input = (MX_DIGITAL_INPUT *) malloc(sizeof(MX_DIGITAL_INPUT));

        if ( digital_input == (MX_DIGITAL_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_INPUT structure." );
        }

        soft_dinput = (MX_SOFT_DINPUT *)
				malloc( sizeof(MX_SOFT_DINPUT) );

        if ( soft_dinput == (MX_SOFT_DINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_SOFT_DINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_input;
        record->record_type_struct = soft_dinput;
        record->class_specific_function_list
			= &mxd_soft_dinput_digital_input_function_list;

        digital_input->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_delete_record( MX_RECORD *record )
{
        if ( record == NULL ) {
                return MX_SUCCESSFUL_RESULT;
        }
        if ( record->record_type_struct != NULL ) {
                free( record->record_type_struct );

                record->record_type_struct = NULL;
        }
        if ( record->record_class_struct != NULL ) {
                free( record->record_class_struct );

                record->record_class_struct = NULL;
        }
        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_dinput_read( MX_DIGITAL_INPUT *dinput )
{
	const char fname[] = "mxd_soft_dinput_read()";

	MX_DEBUG( 2,("%s: returning value %#lx (%ld)", fname,
				dinput->value, dinput->value ));
	return MX_SUCCESSFUL_RESULT;
}

