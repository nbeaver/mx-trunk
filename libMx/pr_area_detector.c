/*
 * Name:    pr_area_detector.c
 *
 * Purpose: Functions used to process MX area detector record events.
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
#include "mx_area_detector.h"
#include "mx_socket.h"

#include "mx_process.h"
#include "pr_handlers.h"

static mx_status_type
mxp_area_detector_get_frame_handler( MX_RECORD *record,
				MX_RECORD_FIELD *record_field,
				MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxp_area_detector_get_frame_handler()";

	MX_IMAGE_FRAME *frame;
	MX_RECORD_FIELD *frame_buffer_field;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));

	/* Tell it to read in the frame. */

	mx_status = mx_area_detector_get_frame( record, ad->get_frame,
						&(ad->frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the frame buffer pointer to point to the buffer in the frame
	 * that we just read in.
	 */

	frame = ad->frame;

	ad->frame_buffer = frame->image_data;

	MX_DEBUG(-2,("%s: bytes_per_frame = %ld, frame_buffer = %p",
		fname, ad->bytes_per_frame, ad->frame_buffer));

#if 1
	{
		int i;
		unsigned char c;

		for ( i = 0; i < 10; i++ ) {
			c = ad->frame_buffer[i];

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

	MX_DEBUG(-2,
	("%s: OLD frame buffer, num_dimensions = %ld, dimension[0] = %ld",
	 	fname, frame_buffer_field->num_dimensions,
		frame_buffer_field->dimension[0]));

	frame_buffer_field->dimension[0] = ad->bytes_per_frame;

	MX_DEBUG(-2,
	("%s: NEW frame buffer, num_dimensions = %ld, dimension[0] = %ld",
	 	fname, frame_buffer_field->num_dimensions,
		frame_buffer_field->dimension[0]));

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

mx_status_type
mx_setup_area_detector_process_functions( MX_RECORD *record )
{
	static const char fname[] = "mx_setup_area_detector_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(-2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AD_ABORT:
		case MXLV_AD_ARM:
		case MXLV_AD_BUSY:
		case MXLV_AD_BYTES_PER_FRAME:
		case MXLV_AD_FORMAT:
		case MXLV_AD_FORMAT_NAME:
		case MXLV_AD_FRAMESIZE:
		case MXLV_AD_FRAME_BUFFER:
		case MXLV_AD_GET_FRAME:
		case MXLV_AD_STATUS:
		case MXLV_AD_STOP:
		case MXLV_AD_TRIGGER:
			record_field->process_function
					= mx_area_detector_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

mx_status_type
mx_area_detector_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mx_area_detector_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ad = (MX_AREA_DETECTOR *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	MX_DEBUG(-2,("%s: record '%s', field = '%s', operation = %d",
		fname, record->name, record_field->name, operation));

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AD_BUSY:
			mx_status = mx_area_detector_is_busy( record, NULL );
			break;
		case MXLV_AD_BYTES_PER_FRAME:
			mx_status = mx_area_detector_get_bytes_per_frame(
								record, NULL );
			break;
		case MXLV_AD_FORMAT:
		case MXLV_AD_FORMAT_NAME:
			mx_status = 
			   mx_area_detector_get_image_format( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_get_framesize( record,
								NULL, NULL );
			break;
		case MXLV_AD_FRAME_BUFFER:
			if ( ad->frame_buffer == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			"area detector '%s' has not yet taken its first frame.",
					record->name );
			}
			break;
		case MXLV_AD_STATUS:
			mx_status = mx_area_detector_get_status( record,
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
		case MXLV_AD_ABORT:
			mx_status = mx_area_detector_abort( record );
			break;
		case MXLV_AD_ARM:
			mx_status = mx_area_detector_arm( record );
			break;
		case MXLV_AD_FORMAT_NAME:
			mx_status = mx_get_image_format_type_from_name(
					ad->image_format_name,
					&(ad->image_format) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */
		case MXLV_AD_FORMAT:
			mx_status = mx_area_detector_set_image_format( record,
						ad->image_format );
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_set_framesize( record,
						ad->framesize[0],
						ad->framesize[1] );
		case MXLV_AD_GET_FRAME:
			mx_status = mxp_area_detector_get_frame_handler(
					record, record_field, ad );
			break;
		case MXLV_AD_STOP:
			mx_status = mx_area_detector_stop( record );
			break;
		case MXLV_AD_TRIGGER:
			mx_status = mx_area_detector_trigger( record );
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

