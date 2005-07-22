/*
 * Name:    d_mcs_time_mce.c
 *
 * Purpose: MX MCE driver to report the elapsed MCS time as a multichannel
 *          encoder array.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003 Illinois Institute of Technology
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
#include "d_mcs_time_mce.h"

/* Initialize the MCE driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_mcs_time_mce_record_function_list = {
	mxd_mcs_time_mce_initialize_type,
	mxd_mcs_time_mce_create_record_structures,
	mxd_mcs_time_mce_finish_record_initialization
};

MX_MCE_FUNCTION_LIST mxd_mcs_time_mce_mce_function_list = {
	NULL,
	NULL,
	mxd_mcs_time_mce_read,
	mxd_mcs_time_mce_get_current_num_values
};

/* MCS encoder data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_mcs_time_mce_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCE_STANDARD_FIELDS,
	MXD_MCS_TIME_MCE_STANDARD_FIELDS
};

long mxd_mcs_time_mce_num_record_fields
		= sizeof( mxd_mcs_time_mce_record_field_defaults )
		  / sizeof( mxd_mcs_time_mce_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mcs_time_mce_rfield_def_ptr
			= &mxd_mcs_time_mce_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_mcs_time_mce_get_pointers( MX_MCE *mce,
			MX_MCS_TIME_MCE **mcs_time_mce,
			MX_MCS **mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mcs_time_mce_get_pointers()";

	if ( mce == (MX_MCE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs_time_mce == (MX_MCS_TIME_MCE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCS_TIME_MCE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mcs_time_mce = (MX_MCS_TIME_MCE *) (mce->record->record_type_struct);

	if ( *mcs_time_mce == (MX_MCS_TIME_MCE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCS_TIME_MCE pointer for record '%s' is NULL.",
			mce->record->name );
	}

	*mcs = (MX_MCS *) (*mcs_time_mce)->mcs_record->record_class_struct;

	if ( *mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCS pointer for MCS record '%s' is NULL.",
			(*mcs_time_mce)->mcs_record->name );
	}
	if ( (*mcs)->data_array == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"data_array pointer for MCS record '%s' is NULL.",
			(*mcs_time_mce)->mcs_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_mcs_time_mce_initialize_type( long record_type )
{
	long num_record_fields;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long maximum_num_values_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mce_initialize_type( record_type,
					&num_record_fields,
					&record_field_defaults,
					&maximum_num_values_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_time_mce_create_record_structures( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_mcs_time_mce_create_record_structures()";

	MX_MCE *mce;
	MX_MCS_TIME_MCE *mcs_time_mce;

	/* Allocate memory for the necessary structures. */

	mce = (MX_MCE *) malloc( sizeof(MX_MCE) );

	if ( mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCE structure." );
	}

	mcs_time_mce = (MX_MCS_TIME_MCE *) malloc( sizeof(MX_MCS_TIME_MCE) );

	if ( mcs_time_mce == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS_TIME_MCE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mce;
	record->record_type_struct = mcs_time_mce;
	record->class_specific_function_list
			= &mxd_mcs_time_mce_mce_function_list;

	mce->record = record;
	mce->encoder_type = MXT_MCE_ABSOLUTE_ENCODER;
	mce->current_num_values = mce->maximum_num_values;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_time_mce_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_mcs_time_mce_finish_record_initialization()";

	MX_MCE *mce;
	MX_MCS_TIME_MCE *mcs_time_mce;
	MX_MCS *mcs;
	mx_status_type mx_status;

	if ( record == ( MX_RECORD * ) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	mce = ( MX_MCE * ) record->record_class_struct;

	mx_status = mxd_mcs_time_mce_get_pointers( mce,
					&mcs_time_mce, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

        mx_strncpy( mce->selected_motor_name,
                        mcs_time_mce->associated_motor_record->name,
                        MXU_RECORD_NAME_LENGTH );

	mce->num_motors = 1;

	mce->motor_record_array = &(mcs_time_mce->associated_motor_record);

	mx_status = mx_mce_fixup_motor_record_array_field( mce );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mcs_time_mce_read( MX_MCE *mce )
{
	static const char fname[] = "mxd_mcs_time_mce_read()";

	MX_MCS_TIME_MCE *mcs_time_mce;
	MX_MCS *mcs;
	long i;
	double measurement_time;
	mx_status_type mx_status;

	mx_status = mxd_mcs_time_mce_get_pointers( mce,
					&mcs_time_mce, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	measurement_time = mcs->measurement_time;

	for ( i = 0; i < mce->current_num_values; i++ ) {
		mce->value_array[i] = measurement_time * (double) i;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mcs_time_mce_get_current_num_values( MX_MCE *mce )
{
	static const char fname[] = "mxd_mcs_time_mce_get_current_num_values()";

	MX_MCS_TIME_MCE *mcs_time_mce;
	MX_MCS *mcs;
	mx_status_type mx_status;

	mx_status = mxd_mcs_time_mce_get_pointers( mce,
					&mcs_time_mce, &mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for MCE '%s'", fname, mce->record->name));

	mce->current_num_values = (long) mcs->current_num_measurements;

	MX_DEBUG(-2,("%s: mce->current_num_values = %ld",
		fname, mce->current_num_values));

	return MX_SUCCESSFUL_RESULT;
}

