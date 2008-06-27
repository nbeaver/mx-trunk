/*
 * Name:    pr_waveform_output.c
 *
 * Purpose: Functions used to process MX waveform output record events.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_waveform_output.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_waveform_output_process_functions( MX_RECORD *record )
{
	static const char fname[] =
			"mx_setup_waveform_output_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_WVO_ARM:
		case MXLV_WVO_BUSY:
		case MXLV_WVO_CHANNEL_DATA:
		case MXLV_WVO_CHANNEL_INDEX:
		case MXLV_WVO_FREQUENCY:
		case MXLV_WVO_STOP:
		case MXLV_WVO_TRIGGER:
		case MXLV_WVO_TRIGGER_MODE:
			record_field->process_function
				    = mx_waveform_output_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_waveform_output_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_waveform_output_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_WAVEFORM_OUTPUT *wvout;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	wvout = (MX_WAVEFORM_OUTPUT *) record->record_class_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_WVO_BUSY:
			mx_status = mx_waveform_output_is_busy( record, NULL );
			break;
		case MXLV_WVO_CHANNEL_DATA:
			mx_status = mx_waveform_output_read_channel( record,
							wvout->channel_index,
							NULL, NULL );
			break;
		case MXLV_WVO_FREQUENCY:
			mx_status = mx_waveform_output_get_frequency(
							record, NULL );
			break;
		case MXLV_WVO_TRIGGER_MODE:
			mx_status = mx_waveform_output_get_trigger_mode(
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
		case MXLV_WVO_ARM:
			mx_status = mx_waveform_output_arm( record );
			break;
		case MXLV_WVO_CHANNEL_DATA:
			mx_status = mx_waveform_output_write_channel( record,
						wvout->channel_index,
						wvout->current_num_steps,
						wvout->channel_data );
			break;
		case MXLV_WVO_CHANNEL_INDEX:
			if (wvout->channel_index >= wvout->maximum_num_channels)
			{
			    mx_status = mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The requested channel index (%ld) "
				"for record '%s' is larger than the maximum "
				"allowed channel number (%ld).",
					wvout->channel_index, record->name,
					wvout->maximum_num_channels - 1 );
			} else {
			    wvout->channel_data =
				    (wvout->data_array)[wvout->channel_index];
			}
			break;
		case MXLV_WVO_FREQUENCY:
			mx_status = mx_waveform_output_set_frequency( record,
							wvout->frequency );
			break;
		case MXLV_WVO_STOP:
			mx_status = mx_waveform_output_stop( record );
			break;
		case MXLV_WVO_TRIGGER:
			mx_status = mx_waveform_output_trigger( record );
			break;
		case MXLV_WVO_TRIGGER_MODE:
			mx_status = mx_waveform_output_set_trigger_mode(
						record, wvout->trigger_mode );
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

