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

#define PR_AREA_DETECTOR_DEBUG	FALSE

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
mxp_area_detector_readout_frame_handler( MX_RECORD *record,
					MX_RECORD_FIELD *record_field,
					MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxp_area_detector_readout_frame_handler()";

	MX_IMAGE_FRAME *image_frame;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));

	mx_status = mx_area_detector_setup_frame( record, &(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_readout_frame( record, ad->readout_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the image_frame buffer pointer to point to the buffer in
	 * the frame that we just read in.
	 */

	image_frame = ad->image_frame;

	ad->image_frame_buffer = image_frame->image_data;

	MX_DEBUG( 2,("%s: bytes_per_frame = %ld, frame_buffer = %p",
		fname, ad->bytes_per_frame, ad->image_frame_buffer));

#if 0
	{
		int i;
		unsigned char c;

		for ( i = 0; i < 10; i++ ) {
			c = ad->image_frame_buffer[i];

			MX_DEBUG(-2,
			("%s: image_frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	/* Modify the 'image_frame_buffer' record field to have
	 * the correct number of array elements.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( record,
				"image_frame_buffer", ad->bytes_per_frame );

	return mx_status;
}

/*----*/

static mx_status_type
mxp_area_detector_get_roi_frame_handler( MX_RECORD *record,
				MX_RECORD_FIELD *record_field,
				MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxp_area_detector_get_roi_frame_handler()";

	MX_IMAGE_FRAME *roi_frame;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s', field = '%s'",
			fname, record->name, record_field->name));

	/* Check to see if a full image frame has been read from 
	 * the camera hardware yet.
	 */

	if ( ad->image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No image frame has been read from the camera hardware yet "
		"for area detector '%s', so we are not yet ready to return "
		"an ROI frame.", record->name );
	}

	/* Get the ROI frame from the full image frame. */

	mx_status = mx_area_detector_get_roi_frame( record, ad->image_frame,
						ad->get_roi_frame,
						&(ad->roi_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the ROI frame buffer pointer to point to the buffer in
	 * the ROI frame that we just copied in.
	 */

	roi_frame = ad->roi_frame;

	ad->roi_frame_buffer = roi_frame->image_data;

	MX_DEBUG( 2,("%s: roi_bytes_per_frame = %ld, roi_frame_buffer = %p",
		fname, ad->roi_bytes_per_frame, ad->roi_frame_buffer));

#if 0
	{
		int i;
		unsigned char c;

		for ( i = 0; i < 10; i++ ) {
			c = ad->roi_frame_buffer[i];

			MX_DEBUG(-2,
			("%s: roi_frame_buffer[%d] = %u", fname, i, c));
		}
	}
#endif

	/* Modify the 'roi_frame_buffer' record field to have the
	 * correct number of array elements.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( record,
						"roi_frame_buffer",
						ad->roi_bytes_per_frame );
	return mx_status;
}

/*----*/

mx_status_type
mx_setup_area_detector_process_functions( MX_RECORD *record )
{
	static const char fname[] =
			"mx_setup_area_detector_process_functions()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG( 2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_AD_ABORT:
		case MXLV_AD_ARM:
		case MXLV_AD_BINSIZE:
		case MXLV_AD_BYTES_PER_FRAME:
		case MXLV_AD_BYTES_PER_PIXEL:
		case MXLV_AD_COPY_FRAME:
		case MXLV_AD_CORRECT_FRAME:
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		case MXLV_AD_EXTENDED_STATUS:
		case MXLV_AD_FRAMESIZE:
		case MXLV_AD_GET_ROI_FRAME:
		case MXLV_AD_IMAGE_FORMAT:
		case MXLV_AD_IMAGE_FORMAT_NAME:
		case MXLV_AD_IMAGE_FRAME_BUFFER:
		case MXLV_AD_LAST_FRAME_NUMBER:
		case MXLV_AD_LOAD_FRAME:
		case MXLV_AD_PROPERTY_DOUBLE:
		case MXLV_AD_PROPERTY_LONG:
		case MXLV_AD_PROPERTY_STRING:
		case MXLV_AD_READOUT_FRAME:
		case MXLV_AD_ROI:
		case MXLV_AD_ROI_FRAME_BUFFER:
		case MXLV_AD_SAVE_FRAME:
		case MXLV_AD_STATUS:
		case MXLV_AD_STOP:
		case MXLV_AD_TRANSFER_FRAME:
		case MXLV_AD_TRIGGER:

#if PR_AREA_DETECTOR_DEBUG
		case MXLV_AD_ROI_NUMBER:
		case MXLV_AD_FRAME_FILENAME:
		case MXLV_AD_SEQUENCE_TYPE:
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
#endif
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
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

#if PR_AREA_DETECTOR_DEBUG
	unsigned long i;
#endif

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	ad = (MX_AREA_DETECTOR *) (record->record_class_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

#if PR_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', field = '%s', operation = %d",
		fname, record->name, record_field->name, operation));
#endif

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_AD_BINSIZE:
			mx_status = mx_area_detector_get_binsize( record,
								NULL, NULL );
			break;
		case MXLV_AD_BYTES_PER_FRAME:
			mx_status = mx_area_detector_get_bytes_per_frame(
								record, NULL );
			break;
		case MXLV_AD_BYTES_PER_PIXEL:
			mx_status = mx_area_detector_get_bytes_per_pixel(
								record, NULL );
			break;
		case MXLV_AD_EXTENDED_STATUS:
			mx_status = mx_area_detector_get_extended_status(
							record, NULL, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			snprintf(
			    ad->extended_status, sizeof(ad->extended_status),
			    "%ld %#lx", ad->last_frame_number, ad->status );
			break;
		case MXLV_AD_IMAGE_FORMAT:
		case MXLV_AD_IMAGE_FORMAT_NAME:
			mx_status = 
			   mx_area_detector_get_image_format( record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_image_get_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_get_framesize( record,
								NULL, NULL );
			break;
		case MXLV_AD_IMAGE_FRAME_BUFFER:
			if ( ad->image_frame_buffer == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			"Area detector '%s' has not yet taken its first frame.",
					record->name );
			}
			break;
		case MXLV_AD_LAST_FRAME_NUMBER:
			mx_status = mx_area_detector_get_last_frame_number(
								record, NULL );
			break;
		case MXLV_AD_MAXIMUM_FRAMESIZE:
			mx_status = mx_area_detector_get_maximum_framesize(
							record, NULL, NULL );
			break;
		case MXLV_AD_PROPERTY_DOUBLE:
			mx_status = mx_area_detector_get_property_double(
						record, ad->property_name,
						NULL );
			break;
		case MXLV_AD_PROPERTY_LONG:
			mx_status = mx_area_detector_get_property_long(
						record, ad->property_name,
						NULL );
			break;
		case MXLV_AD_PROPERTY_STRING:
			mx_status = mx_area_detector_get_property_string(
						record, ad->property_name,
						NULL, 0 );
			break;
		case MXLV_AD_ROI:
			mx_status = mx_area_detector_get_roi( record,
							ad->roi_number, NULL );
#if PR_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: get ROI(%lu) = (%lu,%lu,%lu,%lu)",
				fname, ad->roi_number,
				ad->roi[0], ad->roi[1],
				ad->roi[2], ad->roi[3]));
#endif
			break;
		case MXLV_AD_ROI_FRAME_BUFFER:
			if ( ad->roi_frame_buffer == NULL ) {
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
		"Area detector '%s' has not yet initialized its first ROI.",
					record->name );
			}
			break;
		case MXLV_AD_STATUS:
			mx_status = mx_area_detector_get_status( record, NULL );
			break;

#if PR_AREA_DETECTOR_DEBUG
		case MXLV_AD_ROI_NUMBER:
			MX_DEBUG(-2,("%s: ROI number = %ld",
				fname, ad->roi_number));
			break;
		case MXLV_AD_SEQUENCE_TYPE:
			MX_DEBUG(-2,("%s: sequence type = %ld",
				fname, ad->sequence_parameters.sequence_type));
			break;
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
			MX_DEBUG(-2,("%s: num_parameters = %ld",
				fname, ad->sequence_parameters.num_parameters));
			break;
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
			for ( i = 0;
			    i < ad->sequence_parameters.num_parameters;
			    i++ )
			{
				MX_DEBUG(-2,("%s: parameter_array[%lu] = %g",
					    fname, i,
				   ad->sequence_parameters.parameter_array[i]));
			}
			break;
#endif
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
		case MXLV_AD_BINSIZE:
			mx_status = mx_area_detector_set_binsize( record,
						ad->binsize[0],
						ad->binsize[1] );
			break;
		case MXLV_AD_COPY_FRAME:
			mx_status = mx_area_detector_copy_frame( record,
					ad->copy_frame[0], ad->copy_frame[1] );
			break;
		case MXLV_AD_CORRECT_FRAME:
			mx_status = mx_area_detector_correct_frame( record );
			break;
		case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
			mx_status = mx_area_detector_measure_correction_frame(
					record, ad->correction_measurement_type,
					ad->correction_measurement_time,
					ad->num_correction_measurements );
			break;
		case MXLV_AD_FRAMESIZE:
			mx_status = mx_area_detector_set_framesize( record,
						ad->framesize[0],
						ad->framesize[1] );
			break;
		case MXLV_AD_GET_ROI_FRAME:
			mx_status = mxp_area_detector_get_roi_frame_handler(
					record, record_field, ad );
			break;
		case MXLV_AD_IMAGE_FORMAT_NAME:
			mx_status = mx_image_get_format_type_from_name(
				ad->image_format_name, &(ad->image_format) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Fall through to the next case. */
		case MXLV_AD_IMAGE_FORMAT:
			mx_status = mx_area_detector_set_image_format( record,
							ad->image_format );
			break;
		case MXLV_AD_LOAD_FRAME:
			mx_status = mx_area_detector_load_frame( record,
					ad->load_frame, ad->frame_filename );
			break;
		case MXLV_AD_PROPERTY_DOUBLE:
			mx_status = mx_area_detector_set_property_double(
						record, ad->property_name,
						ad->property_double );
			break;
		case MXLV_AD_PROPERTY_LONG:
			mx_status = mx_area_detector_set_property_long(
						record, ad->property_name,
						ad->property_long );
			break;
		case MXLV_AD_PROPERTY_STRING:
			mx_status = mx_area_detector_set_property_string(
						record, ad->property_name,
						ad->property_string );
			break;
		case MXLV_AD_ROI:
			mx_status = mx_area_detector_set_roi( record,
						ad->roi_number, ad->roi );
#if PR_AREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: set ROI(%lu) = (%lu,%lu,%lu,%lu)",
				fname, ad->roi_number,
				ad->roi[0], ad->roi[1],
				ad->roi[2], ad->roi[3]));
#endif
			break;
		case MXLV_AD_READOUT_FRAME:
			mx_status = mxp_area_detector_readout_frame_handler(
					record, record_field, ad );
			break;
		case MXLV_AD_SAVE_FRAME:
			mx_status = mx_area_detector_save_frame( record,
					ad->save_frame, ad->frame_filename );
			break;
		case MXLV_AD_STOP:
			mx_status = mx_area_detector_stop( record );
			break;
		case MXLV_AD_TRANSFER_FRAME:
			mx_status = mx_area_detector_transfer_frame( record,
							ad->transfer_frame );
			break;
		case MXLV_AD_TRIGGER:
			mx_status = mx_area_detector_trigger( record );
			break;

#if PR_AREA_DETECTOR_DEBUG
		case MXLV_AD_ROI_NUMBER:
			MX_DEBUG(-2,("%s: ROI number = %ld",
				fname, ad->roi_number));
			break;
		case MXLV_AD_FRAME_FILENAME:
			MX_DEBUG(-2,("%s: Frame filename = '%s'",
				fname, ad->frame_filename));
			break;
		case MXLV_AD_SEQUENCE_TYPE:
			MX_DEBUG(-2,("%s: sequence type = %ld",
				fname, ad->sequence_parameters.sequence_type));
			break;
		case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
			MX_DEBUG(-2,("%s: num_parameters = %ld",
				fname, ad->sequence_parameters.num_parameters));
			break;
#endif
		case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
#if PR_AREA_DETECTOR_DEBUG
			for ( i = 0;
			    i < ad->sequence_parameters.num_parameters;
			    i++ )
			{
				MX_DEBUG(-2,("%s: parameter_array[%lu] = %g",
					    fname, i,
				   ad->sequence_parameters.parameter_array[i]));
			}
#endif
			mx_status = mx_area_detector_set_sequence_parameters(
					record, &(ad->sequence_parameters) );
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

