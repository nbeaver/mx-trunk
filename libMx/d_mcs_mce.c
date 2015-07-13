/*
 * Name:    d_mcs_mce.c
 *
 * Purpose: MX MCE driver to use pairs of MCS channels as incremental
 *          encoders.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003-2004, 2010 Illinois Institute of Technology
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
#include "mx_motor.h"
#include "d_mcs_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mcs_encoder_record_function_list = {
	mxd_mcs_encoder_initialize_driver,
	mxd_mcs_encoder_create_record_structures,
	mxd_mcs_encoder_finish_record_initialization
};

MX_MCE_FUNCTION_LIST mxd_mcs_encoder_mce_function_list = {
	NULL,
	NULL,
	mxd_mcs_encoder_read,
	mxd_mcs_encoder_get_current_num_values
};

/* MCS encoder data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mcs_encoder_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_MCS_ENCODER_STANDARD_FIELDS
};

long mxd_mcs_encoder_num_record_fields
		= sizeof( mxd_mcs_encoder_record_field_defaults )
		  / sizeof( mxd_mcs_encoder_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mcs_encoder_rfield_def_ptr
			= &mxd_mcs_encoder_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mcs_encoder_get_pointers( MX_MCE *mce,
			MX_MCS_ENCODER **mcs_encoder,
			MX_MCS **mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mcs_encoder_get_pointers()";

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs_encoder == (MX_MCS_ENCODER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCS_ENCODER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mcs_encoder = (MX_MCS_ENCODER *) (mce->record->record_type_struct);

	if ( *mcs_encoder == (MX_MCS_ENCODER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS_ENCODER pointer for record '%s' is NULL.",
			mce->record->name );
	}

	*mcs = (MX_MCS *) (*mcs_encoder)->mcs_record->record_class_struct;

	if ( *mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCS pointer for MCS record '%s' is NULL.",
			(*mcs_encoder)->mcs_record->name );
	}
	if ( (*mcs)->data_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"data_array pointer for MCS record '%s' is NULL.",
			(*mcs_encoder)->mcs_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_mcs_encoder_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_driver( driver,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_encoder_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mcs_encoder_create_record_structures()";

	MX_MCE *mce;
	MX_MCS_ENCODER *mcs_encoder;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCE structure." );
	}

	mcs_encoder = (MX_MCS_ENCODER *) malloc( sizeof(MX_MCS_ENCODER) );

	if ( mcs_encoder == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS_ENCODER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = mcs_encoder;
	record->class_specific_function_list
			= &mxd_mcs_encoder_mce_function_list;

	mce->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_encoder_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mcs_encoder_finish_record_initialization()";

	MX_MCE *mce;
	MX_MCS_ENCODER *mcs_encoder;
	MX_MCS *mcs;
	mx_status_type mx_status;

	mce = (MX_MCE *) record->record_class_struct;

	mx_status = mxd_mcs_encoder_get_pointers( mce,
					&mcs_encoder, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mce->window_is_available = FALSE;
	mce->use_window = FALSE;
	mce->window[0] = 0.0;
	mce->window[1] = 0.0;

	mce->encoder_type = MXT_MCE_DELTA_ENCODER;

	mce->current_num_values = mce->maximum_num_values;

	strlcpy( mce->selected_motor_name,
			mcs_encoder->associated_motor_record->name,
			MXU_RECORD_NAME_LENGTH );

	mce->num_motors = 1;

	mce->motor_record_array = &(mcs_encoder->associated_motor_record);

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the scaler channel numbers listed are in
	 * the legal range.
	 */

	if ( mcs_encoder->up_channel >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The up channel %lu for MCS encoder '%s' is outside the allowed range 0-%ld "
"for MCS '%s'.", mcs_encoder->up_channel, record->name,
		mcs->maximum_num_scalers - 1L,
		mcs_encoder->mcs_record->name );
	}

	if ( mcs_encoder->down_channel >= mcs->maximum_num_scalers ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The down channel %lu for MCS encoder '%s' is outside the allowed range 0-%ld "
"for MCS '%s'.", mcs_encoder->down_channel, record->name,
		mcs->maximum_num_scalers - 1L,
		mcs_encoder->mcs_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_encoder_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_mcs_encoder_read()";

	MX_MCS_ENCODER *mcs_encoder;
	MX_MCS *mcs;
	MX_MOTOR *associated_motor;
	double motor_scale, raw_encoder_value, scaled_encoder_value;
	long i;
	long up_channel, down_channel;
	long *up_value_array, *down_value_array;
	mx_status_type mx_status;

	mx_status = mxd_mcs_encoder_get_pointers( mce,
					&mcs_encoder, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'",
			fname, mce->record->name));

	if ( mcs_encoder->associated_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The associated_motor_record pointer for MCS MCE '%s' is NULL.",
			mce->record->name );
	}

	associated_motor = (MX_MOTOR *)
		mcs_encoder->associated_motor_record->record_class_struct;

	if ( associated_motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for associated motor record '%s' used "
		"by MCS MCE '%s' is NULL.",
			mcs_encoder->associated_motor_record->name,
			mce->record->name );
	}

	motor_scale = associated_motor->scale;

	up_channel = (long) mcs_encoder->up_channel;
	down_channel = (long) mcs_encoder->down_channel;

	mx_status = mx_mcs_read_scaler( mcs_encoder->mcs_record,
						up_channel, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mcs_read_scaler( mcs_encoder->mcs_record,
						down_channel, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	up_value_array = mcs->data_array[ up_channel ];
	down_value_array = mcs->data_array[ down_channel ];

	if ( mcs->current_num_measurements <= mce->maximum_num_values ) {
		mce->current_num_values = (long) mcs->current_num_measurements;
	} else {
		mce->current_num_values = mce->maximum_num_values;

		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"\007*** MX database configuration error ***\007\n"
	"MCS record '%s' currently contains %ld measurements, "
	"but MCE record '%s' is only configured for a maximum of %ld "
	"motor positions.  This means that the recorded motor positions "
	"for all measurements after measurement %ld will be INCORRECT!!!  "
	"This can only be fixed by modifying MX records '%s' and '%s' in "
	"your MX database configuration file.",
			mcs->record->name, (long) mcs->current_num_measurements,
			mce->record->name, mce->maximum_num_values,
			mce->maximum_num_values,
			mcs->record->name, mce->record->name );
	}

	for ( i = 0; i < mce->current_num_values; i++ ) {

		raw_encoder_value = (double)
			( up_value_array[i] - down_value_array[i] );

		scaled_encoder_value =
			mce->offset + mce->scale * raw_encoder_value;

		mce->value_array[i] = motor_scale * scaled_encoder_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_encoder_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] = "mxd_mcs_encoder_get_current_num_values()";

	MX_MCS_ENCODER *mcs_encoder;
	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mxd_mcs_encoder_get_pointers( mce,
					&mcs_encoder, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCE '%s'", fname, mce->record->name));

	mce->current_num_values = (long) mcs->current_num_measurements;

	MX_DEBUG( 2,("%s: mce->current_num_values = %ld",
		fname, mce->current_num_values));

	return MX_SUCCESSFUL_RESULT;
}

