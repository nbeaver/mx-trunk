/*
 * Name:    d_handel_sum.c
 *
 * Purpose: Support for weighted sums of XIA Handel values.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_HANDEL_SUM_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "mx_analog_input.h"
#include "mx_mca.h"
#include "d_handel_mca.h"
#include "d_handel_sum.h"

MX_RECORD_FUNCTION_LIST mxd_handel_sum_record_function_list = {
	NULL,
	mxd_handel_sum_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_handel_sum_analog_input_function_list = {
	mxd_handel_sum_read
};

MX_RECORD_FIELD_DEFAULTS mxd_handel_sum_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MX_HANDEL_SUM_STANDARD_FIELDS,
};

long mxd_handel_sum_num_record_fields
	= sizeof( mxd_handel_sum_record_field_defaults )
	/ sizeof( mxd_handel_sum_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_sum_rfield_def_ptr
		= &mxd_handel_sum_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxd_handel_sum_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_handel_sum_create_record_structures()";

	MX_ANALOG_INPUT *analog_input;
	MX_HANDEL_SUM *handel_sum;

	/* Allocate memory for the necessary structures. */

	analog_input = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	handel_sum = (MX_HANDEL_SUM *)
				malloc( sizeof(MX_HANDEL_SUM) );

	if ( handel_sum == (MX_HANDEL_SUM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HANDEL_SUM structure." );
	}

	/* Now set up the necessary pointers. */

	analog_input->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = analog_input;
	record->record_type_struct = handel_sum;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
				&mxd_handel_sum_analog_input_function_list;

	/* Raw XIA Handel sums are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_sum_read( MX_ANALOG_INPUT *analog_input )
{
	static const char fname[] = "mxd_handel_sum_read()";

	MX_HANDEL_SUM *handel_sum;

	MX_RECORD *handel_record;
	unsigned long num_mcas;
	MX_RECORD **mca_record_array;

	MX_RECORD *mca_record;
	MX_MCA *mca;

	void *field_value_pointer;
	mx_bool_type *enable_array;
	long num_elements;
	unsigned long i, num_enabled_mcas;
	double corrected_roi_value, weighted_roi_value, weighted_sum;
	mx_status_type mx_status;

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed was NULL." );
	}
	if ( analog_input->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	handel_sum = (MX_HANDEL_SUM *)
				analog_input->record->record_type_struct;

	if ( handel_sum == (MX_HANDEL_SUM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_SUM pointer for variable '%s' is NULL.",
			analog_input->record->name );
	}

	handel_record = handel_sum->handel_record;

	if ( handel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The handel_record pointer for record '%s' is NULL.",
			analog_input->record->name );
	}

	mx_status = mxd_handel_mca_get_mca_array( handel_record,
						&num_mcas,
						&mca_record_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the MCA enable flags. */

	mx_status = mx_get_1d_array( handel_sum->mca_enable_record,
					MXFT_BOOL, &num_elements,
					&field_value_pointer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	enable_array = (mx_bool_type *) field_value_pointer;

	if ( num_elements < num_mcas ) {
		num_mcas = num_elements;
	}

	weighted_sum = 0.0;
	num_enabled_mcas = 0;

	for ( i = 0; i < num_mcas; i++ ) {

		mca_record = mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {

			/* Skip this record. */

			continue;
		}

		mca = (MX_MCA *) mca_record->record_class_struct;

		if ( mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MCA pointer for XIA Handel MCA '%s' is NULL.",
				mca_record->name );
		}

		if ( enable_array[i] != FALSE ) {

			mx_status =
			    mxd_handel_mca_get_rate_corrected_roi_integral(
							mca,
							handel_sum->roi_number,
							&corrected_roi_value );

			weighted_roi_value = pow( corrected_roi_value, 1.5 );

			weighted_sum += weighted_roi_value;

			num_enabled_mcas++;
		}
	}

	if ( num_enabled_mcas == 0 ) {
		analog_input->raw_value.double_value = 0;
	} else {
		analog_input->raw_value.double_value
				= weighted_sum / (double) num_enabled_mcas;
	}

	return MX_SUCCESSFUL_RESULT;
}

