/*
 * Name:    d_databox_mce.c
 *
 * Purpose: MX MCE driver to read stored Databox motor positions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2005-2006 Illinois Institute of Technology
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
#include "mx_measurement.h"
#include "mx_mce.h"
#include "mx_mcs.h"
#include "i_databox.h"
#include "d_databox_mce.h"
#include "d_databox_mcs.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_databox_encoder_record_function_list = {
	mxd_databox_encoder_initialize_type,
	mxd_databox_encoder_create_record_structures,
	mxd_databox_encoder_finish_record_initialization
};

MX_MCE_FUNCTION_LIST mxd_databox_encoder_mce_function_list = {
	NULL,
	NULL,
	mxd_databox_encoder_read,
	mxd_databox_encoder_get_current_num_values
};

/* DATABOX encoder data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_databox_encoder_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_DATABOX_ENCODER_STANDARD_FIELDS
};

mx_length_type mxd_databox_encoder_num_record_fields
		= sizeof( mxd_databox_encoder_record_field_defaults )
		  / sizeof( mxd_databox_encoder_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_databox_encoder_rfield_def_ptr
			= &mxd_databox_encoder_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_databox_encoder_get_pointers( MX_MCE *mce,
			MX_DATABOX_ENCODER **databox_encoder,
			MX_MCS **mcs,
			MX_DATABOX_MCS **databox_mcs,
			const char *calling_fname )
{
	const char fname[] = "mxd_databox_encoder_get_pointers()";

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( databox_encoder == (MX_DATABOX_ENCODER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DATABOX_ENCODER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mcs == (MX_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( databox_mcs == (MX_DATABOX_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*databox_encoder = (MX_DATABOX_ENCODER *)
					(mce->record->record_type_struct);

	if ( *databox_encoder == (MX_DATABOX_ENCODER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DATABOX_ENCODER pointer for record '%s' is NULL.",
			mce->record->name );
	}

	*mcs = (MX_MCS *)
		(*databox_encoder)->databox_mcs_record->record_class_struct;

	if ( *mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MCS pointer for Databox MCS record '%s' is NULL.",
			(*databox_encoder)->databox_mcs_record->name );
	}

	*databox_mcs = (MX_DATABOX_MCS *)
		(*databox_encoder)->databox_mcs_record->record_type_struct;

	if ( *databox_mcs == (MX_DATABOX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATABOX_MCS pointer for Databox MCS record '%s' is NULL.",
			(*databox_encoder)->databox_mcs_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_databox_encoder_initialize_type( long record_type )
{
	mx_length_type num_record_fields;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	mx_length_type maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_encoder_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_databox_encoder_create_record_structures()";

	MX_MCE *mce;
	MX_DATABOX_ENCODER *databox_encoder;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCE structure." );
	}

	databox_encoder = (MX_DATABOX_ENCODER *)
				malloc( sizeof(MX_DATABOX_ENCODER) );

	if ( databox_encoder == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_DATABOX_ENCODER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = databox_encoder;
	record->class_specific_function_list
			= &mxd_databox_encoder_mce_function_list;

	mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_encoder_finish_record_initialization( MX_RECORD *record )
{
	const char fname[]
		= "mxd_databox_encoder_finish_record_initialization()";

	MX_MCE *mce;
	MX_DATABOX_ENCODER *databox_encoder;
	MX_MCS *mcs;
	MX_DATABOX_MCS *databox_mcs;
	mx_status_type mx_status;

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_databox_encoder_get_pointers( mce,
				&databox_encoder, &mcs, &databox_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->encoder_type = MXT_MCE_ABSOLUTE_ENCODER;

	mce->current_num_values = mce->maximum_num_values;

        strlcpy( mce->selected_motor_name,
                        databox_encoder->associated_motor_record->name,
                        MXU_RECORD_NAME_LENGTH );

	mce->num_motors = 1;

	mce->motor_record_array = &(databox_encoder->associated_motor_record);

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_databox_encoder_read( MX_MCE *mce )
{
	const char fname[] = "mxd_databox_encoder_read()";

	MX_DATABOX_ENCODER *databox_encoder;
	MX_MCS *mcs;
	MX_DATABOX_MCS *databox_mcs;
	long i;
	double *motor_position_data;
	mx_status_type mx_status;

	mx_status = mxd_databox_encoder_get_pointers( mce,
				&databox_encoder, &mcs, &databox_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'", fname, mce->record->name));

	motor_position_data = databox_mcs->motor_position_data;

	if ( motor_position_data == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"motor_position_data pointer for Databox MCS record '%s' is NULL.",
			databox_encoder->databox_mcs_record->name );
	}

	mce->current_num_values = (long) mcs->current_num_measurements;

	MX_DEBUG( 2,("%s: mce->current_num_values = %ld",
		fname, (long) mce->current_num_values));

	for ( i = 0; i < mce->current_num_values; i++ ) {

		mce->value_array[i] = motor_position_data[i];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_databox_encoder_get_current_num_values( MX_MCE *mce )
{
	const char fname[] = "mxd_databox_encoder_get_current_num_values()";

	MX_DATABOX_ENCODER *databox_encoder;
	MX_MCS *mcs;
	MX_DATABOX_MCS *databox_mcs;
	mx_status_type mx_status;

	mx_status = mxd_databox_encoder_get_pointers( mce,
				&databox_encoder, &mcs, &databox_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'", fname, mce->record->name));

	mce->current_num_values = (long) mcs->current_num_measurements;

	MX_DEBUG( 2,("%s: mce->current_num_values = %ld",
		fname, (long) mce->current_num_values));

	return MX_SUCCESSFUL_RESULT;
}

