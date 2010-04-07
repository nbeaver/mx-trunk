/*
 * Name:    d_handel_input.c
 *
 * Purpose: Support for variable values derived from XIA Handel-supported
 *          multichannel analyzers.
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

#define MXD_HANDEL_INPUT_DEBUG		TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_mca.h"
#include "d_handel_mca.h"
#include "d_handel_input.h"

MX_RECORD_FUNCTION_LIST mxd_handel_input_record_function_list = {
	NULL,
	mxd_handel_input_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_handel_input_analog_input_function_list = {
	mxd_handel_input_read
};

MX_RECORD_FIELD_DEFAULTS mxd_handel_input_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MX_HANDEL_INPUT_STANDARD_FIELDS,
};

long mxd_handel_input_num_record_fields
	= sizeof( mxd_handel_input_record_field_defaults )
	/ sizeof( mxd_handel_input_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_handel_input_rfield_def_ptr
		= &mxd_handel_input_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxd_handel_input_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_handel_input_create_record_structures()";

	MX_ANALOG_INPUT *analog_input;
	MX_HANDEL_INPUT *handel_input;

	/* Allocate memory for the necessary structures. */

	analog_input = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	handel_input = (MX_HANDEL_INPUT *)
				malloc( sizeof(MX_HANDEL_INPUT) );

	if ( handel_input == (MX_HANDEL_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_HANDEL_INPUT structure." );
	}

	/* Now set up the necessary pointers. */

	analog_input->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = analog_input;
	record->record_type_struct = handel_input;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
				&mxd_handel_input_analog_input_function_list;

	/* Raw XIA Handel input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_handel_input_read( MX_ANALOG_INPUT *analog_input )
{
	static const char fname[] = "mxd_handel_input_read()";

	MX_RECORD *mca_record;
	MX_MCA *mca;
	MX_HANDEL_MCA *handel_mca;
	MX_HANDEL_INPUT *handel_input;

	unsigned long parameter_value;
	unsigned long roi_number, mca_value;
	double corrected_value;
	int num_items;
	mx_status_type mx_status;

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed was NULL." );
	}
	if ( analog_input->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	handel_input = (MX_HANDEL_INPUT *)
				analog_input->record->record_type_struct;

	if ( handel_input == (MX_HANDEL_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_INPUT pointer for variable '%s' is NULL.",
			analog_input->record->name );
	}

	/* Construct pointers to the XIA Handel data structures. */

	mca_record = handel_input->mca_record;

	if ( mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record pointer for variable '%s' is NULL.",
			analog_input->record->name );
	}

	mca = (MX_MCA *) mca_record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MCA pointer for MCA record '%s' is NULL.",
			mca_record->name );
	}

	handel_mca = (MX_HANDEL_MCA *) mca_record->record_type_struct;

	if ( handel_mca == (MX_HANDEL_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_HANDEL_MCA pointer for MCA record '%s' is NULL.",
			mca_record->name );
	}

	MX_DEBUG( 2,("%s invoked for record '%s'. value_type = %ld",
		fname, analog_input->record->name, handel_input->value_type));

	MX_DEBUG( 2,
	("%s: new_data_available = %d, new_statistics_available = %d",
		fname, (int) mca->new_data_available,
		(int) handel_mca->new_statistics_available));

	MX_DEBUG( 2,("%s: mca->busy = %d", fname, (int) mca->busy));

	/**** Handle the different value types. ****/

	switch( handel_input->value_type ) {

	case MXT_HANDEL_INPUT_PARAMETER:
		if ( handel_mca->read_parameter == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Handel interface record '%s' has not been initialized properly.",
				handel_mca->handel_record->name );
		}

		mx_status = (handel_mca->read_parameter)( mca,
					handel_input->value_parameters,
					&parameter_value );

		analog_input->raw_value.double_value = (double) parameter_value;
		break;

	case MXT_HANDEL_INPUT_ROI_INTEGRAL:
		num_items = sscanf( handel_input->value_parameters,
					"%lu", &roi_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				handel_input->value_parameters,
				analog_input->record->name );
		}

		mx_status = mx_mca_get_roi_integral(
						handel_input->mca_record,
						roi_number, &mca_value );

		analog_input->raw_value.double_value = (double) mca_value;
		break;

	case MXT_HANDEL_INPUT_RATE_CORRECTED_ROI_INTEGRAL:
		num_items = sscanf( handel_input->value_parameters,
					"%lu", &roi_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				handel_input->value_parameters,
				analog_input->record->name );
		}

		mx_status = mxd_handel_mca_get_rate_corrected_roi_integral(
							mca, roi_number,
							&corrected_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = corrected_value;
		break;

	case MXT_HANDEL_INPUT_LIVETIME_CORRECTED_ROI_INTEGRAL:
		num_items = sscanf( handel_input->value_parameters,
					"%lu", &roi_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				handel_input->value_parameters,
				analog_input->record->name );
		}

		mx_status = mxd_handel_mca_get_livetime_corrected_roi_integral(
							mca, roi_number,
							&corrected_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = corrected_value;
		break;

	case MXT_HANDEL_INPUT_REALTIME:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = mca->real_time;
		break;

	case MXT_HANDEL_INPUT_LIVETIME:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = mca->live_time;
		break;

	case MXT_HANDEL_INPUT_INPUT_COUNT_RATE:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= handel_mca->input_count_rate;
		break;

	case MXT_HANDEL_INPUT_OUTPUT_COUNT_RATE:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= handel_mca->output_count_rate;
		break;

	case MXT_HANDEL_INPUT_NUM_TRIGGERS:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= (double) handel_mca->num_triggers;
		break;

	case MXT_HANDEL_INPUT_NUM_EVENTS:
		mx_status = mxd_handel_mca_read_statistics( mca, handel_mca );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= (double) handel_mca->num_events;
		break;

	case MXT_HANDEL_INPUT_ACQUISITION_VALUE:
		mx_status = ( handel_mca->get_acquisition_values )( mca,
				handel_input->value_parameters,
				&(analog_input->raw_value.double_value) );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	    "The value type %ld for Handel MCA variable '%s' is unsupported.",
			handel_input->value_type, analog_input->record->name );
	}

	MX_DEBUG( 2,("%s: value_type = %ld, raw_value = %g",
		fname, handel_input->value_type,
		analog_input->raw_value.double_value ));

	MX_DEBUG( 2, ("%s: (AFTER) new_statistics_available = %d",
			fname, (int) handel_mca->new_statistics_available));

	return mx_status;
}

