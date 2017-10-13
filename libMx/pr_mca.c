/*
 * Name:    pr_mca.c
 *
 * Purpose: Functions used to process MX MCA record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2004, 2006, 2010, 2012, 2015, 2017
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_mca.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_mca_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_mca_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_MCA_ARM:
		case MXLV_MCA_BUSY:
		case MXLV_MCA_BUSY_START_INTERVAL:
		case MXLV_MCA_CHANNEL_ARRAY:
		case MXLV_MCA_CHANNEL_VALUE:
		case MXLV_MCA_CLEAR:
		case MXLV_MCA_CURRENT_NUM_CHANNELS:
		case MXLV_MCA_ENERGY_OFFSET:
		case MXLV_MCA_ENERGY_SCALE:
		case MXLV_MCA_INPUT_COUNT_RATE:
		case MXLV_MCA_LAST_START_TIME:
		case MXLV_MCA_LIVE_TIME:
		case MXLV_MCA_OUTPUT_COUNT_RATE:
		case MXLV_MCA_PRESET_COUNT:
		case MXLV_MCA_PRESET_LIVE_TIME:
		case MXLV_MCA_PRESET_REAL_TIME:
		case MXLV_MCA_PRESET_TYPE:
		case MXLV_MCA_REAL_TIME:
		case MXLV_MCA_ROI:
		case MXLV_MCA_ROI_ARRAY:
		case MXLV_MCA_ROI_INTEGRAL:
		case MXLV_MCA_ROI_INTEGRAL_ARRAY:
		case MXLV_MCA_SOFT_ROI:
		case MXLV_MCA_SOFT_ROI_INTEGRAL:
		case MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY:
		case MXLV_MCA_START:
		case MXLV_MCA_START_WITH_PRESET:
		case MXLV_MCA_STOP:
		case MXLV_MCA_TRIGGER:
		case MXLV_MCA_TRIGGER_MODE:
			record_field->process_function
					    = mx_mca_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_mca_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_mca_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MCA *mca;
	unsigned long ulong_value;
	double double_value;
	double preset_value;
	long preset_type;
	mx_bool_type bool_value;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mca = (MX_MCA *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_MCA_BUSY:
			mx_status = mx_mca_is_busy( record, &bool_value );
			break;
		case MXLV_MCA_TRIGGER_MODE:
			mx_status = mx_mca_get_trigger_mode( record,
							&ulong_value );
			break;
		case MXLV_MCA_PRESET_TYPE:
			mx_status = mx_mca_get_preset_type( record, NULL );
			break;
		case MXLV_MCA_CHANNEL_ARRAY:
			mx_status = mx_mca_read( record, NULL, NULL );
			break;
		case MXLV_MCA_CURRENT_NUM_CHANNELS:
			mx_status = mx_mca_get_num_channels(
						record, &ulong_value );
			break;
		case MXLV_MCA_LAST_START_TIME:
			mca->last_start_time =
		    mx_convert_clock_ticks_to_seconds( mca->last_start_tick );
			break;
		case MXLV_MCA_ROI_ARRAY:
			mx_status = mx_mca_get_roi_array( record,
						mca->current_num_rois, NULL );
			break;
		case MXLV_MCA_ROI_INTEGRAL_ARRAY:
			mx_status = mx_mca_get_roi_integral_array( record,
						mca->current_num_rois, NULL );
			break;
		case MXLV_MCA_ROI:
			mx_status = mx_mca_get_roi(record, mca->roi_number, NULL);
			break;
		case MXLV_MCA_ROI_INTEGRAL:
			mx_status = mx_mca_get_roi_integral( record,
						mca->roi_number, NULL );
			break;
		case MXLV_MCA_CHANNEL_VALUE:
			mx_status = mx_mca_get_channel( record,
						mca->channel_number,
						&ulong_value );
			break;
		case MXLV_MCA_REAL_TIME:
			mx_status = mx_mca_get_real_time( record,
							&double_value );
			break;
		case MXLV_MCA_LIVE_TIME:
			mx_status = mx_mca_get_live_time( record,
							&double_value );
			break;
		case MXLV_MCA_PRESET_REAL_TIME:
			mx_status = mx_mca_get_preset_real_time( record, NULL );
			break;
		case MXLV_MCA_PRESET_LIVE_TIME:
			mx_status = mx_mca_get_preset_live_time( record, NULL );
			break;
		case MXLV_MCA_PRESET_COUNT:
			mx_status = mx_mca_get_preset_count( record, NULL );
			break;
		case MXLV_MCA_SOFT_ROI:
			mx_status = mx_mca_get_soft_roi(record,
						mca->soft_roi_number, NULL );
			break;
		case MXLV_MCA_SOFT_ROI_INTEGRAL:
			mx_status = mx_mca_get_soft_roi_integral( record,
					mca->soft_roi_number, &ulong_value );

			MX_DEBUG(-2,
				("%s: MXLV_MCA_SOFT_ROI_INTEGRAL value = %lu",
				 fname, ulong_value));
			break;
		case MXLV_MCA_SOFT_ROI_INTEGRAL_ARRAY:
			mx_status = mx_mca_get_soft_roi_integral_array( record,
						mca->num_soft_rois, NULL );
			break;
		case MXLV_MCA_ENERGY_SCALE:
			mx_status = mx_mca_get_energy_scale( record, NULL );
			break;
		case MXLV_MCA_ENERGY_OFFSET:
			mx_status = mx_mca_get_energy_offset( record, NULL );
			break;
		case MXLV_MCA_INPUT_COUNT_RATE:
			mx_status = mx_mca_get_input_count_rate( record, NULL );
			break;
		case MXLV_MCA_OUTPUT_COUNT_RATE:
			mx_status = mx_mca_get_output_count_rate( record, NULL );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_MCA_ARM:
			mx_status = mx_mca_arm( record );
			break;
		case MXLV_MCA_TRIGGER:
			mx_status = mx_mca_trigger( record );
			break;
		case MXLV_MCA_TRIGGER_MODE:
			mx_status = mx_mca_set_trigger_mode( record,
							mca->trigger_mode );
			break;
		case MXLV_MCA_PRESET_TYPE:
			mx_status = mx_mca_set_preset_type( record,
							mca->preset_type );
			break;
		case MXLV_MCA_CURRENT_NUM_CHANNELS:
			mx_status = mx_mca_set_num_channels( record,
						mca->current_num_channels );
			break;
		case MXLV_MCA_START:
			mx_status = mx_mca_start( record );
			break;
		case MXLV_MCA_STOP:
			mx_status = mx_mca_stop( record );
			break;
		case MXLV_MCA_CLEAR:
			mx_status = mx_mca_clear( record );
			break;
		case MXLV_MCA_START_WITH_PRESET:
			preset_type = mx_round( mca->start_with_preset[0] );

			preset_value = mca->start_with_preset[1];

			MX_DEBUG( 2,
		("%s: START_WITH_PRESET: mca '%s', type = %ld, value = %g",
			fname, record->name, preset_type, preset_value));

			mx_status = mx_mca_start_with_preset( record,
							preset_type,
							preset_value );
			break;
		case MXLV_MCA_BUSY_START_INTERVAL:
			mx_status = mx_mca_set_busy_start_interval( record,
						mca->busy_start_interval );
			break;
		case MXLV_MCA_ROI_ARRAY:
			mx_status = mx_mca_set_roi_array( record,
						mca->current_num_rois,
						mca->roi_array );
			break;
		case MXLV_MCA_ROI:
			mx_status = mx_mca_set_roi( record,
						mca->roi_number,
						mca->roi );
			break;
		case MXLV_MCA_PRESET_REAL_TIME:
			mx_status = mx_mca_set_preset_real_time( record,
							mca->preset_real_time );
			break;
		case MXLV_MCA_PRESET_LIVE_TIME:
			mx_status = mx_mca_set_preset_live_time( record,
							mca->preset_live_time );
			break;
		case MXLV_MCA_PRESET_COUNT:
			mx_status = mx_mca_set_preset_count( record,
							mca->preset_count );
			break;
		case MXLV_MCA_SOFT_ROI:
			mx_status = mx_mca_set_soft_roi(record,
						mca->soft_roi_number,
						mca->soft_roi );
			break;
		case MXLV_MCA_ENERGY_SCALE:
			mx_status = mx_mca_set_energy_scale( record,
							mca->energy_scale );
			break;
		case MXLV_MCA_ENERGY_OFFSET:
			mx_status = mx_mca_set_energy_offset( record,
							mca->energy_offset );
			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

