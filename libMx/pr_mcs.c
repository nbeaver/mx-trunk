/*
 * Name:    pr_mcs.c
 *
 * Purpose: Functions used to process MX MCS record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2004, 2006, 2009, 2012, 2015, 2018-2019
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
#include "mx_mcs.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_mcs_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_mcs_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_MCS_ARM:
		case MXLV_MCS_BUSY:
		case MXLV_MCS_CLEAR:
		case MXLV_MCS_CLEAR_DEADBAND:
		case MXLV_MCS_COUNTING_MODE:
		case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		case MXLV_MCS_CURRENT_NUM_SCALERS:
		case MXLV_MCS_DARK_CURRENT:
		case MXLV_MCS_DARK_CURRENT_ARRAY:
		case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
		case MXLV_MCS_EXTERNAL_PRESCALE:
		case MXLV_MCS_LAST_MEASUREMENT_NUMBER:
		case MXLV_MCS_MEASUREMENT_COUNTS:
		case MXLV_MCS_MEASUREMENT_DATA:
		case MXLV_MCS_MEASUREMENT_INDEX:
		case MXLV_MCS_MEASUREMENT_TIME:
		case MXLV_MCS_SCALER_DATA:
		case MXLV_MCS_SCALER_INDEX:
		case MXLV_MCS_SCALER_MEASUREMENT:
		case MXLV_MCS_START:
		case MXLV_MCS_STATUS:
		case MXLV_MCS_STOP:
		case MXLV_MCS_TIMER_DATA:
		case MXLV_MCS_TOTAL_NUM_MEASUREMENTS:
		case MXLV_MCS_TRIGGER:
		case MXLV_MCS_TRIGGER_MODE:
			record_field->process_function
					    = mx_mcs_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_mcs_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_mcs_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_MCS *mcs;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_MCS_BUSY:
			mx_status = mx_mcs_is_busy( record, NULL );
			break;
		case MXLV_MCS_STATUS:
			mx_status = mx_mcs_get_status( record, NULL );
			break;
		case MXLV_MCS_COUNTING_MODE:
			mx_status = mx_mcs_get_counting_mode( record, NULL );
			break;
		case MXLV_MCS_TRIGGER_MODE:
			mx_status = mx_mcs_get_trigger_mode( record, NULL );
			break;
		case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
			mx_status = mx_mcs_get_external_next_measurement(
								record, NULL );
			break;
		case MXLV_MCS_EXTERNAL_PRESCALE:
			mx_status = mx_mcs_get_external_prescale( record, NULL );
			break;
		case MXLV_MCS_MEASUREMENT_TIME:
			mx_status = mx_mcs_get_measurement_time( record, NULL );
			break;
		case MXLV_MCS_MEASUREMENT_COUNTS:
			mx_status = mx_mcs_get_measurement_counts( record, NULL );
			break;
		case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
			mx_status = mx_mcs_get_num_measurements( record, NULL );
			break;
		case MXLV_MCS_DARK_CURRENT:
			mx_status = mx_mcs_get_dark_current( record,
						mcs->scaler_index, NULL );

			mcs->dark_current = 
				mcs->dark_current_array[mcs->scaler_index];
			break;
		case MXLV_MCS_DARK_CURRENT_ARRAY:
			mx_status = mx_mcs_get_dark_current_array( record,
								-1, NULL );
			break;
		case MXLV_MCS_SCALER_DATA:
			mx_status = mx_mcs_read_scaler( record,
					mcs->scaler_index, NULL, NULL );
			break;
		case MXLV_MCS_MEASUREMENT_DATA:
			mx_status = mx_mcs_read_measurement( record,
					mcs->measurement_index, NULL, NULL );
			break;
		case MXLV_MCS_SCALER_MEASUREMENT:
			mx_status = mx_mcs_read_scaler_measurement( record,
					mcs->scaler_index,
					mcs->measurement_index,
					NULL );
			break;
		case MXLV_MCS_TIMER_DATA:
			mx_status = mx_mcs_read_timer( record, NULL, NULL );
			break;
		case MXLV_MCS_LAST_MEASUREMENT_NUMBER:
			mx_status = mx_mcs_get_last_measurement_number(
								record, NULL );
			break;
		case MXLV_MCS_TOTAL_NUM_MEASUREMENTS:
			mx_status = mx_mcs_get_total_num_measurements(
								record, NULL );
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
		case MXLV_MCS_ARM:
			mx_status = mx_mcs_arm( record );
			break;
		case MXLV_MCS_TRIGGER:
			mx_status = mx_mcs_trigger( record );
			break;
		case MXLV_MCS_START:
			mx_status = mx_mcs_start( record );
			break;
		case MXLV_MCS_STOP:
			mx_status = mx_mcs_stop( record );
			break;
		case MXLV_MCS_CLEAR:
			mx_status = mx_mcs_clear( record );
			break;
		case MXLV_MCS_COUNTING_MODE:
			mx_status =
			  mx_mcs_set_counting_mode( record, mcs->counting_mode);
			break;
		case MXLV_MCS_TRIGGER_MODE:
			mx_status =
			  mx_mcs_set_trigger_mode( record, mcs->trigger_mode );
			break;
		case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
			mx_status = mx_mcs_set_external_next_measurement(
				record, mcs->external_next_measurement );
			break;
		case MXLV_MCS_EXTERNAL_PRESCALE:
			mx_status = mx_mcs_set_external_prescale( record,
						mcs->external_prescale );
			break;
		case MXLV_MCS_MEASUREMENT_TIME:
			mx_status = mx_mcs_set_measurement_time( record,
						mcs->measurement_time );
			break;
		case MXLV_MCS_MEASUREMENT_COUNTS:
			mx_status = mx_mcs_set_measurement_counts( record,
						mcs->measurement_counts );
			break;
		case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
			mx_status = mx_mcs_set_num_measurements( record,
						mcs->current_num_measurements );
			break;
		case MXLV_MCS_SCALER_INDEX:
			break;
		case MXLV_MCS_MEASUREMENT_INDEX:
			break;
		case MXLV_MCS_DARK_CURRENT:
			mx_status = mx_mcs_set_dark_current( record,
						mcs->scaler_index,
						mcs->dark_current );
			break;
		case MXLV_MCS_CLEAR_DEADBAND:
			mx_status = mx_mcs_set_parameter( record,
						MXLV_MCS_CLEAR_DEADBAND );
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

