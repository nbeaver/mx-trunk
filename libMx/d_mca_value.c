/*
 * Name:    d_mca_value.c
 *
 * Purpose: Support for MCA derived values such as ROIs and corrected ROIs.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MCA_VALUE_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_mca.h"
#include "d_mca_value.h"

MX_RECORD_FUNCTION_LIST mxd_mca_value_record_function_list = {
	NULL,
	mxd_mca_value_create_record_structures,
	mxd_mca_value_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_mca_value_analog_input_function_list = {
	mxd_mca_value_read
};

MX_RECORD_FIELD_DEFAULTS mxd_mca_value_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MX_MCA_VALUE_STANDARD_FIELDS,
};

mx_length_type mxd_mca_value_num_record_fields
	= sizeof( mxd_mca_value_record_field_defaults )
	/ sizeof( mxd_mca_value_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mca_value_rfield_def_ptr
		= &mxd_mca_value_record_field_defaults[0];

/* === */

static mx_status_type
mxd_mca_value_get_pointers( MX_ANALOG_INPUT *analog_input,
				MX_MCA_VALUE **mca_value,
				const char *calling_fname )
{
	static const char fname[] = "mxd_mca_value_get_pointers()";

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_ANALOG_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca_value == (MX_MCA_VALUE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA_VALUE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( analog_input->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*mca_value = (MX_MCA_VALUE *) analog_input->record->record_type_struct;

	if ( *mca_value == (MX_MCA_VALUE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA_VALUE pointer for MCA value '%s' is NULL.",
			analog_input->record->name );
	}

	if ( (*mca_value)->mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The 'mca_record' pointer for MCA value record '%s' is NULL.",
			analog_input->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxd_mca_value_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_mca_value_create_record_structures()";

	MX_ANALOG_INPUT *analog_input;
	MX_MCA_VALUE *mca_value;

	/* Allocate memory for the necessary structures. */

	analog_input = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	mca_value = (MX_MCA_VALUE *)
				malloc( sizeof(MX_MCA_VALUE) );

	if ( mca_value == (MX_MCA_VALUE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCA_VALUE structure." );
	}

	/* Now set up the necessary pointers. */

	analog_input->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = analog_input;
	record->record_type_struct = mca_value;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
				&mxd_mca_value_analog_input_function_list;

	/* MCA values are treated as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mca_value_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_mca_value_finish_record_initialization()";

	MX_ANALOG_INPUT *analog_input;
	MX_MCA_VALUE *mca_value;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	analog_input = (MX_ANALOG_INPUT *) record->record_class_struct;

	mx_status = mxd_mca_value_get_pointers( analog_input,
						&mca_value, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strcmp( mca_value->value_name, "roi_integral" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_ROI_INTEGRAL;
	} else
	if ( strcmp( mca_value->value_name, "soft_roi_integral" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_SOFT_ROI_INTEGRAL;
	} else
	if ( strcmp( mca_value->value_name, "real_time" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_REAL_TIME;
	} else
	if ( strcmp( mca_value->value_name, "live_time" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_LIVE_TIME;
	} else
	if ( strcmp( mca_value->value_name, "corrected_roi_integral" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_CORRECTED_ROI_INTEGRAL;
	} else
	if ( strcmp( mca_value->value_name,
			"corrected_soft_roi_integral" ) == 0 )
	{
		mca_value->value_type
			= MXT_MCA_VALUE_CORRECTED_SOFT_ROI_INTEGRAL;
	} else
	if ( strcmp( mca_value->value_name, "input_count_rate" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_INPUT_COUNT_RATE;
	} else
	if ( strcmp( mca_value->value_name, "output_count_rate" ) == 0 ) {
		mca_value->value_type = MXT_MCA_VALUE_OUTPUT_COUNT_RATE;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The MCA value type '%s' for record '%s' is not recognized.",
			mca_value->value_name, record->name );
	}

	mx_status = mx_analog_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mca_value_read( MX_ANALOG_INPUT *analog_input )
{
	static const char fname[] = "mxd_mca_value_read()";

	MX_MCA_VALUE *mca_value;

	mx_length_type roi_number, soft_roi_number;
	uint32_t roi_integral, soft_roi_integral;
	unsigned long ulong_value;
	double corrected_integral, real_time, live_time, multiplier;
	double input_count_rate, output_count_rate;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_mca_value_get_pointers( analog_input,
						&mca_value, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
("%s invoked for record '%s'. value_type = %d, value_name = '%s'.",
		fname, analog_input->record->name,
		mca_value->value_type, mca_value->value_name ));

	/**** Handle the different value types. ****/

	switch( mca_value->value_type ) {

	case MXT_MCA_VALUE_ROI_INTEGRAL:
		num_items = sscanf( mca_value->value_parameters,
					"%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				mca_value->value_parameters,
				analog_input->record->name );
		}

		roi_number = ulong_value;

		mx_status = mx_mca_get_roi_integral( mca_value->mca_record,
						roi_number, &roi_integral );

		analog_input->raw_value.double_value = (double) roi_integral;
		break;

	case MXT_MCA_VALUE_SOFT_ROI_INTEGRAL:
		num_items = sscanf( mca_value->value_parameters,
					"%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				mca_value->value_parameters,
				analog_input->record->name );
		}

		soft_roi_number = ulong_value;

		mx_status = mx_mca_get_soft_roi_integral( mca_value->mca_record,
					soft_roi_number, &soft_roi_integral );

		analog_input->raw_value.double_value
					= (double) soft_roi_integral;
		break;

	case MXT_MCA_VALUE_REAL_TIME:
		mx_status = mx_mca_get_real_time( mca_value->mca_record,
							&real_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = real_time;
		break;

	case MXT_MCA_VALUE_LIVE_TIME:
		mx_status = mx_mca_get_live_time( mca_value->mca_record,
							&live_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = live_time;
		break;

	case MXT_MCA_VALUE_CORRECTED_ROI_INTEGRAL:
		num_items = sscanf( mca_value->value_parameters,
					"%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				mca_value->value_parameters,
				analog_input->record->name );
		}

		roi_number = ulong_value;

		mx_status = mx_mca_get_roi_integral( mca_value->mca_record,
						roi_number, &roi_integral );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_mca_get_real_time( mca_value->mca_record,
							&real_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_mca_get_live_time( mca_value->mca_record,
							&live_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		multiplier = mx_divide_safely( real_time, live_time );

		corrected_integral = mx_multiply_safely( multiplier,
						 (double) roi_integral );

		analog_input->raw_value.double_value = corrected_integral;
		break;

	case MXT_MCA_VALUE_CORRECTED_SOFT_ROI_INTEGRAL:
		num_items = sscanf( mca_value->value_parameters,
					"%lu", &ulong_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				mca_value->value_parameters,
				analog_input->record->name );
		}

		soft_roi_number = ulong_value;

		mx_status = mx_mca_get_soft_roi_integral( mca_value->mca_record,
					soft_roi_number, &soft_roi_integral );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_mca_get_real_time( mca_value->mca_record,
							&real_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_mca_get_live_time( mca_value->mca_record,
							&live_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		corrected_integral = (double) soft_roi_integral;

		multiplier = mx_divide_safely( real_time, live_time );

		corrected_integral = mx_multiply_safely( corrected_integral,
							multiplier );

		analog_input->raw_value.double_value = corrected_integral;
		break;

	case MXT_MCA_VALUE_INPUT_COUNT_RATE:
		mx_status = mx_mca_get_input_count_rate( mca_value->mca_record,
							&input_count_rate );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = input_count_rate;
		break;

	case MXT_MCA_VALUE_OUTPUT_COUNT_RATE:
		mx_status = mx_mca_get_output_count_rate( mca_value->mca_record,
							&output_count_rate );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = output_count_rate;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"The value type '%s' (%d) for MCA value record '%s' is unsupported.",
			mca_value->value_name, mca_value->value_type,
			analog_input->record->name );
		break;
	}

	MX_DEBUG( 2,("%s: value_type = %d, raw_value = %g",
		fname, mca_value->value_type,
		analog_input->raw_value.double_value ));

	return mx_status;
}

