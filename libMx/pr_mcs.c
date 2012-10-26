/*
 * Name:    pr_mcs.c
 *
 * Purpose: Functions used to process MX MCS record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2004, 2006, 2009, 2012 Illinois Institute of Technology
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
		case MXLV_MCS_START:
		case MXLV_MCS_STOP:
		case MXLV_MCS_CLEAR:
		case MXLV_MCS_BUSY:
		case MXLV_MCS_MODE:
		case MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE:
		case MXLV_MCS_EXTERNAL_PRESCALE:
		case MXLV_MCS_MEASUREMENT_TIME:
		case MXLV_MCS_MEASUREMENT_COUNTS:
		case MXLV_MCS_CURRENT_NUM_SCALERS:
		case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		case MXLV_MCS_SCALER_INDEX:
		case MXLV_MCS_MEASUREMENT_INDEX:
		case MXLV_MCS_DARK_CURRENT:
		case MXLV_MCS_SCALER_DATA:
		case MXLV_MCS_MEASUREMENT_DATA:
		case MXLV_MCS_TIMER_DATA:
		case MXLV_MCS_CLEAR_DEADBAND:
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
	mx_status_type status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	mcs = (MX_MCS *) (record->record_class_struct);

	status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_MCS_BUSY:
			status = mx_mcs_is_busy( record, NULL );
			break;
		case MXLV_MCS_MODE:
			status = mx_mcs_get_mode( record, NULL );
			break;
		case MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE:
			status = mx_mcs_get_external_channel_advance(
								record, NULL );
			break;
		case MXLV_MCS_EXTERNAL_PRESCALE:
			status = mx_mcs_get_external_prescale( record, NULL );
			break;
		case MXLV_MCS_MEASUREMENT_TIME:
			status = mx_mcs_get_measurement_time( record, NULL );
			break;
		case MXLV_MCS_MEASUREMENT_COUNTS:
			status = mx_mcs_get_measurement_counts( record, NULL );
			break;
		case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
			status = mx_mcs_get_num_measurements( record, NULL );
			break;
		case MXLV_MCS_DARK_CURRENT:
			status = mx_mcs_get_dark_current( record,
						mcs->scaler_index, NULL );

			mcs->dark_current = 
				mcs->dark_current_array[mcs->scaler_index];
			break;
		case MXLV_MCS_SCALER_DATA:
			status = mx_mcs_read_scaler( record,
					mcs->scaler_index, NULL, NULL );
			break;
		case MXLV_MCS_MEASUREMENT_DATA:
			status = mx_mcs_read_measurement( record,
					mcs->measurement_index, NULL, NULL );
			break;
		case MXLV_MCS_TIMER_DATA:
			status = mx_mcs_read_timer( record, NULL, NULL );
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
		case MXLV_MCS_START:
			status = mx_mcs_start( record );
			break;
		case MXLV_MCS_STOP:
			status = mx_mcs_stop( record );
			break;
		case MXLV_MCS_CLEAR:
			status = mx_mcs_clear( record );
			break;
		case MXLV_MCS_MODE:
			status = mx_mcs_set_mode( record, mcs->mode );
			break;
		case MXLV_MCS_EXTERNAL_CHANNEL_ADVANCE:
			status = mx_mcs_set_external_channel_advance( record,
						mcs->external_channel_advance );
			break;
		case MXLV_MCS_EXTERNAL_PRESCALE:
			status = mx_mcs_set_external_prescale( record,
						mcs->external_prescale );
			break;
		case MXLV_MCS_MEASUREMENT_TIME:
			status = mx_mcs_set_measurement_time( record,
						mcs->measurement_time );
			break;
		case MXLV_MCS_MEASUREMENT_COUNTS:
			status = mx_mcs_set_measurement_counts( record,
						mcs->measurement_counts );
			break;
		case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
			status = mx_mcs_set_num_measurements( record,
						mcs->current_num_measurements );
			break;
		case MXLV_MCS_SCALER_INDEX:
			break;
		case MXLV_MCS_MEASUREMENT_INDEX:
			break;
		case MXLV_MCS_DARK_CURRENT:
			status = mx_mcs_set_dark_current( record,
						mcs->scaler_index,
						mcs->dark_current );
			break;
		case MXLV_MCS_CLEAR_DEADBAND:
			status = mx_mcs_set_parameter( record,
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

	return status;
}

