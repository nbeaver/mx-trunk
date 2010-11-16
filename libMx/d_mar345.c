/*
 * Name:    d_mar345.c
 *
 * Purpose: MX driver for the Mar 345 image plate detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_MAR345_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_bit.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "d_mar345.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_mar345_record_function_list = {
	mxd_mar345_initialize_driver,
	mxd_mar345_create_record_structures,
	mxd_mar345_finish_record_initialization,
	NULL,
	NULL,
	mxd_mar345_open
};

MX_AREA_DETECTOR_FUNCTION_LIST mxd_mar345_ad_function_list = {
	NULL,
	mxd_mar345_trigger,
	mxd_mar345_abort,
	mxd_mar345_abort,
	mxd_mar345_get_last_frame_number,
	mxd_mar345_get_total_num_frames,
	mxd_mar345_get_status,
	NULL,
	mxd_mar345_readout_frame,
	mxd_mar345_correct_frame,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_mar345_get_parameter,
	mxd_mar345_set_parameter,
	mx_area_detector_default_measure_correction
};

MX_RECORD_FIELD_DEFAULTS mxd_mar345_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AREA_DETECTOR_STANDARD_FIELDS,
	MX_AREA_DETECTOR_CORRECTION_STANDARD_FIELDS,
	MXD_MAR345_STANDARD_FIELDS
};

long mxd_mar345_num_record_fields
		= sizeof( mxd_mar345_record_field_defaults )
			/ sizeof( mxd_mar345_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_mar345_rfield_def_ptr
			= &mxd_mar345_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_mar345_get_pointers( MX_AREA_DETECTOR *ad,
			MX_MAR345 **mar345,
			const char *calling_fname )
{
	static const char fname[] = "mxd_mar345_get_pointers()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_AREA_DETECTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (mar345 == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MAR345 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*mar345 = (MX_MAR345 *) ad->record->record_type_struct;

	if ( *mar345 == (MX_MAR345 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_MAR345 pointer for record '%s' passed by '%s' is NULL.",
			ad->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_mar345_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_rois_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_area_detector_initialize_driver( driver,
					&maximum_num_rois_varargs_cookie );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mar345_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mar345_create_record_structures()";

	MX_AREA_DETECTOR *ad;
	MX_MAR345 *mar345;

	ad = (MX_AREA_DETECTOR *) malloc( sizeof(MX_AREA_DETECTOR) );

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AREA_DETECTOR structure." );
	}

	mar345 = (MX_MAR345 *)
				malloc( sizeof(MX_MAR345) );

	if ( mar345 == (MX_MAR345 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for an MX_MAR345 structure." );
	}

	record->record_class_struct = ad;
	record->record_type_struct = mar345;
	record->class_specific_function_list = &mxd_mar345_ad_function_list;

	memset( &(ad->sequence_parameters),
			0, sizeof(ad->sequence_parameters) );

	ad->record = record;
	mar345->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_mar345_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	MX_MAR345 *mar345 = NULL;
	mx_status_type mx_status;

	mx_status = mx_area_detector_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_mar345_open()";

	MX_AREA_DETECTOR *ad;
	MX_MAR345 *mar345 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ad = (MX_AREA_DETECTOR *) record->record_class_struct;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;

	ad->maximum_framesize[0] = 0;
	ad->maximum_framesize[1] = 0;

	/* Get the current binsize.  This will update the current framesize
	 * as well as a side effect.
	 */

	mx_status = mx_area_detector_get_binsize( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s: framesize = (%lu, %lu), binsize = (%lu, %lu)",
		fname, ad->framesize[0], ad->framesize[1],
		ad->binsize[0], ad->binsize[1]));
#endif

	/* Initialize area detector parameters. */

	ad->byte_order = (long) mx_native_byteorder();
	ad->header_length = 0;

	mx_status = mx_area_detector_get_bytes_per_frame( record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_trigger( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_trigger()";

	MX_MAR345 *mar345 = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'",
		fname, ad->record->name ));
#endif

	sp = &(ad->sequence_parameters);

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_abort( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_abort()";

	MX_MAR345 *mar345 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mar345_get_last_frame_number( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_get_last_frame_number()";

	MX_MAR345 *mar345 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_get_total_num_frames( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_get_total_num_frames()";

	MX_MAR345 *mar345 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_get_status( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_get_status()";

	MX_MAR345 *mar345 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_readout_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_readout_frame()";

	MX_MAR345 *mar345;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mar345_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_correct_frame()";

	MX_MAR345 *mar345;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG_FRAME_CORRECTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	sp = &(ad->sequence_parameters);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_mar345_get_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_get_parameter()";

	MX_MAR345 *mar345 = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	int32_t trigger_mode, image_mode;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	sp = &(ad->sequence_parameters);

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		break;

	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		ad->binsize[0] = 1;
		ad->binsize[1] = 1;

		if ( ad->binsize[0] > 0 ) {
			ad->framesize[0] =
				ad->maximum_framesize[0] / ad->binsize[0];
		}
		if ( ad->binsize[1] > 0 ) {
			ad->framesize[1] =
				ad->maximum_framesize[1] / ad->binsize[1];
		}
		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		mx_status = mx_image_get_image_format_name_from_type(
				ad->image_format, ad->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		break;

	case MXLV_AD_BYTES_PER_PIXEL:
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		break;

	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		image_mode = 0;

		switch( image_mode ) {
		case 0:			/* Single */
			sp->sequence_type = MXT_SQ_ONE_SHOT;
			sp->num_parameters = 1;
			break;
		case 1:			/* Multiple */
			sp->sequence_type = MXT_SQ_MULTIFRAME;
			sp->num_parameters = 3;
			break;
		case 2:			/* Continuous */
			sp->sequence_type = MXT_SQ_CONTINUOUS;
			sp->num_parameters = 1;
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unrecognized image mode %ld returned for "
			"EPICS Area Detector '%s'",
				(long) image_mode, ad->record->name );
			break;
		}
		break;
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		switch( sp->sequence_type ) {
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:

#if 0
			if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
				sp->parameter_array[0] = num_images;
				sp->parameter_array[1] = acquire_time;
				sp->parameter_array[2] =
					acquire_time + acquire_period;
			} else {
				sp->parameter_array[0] = acquire_time;
			}
#endif
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Sequence type %ld is not supported for "
			"EPICS Area Detector '%s'.",
				sp->sequence_type, ad->record->name );
			break;
		}
		break;

	case MXLV_AD_TRIGGER_MODE:
		trigger_mode = 0;

		switch( trigger_mode ) {
		case 0:			/* Internal */
			ad->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
			break;
		case 1:			/* External */
			ad->trigger_mode = MXT_IMAGE_EXTERNAL_TRIGGER;
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Unrecognized trigger mode %ld returned for "
			"EPICS Area Detector '%s'.",
				(long) trigger_mode, ad->record->name );
			break;
		}
		break;

	default:
		mx_status = mx_area_detector_default_get_parameter_handler(ad);
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_mar345_set_parameter( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mxd_mar345_set_parameter()";

	MX_MAR345 *mar345 = NULL;
	MX_SEQUENCE_PARAMETERS *sp;
	double acquire_time, acquire_period;
	int32_t trigger_mode, image_mode, num_images;
	int32_t x_binsize, y_binsize;
	double raw_x_binsize, raw_y_binsize;
	mx_status_type mx_status;

	mx_status = mxd_mar345_get_pointers( ad, &mar345, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_MAR345_DEBUG
	{
		char name_buffer[MXU_FIELD_NAME_LENGTH+1];

		MX_DEBUG(-2,("%s: record '%s', parameter '%s'",
			fname, ad->record->name,
			mx_get_parameter_name_from_type(
				ad->record, ad->parameter_type,
				name_buffer, sizeof(name_buffer)) ));
	}
#endif

	sp = &(ad->sequence_parameters);

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		if ( ad->parameter_type == MXLV_AD_FRAMESIZE ) {
			raw_x_binsize =
				mx_divide_safely( ad->maximum_framesize[0],
							ad->framesize[0] );
			raw_y_binsize =
				mx_divide_safely( ad->maximum_framesize[1],
							ad->framesize[1] );

			x_binsize = mx_round( raw_x_binsize );
			y_binsize = mx_round( raw_y_binsize );
		} else {
			x_binsize = ad->binsize[0];
			y_binsize = ad->binsize[1];
		}

		break;

	case MXLV_AD_SEQUENCE_TYPE:
		switch( sp->sequence_type ) {
		case MXT_SQ_ONE_SHOT:
			image_mode = 0;
			break;
		case MXT_SQ_CONTINUOUS:
			image_mode = 2;
			break;
		case MXT_SQ_MULTIFRAME:
			image_mode = 1;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector sequence mode %ld is not supported for "
			"EPICS Area Detector '%s'.",
				(long) image_mode, ad->record->name );
			break;
		}

		break;

	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
		break;

	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY: 
		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			acquire_time = sp->parameter_array[1];
		} else {
			acquire_time = sp->parameter_array[0];
		}

		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			num_images = sp->parameter_array[0];
		} else {
			num_images = 1;
		}

		if ( sp->sequence_type == MXT_SQ_MULTIFRAME ) {
			acquire_period = sp->parameter_array[2] - acquire_time;

		}
		break; 

	case MXLV_AD_TRIGGER_MODE:
		switch( ad->trigger_mode ) {
		case MXT_IMAGE_INTERNAL_TRIGGER:
			trigger_mode = 0;
			break;
		case MXT_IMAGE_EXTERNAL_TRIGGER:
			trigger_mode = 1;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported trigger mode %ld requested for "
			"EPICS Area Detector '%s'.",
				(long) trigger_mode, ad->record->name );
		}

		break;

	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Changing parameter '%s' for area detector '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
		break;

	default:
		mx_status = mx_area_detector_default_set_parameter_handler(ad);
		break;
	}

	return mx_status;
}

