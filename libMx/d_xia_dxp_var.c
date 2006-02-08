/*
 * Name:    d_xia_dxp_var.c
 *
 * Purpose: Support for variable values derived from the XIA DXP
 *          multichannel analyzer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_XIA_DXP_INPUT_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_inttypes.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_analog_input.h"
#include "mx_mca.h"
#include "d_xia_dxp_mca.h"
#include "d_xia_dxp_var.h"

MX_RECORD_FUNCTION_LIST mxd_xia_dxp_input_record_function_list = {
	NULL,
	mxd_xia_dxp_input_create_record_structures,
	mx_analog_input_finish_record_initialization
};

MX_ANALOG_INPUT_FUNCTION_LIST mxd_xia_dxp_input_analog_input_function_list = {
	mxd_xia_dxp_input_read
};

MX_RECORD_FIELD_DEFAULTS mxd_xia_dxp_input_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MX_XIA_DXP_INPUT_STANDARD_FIELDS,
};

mx_length_type mxd_xia_dxp_input_num_record_fields
	= sizeof( mxd_xia_dxp_input_record_field_defaults )
	/ sizeof( mxd_xia_dxp_input_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_input_rfield_def_ptr
		= &mxd_xia_dxp_input_record_field_defaults[0];

/********************************************************************/

MX_EXPORT mx_status_type
mxd_xia_dxp_input_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_xia_dxp_input_create_record_structures()";

	MX_ANALOG_INPUT *analog_input;
	MX_XIA_DXP_INPUT *xia_dxp_input;

	/* Allocate memory for the necessary structures. */

	analog_input = (MX_ANALOG_INPUT *) malloc( sizeof(MX_ANALOG_INPUT) );

	if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ANALOG_INPUT structure." );
	}

	xia_dxp_input = (MX_XIA_DXP_INPUT *)
				malloc( sizeof(MX_XIA_DXP_INPUT) );

	if ( xia_dxp_input == (MX_XIA_DXP_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_XIA_DXP_INPUT structure." );
	}

	/* Now set up the necessary pointers. */

	analog_input->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = analog_input;
	record->record_type_struct = xia_dxp_input;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
				&mxd_xia_dxp_input_analog_input_function_list;

	/* Raw XIA DXP input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_input_read( MX_ANALOG_INPUT *analog_input )
{
	static const char fname[] = "mxd_xia_dxp_input_read()";

	MX_RECORD *mca_record;
	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_DXP_INPUT *xia_dxp_input;

	uint32_t parameter_value;
	mx_length_type roi_number;
	uint32_t mca_value;
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

	xia_dxp_input = (MX_XIA_DXP_INPUT *)
				analog_input->record->record_type_struct;

	if ( xia_dxp_input == (MX_XIA_DXP_INPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_XIA_DXP_INPUT pointer for variable '%s' is NULL.",
			analog_input->record->name );
	}

	/* Construct pointers to the XIA DXP MCA data structures. */

	mca_record = xia_dxp_input->mca_record;

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

	xia_dxp_mca = (MX_XIA_DXP_MCA *) mca_record->record_type_struct;

	if ( xia_dxp_mca == (MX_XIA_DXP_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_XIA_DXP_MCA pointer for MCA record '%s' is NULL.",
			mca_record->name );
	}

	MX_DEBUG( 2,("%s invoked for record '%s'. value_type = %d",
		fname, analog_input->record->name, xia_dxp_input->value_type));

	MX_DEBUG( 2,
	("%s: new_data_available = %d, new_statistics_available = %d",
	fname, mca->new_data_available, xia_dxp_mca->new_statistics_available));

	MX_DEBUG( 2,("%s: mca->busy = %d", fname, mca->busy));

	/**** Handle the different value types. ****/

	switch( xia_dxp_input->value_type ) {

	case MXT_XIA_DXP_VAR_PARAMETER:
		if ( xia_dxp_mca->read_parameter == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"XIA interface record '%s' has not been initialized properly.",
				xia_dxp_mca->xia_dxp_record->name );
		}

		mx_status = (xia_dxp_mca->read_parameter)( mca,
					xia_dxp_input->value_parameters,
					&parameter_value,
					MXD_XIA_DXP_INPUT_DEBUG );

		analog_input->raw_value.double_value = (double) parameter_value;
		break;

	case MXT_XIA_DXP_VAR_ROI_INTEGRAL:
		num_items = sscanf( xia_dxp_input->value_parameters,
					"%" SCNu32, &roi_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				xia_dxp_input->value_parameters,
				analog_input->record->name );
		}

		mx_status = mx_mca_get_roi_integral(
						xia_dxp_input->mca_record,
						roi_number, &mca_value );

		analog_input->raw_value.double_value = (double) mca_value;
		break;

	case MXT_XIA_DXP_VAR_RATE_CORRECTED_ROI_INTEGRAL:
		num_items = sscanf( xia_dxp_input->value_parameters,
					"%" SCNu32, &roi_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				xia_dxp_input->value_parameters,
				analog_input->record->name );
		}

		mx_status = mxd_xia_dxp_get_rate_corrected_roi_integral(
							mca, roi_number,
							&corrected_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = corrected_value;
		break;

	case MXT_XIA_DXP_VAR_LIVETIME_CORRECTED_ROI_INTEGRAL:
		num_items = sscanf( xia_dxp_input->value_parameters,
					"%" SCNu32, &roi_number );

		if ( num_items != 1 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Cannot find an ROI number in 'value_parameters' "
			"string '%s' for record '%s'.",
				xia_dxp_input->value_parameters,
				analog_input->record->name );
		}

		mx_status = mxd_xia_dxp_get_livetime_corrected_roi_integral(
							mca, roi_number,
							&corrected_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = corrected_value;
		break;

	case MXT_XIA_DXP_VAR_REALTIME:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = mca->real_time;
		break;

	case MXT_XIA_DXP_VAR_LIVETIME:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value = mca->live_time;
		break;

	case MXT_XIA_DXP_VAR_INPUT_COUNT_RATE:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= xia_dxp_mca->input_count_rate;
		break;

	case MXT_XIA_DXP_VAR_OUTPUT_COUNT_RATE:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= xia_dxp_mca->output_count_rate;
		break;

	case MXT_XIA_DXP_VAR_NUM_FAST_PEAKS:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= (double) xia_dxp_mca->num_fast_peaks;
		break;

	case MXT_XIA_DXP_VAR_NUM_EVENTS:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= (double) xia_dxp_mca->num_events;
		break;

	case MXT_XIA_DXP_VAR_NUM_UNDERFLOWS:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= (double) xia_dxp_mca->num_underflows;
		break;

	case MXT_XIA_DXP_VAR_NUM_OVERFLOWS:
		mx_status = mxd_xia_dxp_read_statistics( mca, xia_dxp_mca,
						MXD_XIA_DXP_INPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		analog_input->raw_value.double_value
				= (double) xia_dxp_mca->num_overflows;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"The value type %d for XIA DXP variable '%s' is unsupported.",
			xia_dxp_input->value_type, analog_input->record->name );
		break;
	}

	MX_DEBUG( 2,("%s: value_type = %d, raw_value = %g",
		fname, xia_dxp_input->value_type,
		analog_input->raw_value.double_value ));

	MX_DEBUG( 2, ("%s: (AFTER) new_statistics_available = %d",
			fname, xia_dxp_mca->new_statistics_available));

	return mx_status;
}

