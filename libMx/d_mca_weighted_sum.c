/*
 * Name:    d_mca_weighted_sum.c
 *
 * Purpose: Support for weighted sums of MCA regions of interest.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MCA_WEIGHTED_SUM_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "mx_analog_input.h"
#include "mx_mca.h"
#include "d_mca_weighted_sum.h"

MX_RECORD_FUNCTION_LIST mxd_mca_weighted_sum_record_function_list = {
	mxd_mca_weighted_sum_initialize_type,
	mxd_mca_weighted_sum_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_mca_weighted_sum_analog_input_function_list = {
	mxd_mca_weighted_sum_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mca_weighted_sum_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MX_MCA_WEIGHTED_SUM_STANDARD_FIELDS,
};

long mxd_mca_weighted_sum_num_record_fields
	= sizeof( mxd_mca_weighted_sum_record_field_defaults )
	/ sizeof( mxd_mca_weighted_sum_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mca_weighted_sum_rfield_def_ptr
		= &mxd_mca_weighted_sum_record_field_defaults[0];

/* === */

static mx_status_type
mxd_mca_weighted_sum_get_pointers( MX_ANALOG_INPUT *analog_input,
				MX_MCA_WEIGHTED_SUM **mca_weighted_sum,
				const char *calling_fname )
{
	static const char fname[] = "mxd_mca_weighted_sum_get_pointers()";

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca_weighted_sum == (MX_MCA_WEIGHTED_SUM **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_MCA_WEIGHTED_SUM pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( analog_input->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*mca_weighted_sum = (MX_MCA_WEIGHTED_SUM *)
				analog_input->record->record_type_struct;

	if ( *mca_weighted_sum == (MX_MCA_WEIGHTED_SUM *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA_WEIGHTED_SUM pointer for MCA value '%s' is NULL.",
			analog_input->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxd_mca_weighted_sum_initialize_type( long record_type )
{
	static const char fname[] = "mxd_mca_weighted_sum_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *mca_record_array_field;
	long num_record_fields;
	long num_mcas_field_index;
	long num_mcas_varargs_cookie;
	mx_status_type mx_status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	record_field_defaults = *record_field_defaults_ptr;

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	num_record_fields = *(driver->num_record_fields);

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_mcas", &num_mcas_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( num_mcas_field_index,
				0, &num_mcas_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"mca_record_array", &mca_record_array_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mca_record_array_field->dimension[0] = num_mcas_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_weighted_sum_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_mca_weighted_sum_create_record_structures()";

	MX_ANALOG_INPUT *analog_input;
	MX_MCA_WEIGHTED_SUM *mca_weighted_sum;

	/* Allocate memory for the necessary structures. */

	analog_input = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	mca_weighted_sum = (MX_MCA_WEIGHTED_SUM *)
				malloc( sizeof(MX_MCA_WEIGHTED_SUM) );

	if ( mca_weighted_sum == (MX_MCA_WEIGHTED_SUM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA_WEIGHTED_SUM structure." );
	}

	/* Now set up the necessary pointers. */

	analog_input->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = analog_input;
	record->record_type_struct = mca_weighted_sum;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
			&mxd_mca_weighted_sum_analog_input_function_list;

	/* MCA values are treated as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_weighted_sum_read( MX_ANALOG_INPUT *analog_input )
{
	static const char fname[] = "mxd_mca_weighted_sum_read()";

	MX_MCA_WEIGHTED_SUM *mca_weighted_sum = NULL;

	unsigned long num_mcas;
	MX_RECORD **mca_record_array;

	MX_RECORD *mca_record;
	MX_MCA *mca;

	void *field_value_pointer;
	mx_bool_type *enable_array;
	long num_elements;
	unsigned long i, num_enabled_mcas;
	unsigned long raw_roi_integral;
	double input_count_rate, output_count_rate, multiplier;
	double rate_corrected_roi_integral;
	double weighted_roi_integral, weighted_sum;
	mx_status_type mx_status;

	mx_status = mxd_mca_weighted_sum_get_pointers( analog_input,
						&mca_weighted_sum, fname );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the MCA enable flags. */

	mx_status = mx_get_1d_array( mca_weighted_sum->mca_enable_record,
					MXFT_BOOL, &num_elements,
					&field_value_pointer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	enable_array = (mx_bool_type *) field_value_pointer;

	num_mcas = mca_weighted_sum->num_mcas;

	if ( num_elements < num_mcas ) {
		num_mcas = num_elements;
	}

	mca_record_array = mca_weighted_sum->mca_record_array;

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
			"MX_MCA pointer for XIA DXP MCA '%s' is NULL.",
				mca_record->name );
		}

		if ( enable_array[i] != FALSE ) {

			mx_status = mx_mca_get_roi_integral(
						mca->record,
						mca_weighted_sum->roi_number,
						&raw_roi_integral );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_mca_get_input_count_rate( mca->record,
							&input_count_rate );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_mca_get_output_count_rate( mca->record,
							&output_count_rate );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			multiplier = mx_divide_safely( input_count_rate,
							output_count_rate );

			rate_corrected_roi_integral =
			    mx_multiply_safely( raw_roi_integral, multiplier );

			weighted_roi_integral =
				pow( rate_corrected_roi_integral, 1.5 );

			weighted_sum += weighted_roi_integral;

			num_enabled_mcas++;

#if MXD_MCA_WEIGHTED_SUM_DEBUG
			MX_DEBUG(-2,
			("%s: MCA '%s' ROI %lu, icr = %f, ocr = %f, "
			"raw ROI int = %lu, rate corrected = %f, weighted = %f",
				fname, mca_record->name,
				mca_weighted_sum->roi_number,
				input_count_rate,
				output_count_rate,
				raw_roi_integral,
				rate_corrected_roi_integral,
				weighted_roi_integral ));
#endif
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

