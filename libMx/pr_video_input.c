/*
 * Name:    pr_video_input.c
 *
 * Purpose: Functions used to process MX video input record events.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
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
#include "mx_image.h"
#include "mx_video_input.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

mx_status_type
mx_setup_video_input_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_video_input_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(-2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_VIN_ABORT:
		case MXLV_VIN_ARM:
		case MXLV_VIN_BUSY:
		case MXLV_VIN_BYTES_PER_FRAME:
		case MXLV_VIN_FORMAT:
		case MXLV_VIN_FRAMESIZE:
		case MXLV_VIN_STATUS:
		case MXLV_VIN_STOP:
		case MXLV_VIN_TRIGGER:
			record_field->process_function
					= mx_video_input_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_video_input_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_video_input_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_VIDEO_INPUT *video_input;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	video_input = (MX_VIDEO_INPUT *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	MX_DEBUG(-2,("%s: record '%s', field = '%s', operation = %d",
		fname, record->name, record_field->name, operation));

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_VIN_BUSY:
			mx_status = mx_video_input_is_busy( record, NULL );
			break;
		case MXLV_VIN_BYTES_PER_FRAME:
			mx_status = mx_video_input_get_bytes_per_frame(
								record, NULL );
			break;
		case MXLV_VIN_FORMAT:
			mx_status = 
				mx_video_input_get_image_format( record, NULL );
			break;
		case MXLV_VIN_FRAMESIZE:
			mx_status = mx_video_input_get_framesize( record,
								NULL, NULL );
			break;
		case MXLV_VIN_STATUS:
			mx_status = mx_video_input_get_status( record,
								NULL, NULL );
			break;
		default:
			MX_DEBUG(-1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_VIN_ABORT:
			mx_status = mx_video_input_abort( record );
			break;
		case MXLV_VIN_ARM:
			mx_status = mx_video_input_arm( record );
			break;
		case MXLV_VIN_FORMAT:
			mx_status = mx_video_input_set_image_format( record,
						video_input->image_format );
			break;
		case MXLV_VIN_FRAMESIZE:
			mx_status = mx_video_input_set_framesize( record,
						video_input->framesize[0],
						video_input->framesize[1] );
		case MXLV_VIN_STOP:
			mx_status = mx_video_input_stop( record );
			break;
		case MXLV_VIN_TRIGGER:
			mx_status = mx_video_input_trigger( record );
			break;
		default:
			MX_DEBUG(-1,(
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

