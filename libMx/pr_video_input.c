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

static mx_status_type
mxp_video_input_get_frame_handler( MX_RECORD *record,
				MX_RECORD_FIELD *record_field,
				MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxp_video_input_get_frame_handler()";

	MX_IMAGE_FRAME *frame;
	MX_RECORD_FIELD *frame_buffer_field;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));

	/* Tell it to read in the frame. */

	mx_status = mx_video_input_get_frame( record, vinput->get_frame,
						&(vinput->frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the frame buffer pointer to point to the buffer in the frame
	 * that we just read in.
	 */

	frame = vinput->frame;

	vinput->frame_buffer = frame->image_data;

	MX_DEBUG( 2,("%s: bytes_per_frame = %ld, frame_buffer = %p",
		fname, vinput->bytes_per_frame, vinput->frame_buffer));

#if 0
	{
		int i;
		unsigned char c;

		for ( i = 0; i < 10; i++ ) {
			c = vinput->frame_buffer[i];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	/* Modify the 'frame_buffer' record field to have the correct number
	 * of array elements.
	 */

	mx_status = mx_find_record_field( record,
			"frame_buffer", &frame_buffer_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frame_buffer_field->num_dimensions != 1 ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The '%s' field for record '%s' has an incorrect "
		"number of dimensions (%ld).  It should be 1.",
			frame_buffer_field->name, record->name,
			frame_buffer_field->num_dimensions );
	}

	MX_DEBUG( 2,
	("%s: OLD frame buffer, num_dimensions = %ld, dimension[0] = %ld",
	 	fname, frame_buffer_field->num_dimensions,
		frame_buffer_field->dimension[0]));

	frame_buffer_field->dimension[0] = vinput->bytes_per_frame;

	MX_DEBUG( 2,
	("%s: NEW frame buffer, num_dimensions = %ld, dimension[0] = %ld",
	 	fname, frame_buffer_field->num_dimensions,
		frame_buffer_field->dimension[0]));

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

mx_status_type
mx_setup_video_input_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_video_input_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_VIN_ABORT:
		case MXLV_VIN_ARM:
		case MXLV_VIN_BUSY:
		case MXLV_VIN_BYTES_PER_FRAME:
		case MXLV_VIN_BYTES_PER_PIXEL:
		case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
		case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
		case MXLV_VIN_FORMAT:
		case MXLV_VIN_FORMAT_NAME:
		case MXLV_VIN_FRAMESIZE:
		case MXLV_VIN_FRAME_BUFFER:
		case MXLV_VIN_GET_FRAME:
		case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
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
	MX_VIDEO_INPUT *vinput;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	vinput = (MX_VIDEO_INPUT *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	MX_DEBUG( 2,("%s: record '%s', field = '%s', operation = %d",
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
		case MXLV_VIN_BYTES_PER_PIXEL:
			mx_status = mx_video_input_get_bytes_per_pixel(
								record, NULL );
			break;
		case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
			mx_status = mx_video_input_get_camera_trigger_polarity(
								record, NULL );
			break;
		case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
			mx_status =
		  mx_video_input_get_external_trigger_polarity( record, NULL );
			break;
		case MXLV_VIN_FORMAT:
		case MXLV_VIN_FORMAT_NAME:
			mx_status = 
				mx_video_input_get_image_format( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_image_get_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
			break;
		case MXLV_VIN_FRAMESIZE:
			mx_status = mx_video_input_get_framesize( record,
								NULL, NULL );
			break;
		case MXLV_VIN_FRAME_BUFFER:
			if ( vinput->frame_buffer == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			"Video input '%s' has not yet taken its first frame.",
					record->name );
			}
			break;
		case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
			mx_status = mx_video_input_get_pixel_clock_frequency(
								record, NULL );
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
		case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
			mx_status = mx_video_input_set_camera_trigger_polarity(
				    record, vinput->camera_trigger_polarity );
			break;
		case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
			mx_status =
			  mx_video_input_set_external_trigger_polarity( record,
				vinput->external_trigger_polarity );
			break;
		case MXLV_VIN_FORMAT_NAME:
			mx_status = mx_image_get_format_type_from_name(
					vinput->image_format_name,
					&(vinput->image_format) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */
		case MXLV_VIN_FORMAT:
			mx_status = mx_video_input_set_image_format( record,
						vinput->image_format );
			break;
		case MXLV_VIN_FRAMESIZE:
			mx_status = mx_video_input_set_framesize( record,
						vinput->framesize[0],
						vinput->framesize[1] );
		case MXLV_VIN_GET_FRAME:
			mx_status = mxp_video_input_get_frame_handler(
					record, record_field, vinput );
			break;
		case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
			mx_status = mx_video_input_set_pixel_clock_frequency(
					record, vinput->pixel_clock_frequency );
			break;
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

