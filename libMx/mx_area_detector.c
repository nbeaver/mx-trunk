/*
 * Name:    mx_area_detector.c
 *
 * Purpose: Functions for reading frames from an area detector.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG    			FALSE

#define MX_AREA_DETECTOR_DEBUG_MX_IMAGE_ALLOC		FALSE

#define MX_AREA_DETECTOR_DEBUG_DEZINGER			FALSE

#define MX_AREA_DETECTOR_DEBUG_FRAME_TIMING		FALSE

#define MX_AREA_DETECTOR_DEBUG_CORRECTION		FALSE

#define MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING	FALSE

#define MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME	FALSE

#define MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES		FALSE

#define MX_AREA_DETECTOR_DEBUG_FRAME_PARAMETERS		FALSE

#define MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD	FALSE

#define MX_AREA_DETECTOR_DEBUG_VCTEST			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_bit.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"
#include "mx_memory.h"
#include "mx_key.h"
#include "mx_array.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_image.h"
#include "mx_area_detector.h"

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_get_pointers( MX_RECORD *record,
			MX_AREA_DETECTOR **ad,
			MX_AREA_DETECTOR_FUNCTION_LIST **flist_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_area_detector_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( record->mx_class != MXC_AREA_DETECTOR ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a area_detector.",
			record->name, calling_fname );
	}

	if ( ad != (MX_AREA_DETECTOR **) NULL ) {
		*ad = (MX_AREA_DETECTOR *) (record->record_class_struct);

		if ( *ad == (MX_AREA_DETECTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_AREA_DETECTOR pointer for record '%s' passed by '%s' is NULL",
				record->name, calling_fname );
		}
	}

	if ( flist_ptr != (MX_AREA_DETECTOR_FUNCTION_LIST **) NULL ) {
		*flist_ptr = (MX_AREA_DETECTOR_FUNCTION_LIST *)
				(record->class_specific_function_list);

		if ( *flist_ptr == (MX_AREA_DETECTOR_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AREA_DETECTOR_FUNCTION_LIST pointer for "
			"record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

#if 0
	if ( *ad != NULL ) {
		MX_DEBUG(-2,("*** %s: bytes_per_frame = %ld",
			calling_fname, (*ad)->bytes_per_frame));
		MX_DEBUG(-2,("*** %s: bytes_per_pixel = %g",
			calling_fname, (*ad)->bytes_per_pixel));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_initialize_type( long record_type,
			long *num_record_fields,
			MX_RECORD_FIELD_DEFAULTS **record_field_defaults,
			long *maximum_num_rois_varargs_cookie )
{
	static const char fname[] = "mx_area_detector_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( num_record_fields == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"num_record_fields pointer passed was NULL." );
	}
	if ( record_field_defaults == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"record_field_defaults pointer passed was NULL." );
	}
	if ( maximum_num_rois_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
	"maximum_num_rois_varargs_cookie pointer passed was NULL." );
	}

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	*record_field_defaults = *record_field_defaults_ptr;

	if ( *record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	*num_record_fields = *(driver->num_record_fields);

	/*** Construct a varargs cookie for 'maximum_num_rois'. ***/

	mx_status = mx_find_record_field_defaults_index(
			*record_field_defaults, *num_record_fields,
			"maximum_num_rois", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_rois_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* 'roi_array' depends on 'maximum_num_rois'. */

	mx_status = mx_find_record_field_defaults( *record_field_defaults,
			*num_record_fields, "roi_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *maximum_num_rois_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mx_area_detector_finish_record_initialization()";

	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->correction_flags = ad->initial_correction_flags & MXFT_AD_ALL;

	ad->correction_measurement = NULL;

	ad->correction_frames_are_unbinned = FALSE;

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = -1;
	ad->status = 0;
	ad->extended_status[0] = '\0';

	ad->byte_order = mx_native_byteorder();

	ad->arm     = FALSE;
	ad->trigger = FALSE;
	ad->stop    = FALSE;
	ad->abort   = FALSE;
	ad->busy    = FALSE;

	ad->correction_measurement_in_progress = FALSE;

	ad->geom_corr_after_flood           = FALSE;
	ad->correction_frame_geom_corr_last = FALSE;
	ad->correction_frame_no_geom_corr   = FALSE;

	ad->current_num_rois = ad->maximum_num_rois;
	ad->roi_number = 0;
	ad->roi[0] = 0;
	ad->roi[1] = 0;
	ad->roi[2] = 0;
	ad->roi[3] = 0;

	ad->roi_frame = NULL;
	ad->roi_frame_buffer = NULL;

	ad->image_frame = NULL;

	ad->image_frame_header_length = 0;
	ad->image_frame_header = NULL;
	ad->image_frame_data = NULL;

	ad->transfer_destination_frame = NULL;
	ad->dezinger_threshold = DBL_MAX;

	ad->frame_filename[0] = '\0';

	ad->mask_frame = NULL;
	ad->mask_frame_buffer = NULL;

	ad->bias_frame = NULL;
	ad->bias_frame_buffer = NULL;

	ad->use_scaled_dark_current = FALSE;
	ad->dark_current_exposure_time = 1.0;

	ad->sequence_start_delay   = 0.0;
	ad->total_acquisition_time = 0.0;
	ad->detector_readout_time  = 0.0;
	ad->total_sequence_time    = 0.0;

	ad->dark_current_frame = NULL;
	ad->dark_current_frame_buffer = NULL;

	ad->flood_field_average_intensity = 1.0;
	ad->bias_average_intensity = 1.0;

	ad->flood_field_frame = NULL;
	ad->flood_field_frame_buffer = NULL;

	ad->flood_field_scale_max = DBL_MAX;
	ad->flood_field_scale_min = DBL_MIN;

	ad->rebinned_mask_frame = NULL;
	ad->rebinned_bias_frame = NULL;
	ad->rebinned_dark_current_frame = NULL;
	ad->rebinned_flood_field_frame = NULL;

	ad->dark_current_offset_array = NULL;
	ad->flood_field_scale_array = NULL;
	ad->old_exposure_time = -1.0;

	strlcpy(ad->image_format_name, "DEFAULT", MXU_IMAGE_FORMAT_NAME_LENGTH);

	mx_status = mx_image_get_format_type_from_name(
			ad->image_format_name, &(ad->image_format) );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_load_correction_files( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_load_correction_files()";

	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name));

	MX_DEBUG(-2,("%s: ad->image_format = %ld", fname, ad->image_format));
	MX_DEBUG(-2,("%s: ad->framesize = (%ld,%ld)",
		fname, ad->framesize[0], ad->framesize[1]));
#endif

	if ( strlen(ad->mask_filename) > 0 ) {
		mx_status = mx_area_detector_load_frame( record,
							MXFT_AD_MASK_FRAME,
							ad->mask_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

	if ( strlen(ad->bias_filename) > 0 ) {
		mx_status = mx_area_detector_load_frame( record,
							MXFT_AD_BIAS_FRAME,
							ad->bias_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

	if ( strlen(ad->dark_current_filename) > 0 ) {
		mx_status = mx_area_detector_load_frame( record,
						MXFT_AD_DARK_CURRENT_FRAME,
						ad->dark_current_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

	if ( strlen(ad->flood_field_filename) > 0 ) {
		mx_status = mx_area_detector_load_frame( record,
						MXFT_AD_FLOOD_FIELD_FRAME,
						ad->flood_field_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for area detector '%s'.",
		fname, ad->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_get_register( MX_RECORD *record,
				char *register_name,
				long *register_value )
{
	static const char fname[] = "mx_area_detector_get_register()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( register_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_name pointer passed was NULL." );
	}

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	strlcpy( ad->register_name, register_name, MXU_FIELD_NAME_LENGTH );

	ad->parameter_type = MXLV_AD_REGISTER_VALUE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( register_value != NULL ) {
		*register_value = ad->register_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_register( MX_RECORD *record,
					char *register_name,
					long register_value )
{
	static const char fname[] = "mx_area_detector_set_register()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( register_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The register_name pointer passed was NULL." );
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	strlcpy( ad->register_name, register_name, MXU_FIELD_NAME_LENGTH );

	ad->register_value = register_value;

	ad->parameter_type = MXLV_AD_REGISTER_VALUE;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_get_image_format( MX_RECORD *record, long *format )
{
	static const char fname[] = "mx_area_detector_get_image_format()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_IMAGE_FORMAT;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( format != NULL ) {
		*format = ad->image_format;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_image_format( MX_RECORD *record, long format )
{
	static const char fname[] = "mx_area_detector_set_image_format()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_IMAGE_FORMAT;

	ad->image_format = format;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_maximum_framesize( MX_RECORD *record,
				long *maximum_x_framesize,
				long *maximum_y_framesize )
{
	static const char fname[] = "mx_area_detector_get_maximum_framesize()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = 
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_MAXIMUM_FRAMESIZE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( maximum_x_framesize != NULL ) {
		*maximum_x_framesize = ad->maximum_framesize[0];
	}
	if ( maximum_y_framesize != NULL ) {
		*maximum_y_framesize = ad->maximum_framesize[1];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_framesize( MX_RECORD *record,
				long *x_framesize,
				long *y_framesize )
{
	static const char fname[] = "mx_area_detector_get_framesize()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = 
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_FRAMESIZE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_framesize != NULL ) {
		*x_framesize = ad->framesize[0];
	}
	if ( y_framesize != NULL ) {
		*y_framesize = ad->framesize[1];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_framesize( MX_RECORD *record,
				long x_framesize,
				long y_framesize )
{
	static const char fname[] = "mx_area_detector_set_framesize()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = 
			mx_area_detector_default_set_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_FRAMESIZE;

	ad->framesize[0] = x_framesize;
	ad->framesize[1] = y_framesize;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_binsize( MX_RECORD *record,
				long *x_binsize,
				long *y_binsize )
{
	static const char fname[] = "mx_area_detector_get_binsize()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = 
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_BINSIZE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_binsize != NULL ) {
		*x_binsize = ad->binsize[0];
	}
	if ( y_binsize != NULL ) {
		*y_binsize = ad->binsize[1];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_binsize( MX_RECORD *record,
				long x_binsize,
				long y_binsize )
{
	static const char fname[] = "mx_area_detector_set_binsize()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = 
			mx_area_detector_default_set_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_BINSIZE;

	ad->binsize[0] = x_binsize;
	ad->binsize[1] = y_binsize;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_roi( MX_RECORD *record,
				unsigned long roi_number,
				unsigned long *roi )
{
	static const char fname[] = "mx_area_detector_get_roi()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = 
			mx_area_detector_default_get_parameter_handler;
	}

	if ( roi_number >= ad->maximum_num_rois ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The ROI number %ld for area detector '%s' is "
		"outside the range of allowed values (0 to %ld).",
			ad->roi_number, ad->record->name,
			ad->maximum_num_rois - 1 );
	}

	ad->roi_number = roi_number;

	ad->parameter_type = MXLV_AD_ROI;

	mx_status = (*get_parameter_fn)( ad );

	ad->roi_array[ roi_number ][0] = ad->roi[0];
	ad->roi_array[ roi_number ][1] = ad->roi[1];
	ad->roi_array[ roi_number ][2] = ad->roi[2];
	ad->roi_array[ roi_number ][3] = ad->roi[3];

	if ( roi != NULL ) {
		roi[0] = ad->roi[0];
		roi[1] = ad->roi[1];
		roi[2] = ad->roi[2];
		roi[3] = ad->roi[3];
	}

	if ( ad->roi[0] > ad->roi[1] ) {
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"For area detector '%s', ROI %lu, the X minimum %lu is larger "
		"than the X maximum %lu.",
			record->name, roi_number, ad->roi[0], ad->roi[1] );
	}
	if ( ad->roi[2] > ad->roi[3] ) {
		return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		"For area detector '%s', ROI %lu, the Y minimum %lu is larger "
		"than the Y maximum %lu.",
			record->name, roi_number, ad->roi[2], ad->roi[3] );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_roi( MX_RECORD *record,
				unsigned long roi_number,
				unsigned long *roi )
{
	static const char fname[] = "mx_area_detector_set_roi()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = 
			mx_area_detector_default_set_parameter_handler;
	}

	if ( roi_number >= ad->maximum_num_rois ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The ROI number %ld for area detector '%s' is "
		"outside the range of allowed values (0-%ld).",
			ad->roi_number, ad->record->name,
			ad->maximum_num_rois - 1 );
	}

	if ( roi[1] >= ad->framesize[0] ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested X maximum %lu is outside the allowed "
			"range of (0-%ld) for area detector '%s'.", roi[1],
			ad->framesize[0] - 1, ad->record->name );
	}
	if ( roi[3] >= ad->framesize[1] ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested Y maximum %lu is outside the allowed "
			"range of (0-%ld) for area detector '%s'.", roi[3],
			ad->framesize[1] - 1, ad->record->name );
	}
	if ( roi[0] > roi[1] ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested X minimum %lu is larger than "
			"the requested X maximum %lu for area detector '%s'.  "
			"This is not allowed.",  roi[0], roi[1],
			record->name );
	}
	if ( roi[2] > roi[3] ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested Y minimum %lu is larger than "
			"the requested Y maximum %lu for area detector '%s'.  "
			"This is not allowed.",  roi[2], roi[3],
			record->name );
	}

	ad->roi_number = roi_number;

	ad->parameter_type = MXLV_AD_ROI;

	ad->roi_array[ roi_number ][0] = roi[0];
	ad->roi_array[ roi_number ][1] = roi[1];
	ad->roi_array[ roi_number ][2] = roi[2];
	ad->roi_array[ roi_number ][3] = roi[3];

	ad->roi[0] = roi[0];
	ad->roi[1] = roi[1];
	ad->roi[2] = roi[2];
	ad->roi[3] = roi[3];

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_subframe_size( MX_RECORD *record,
				unsigned long *num_columns )
{
	static const char fname[] = "mx_area_detector_get_subframe_size()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_SUBFRAME_SIZE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_columns != NULL ) {
		*num_columns = ad->subframe_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_subframe_size( MX_RECORD *record,
				unsigned long num_columns )
{
	static const char fname[] = "mx_area_detector_set_subframe_size()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_SUBFRAME_SIZE;

	ad->subframe_size = num_columns;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_bytes_per_frame( MX_RECORD *record, long *bytes_per_frame )
{
	static const char fname[] = "mx_area_detector_get_bytes_per_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_BYTES_PER_FRAME;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_per_frame != NULL ) {
		*bytes_per_frame = ad->bytes_per_frame;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_bytes_per_pixel(MX_RECORD *record, double *bytes_per_pixel)
{
	static const char fname[] = "mx_area_detector_get_bytes_per_pixel()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_BYTES_PER_PIXEL;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_per_pixel != NULL ) {
		*bytes_per_pixel = ad->bytes_per_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_bits_per_pixel(MX_RECORD *record, long *bits_per_pixel)
{
	static const char fname[] = "mx_area_detector_get_bits_per_pixel()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_BITS_PER_PIXEL;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bits_per_pixel != NULL ) {
		*bits_per_pixel = ad->bits_per_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_API mx_status_type
mx_area_detector_get_correction_flags( MX_RECORD *record,
					unsigned long *correction_flags )
{
	static const char fname[] = "mx_area_detector_get_correction_flags()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_CORRECTION_FLAGS;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( correction_flags != NULL ) {
		*correction_flags = ad->correction_flags;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_API mx_status_type
mx_area_detector_set_correction_flags( MX_RECORD *record,
					unsigned long correction_flags )
{
	static const char fname[] = "mx_area_detector_set_correction_flags()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	mx_free( ad->dark_current_offset_array );
	mx_free( ad->flood_field_scale_array );

	ad->parameter_type = MXLV_AD_CORRECTION_FLAGS;
	ad->correction_flags = correction_flags;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_API mx_status_type
mx_area_detector_measure_correction_frame( MX_RECORD *record,
					long correction_measurement_type,
					double correction_measurement_time,
					long num_correction_measurements )
{
	static const char fname[] =
		"mx_area_detector_measure_correction_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *measure_correction_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	measure_correction_fn = flist->measure_correction;

	if ( measure_correction_fn == NULL ) {
		measure_correction_fn =
			mx_area_detector_default_measure_correction;
	}

	ad->correction_measurement_type = correction_measurement_type;
	ad->correction_measurement_time = correction_measurement_time;
	ad->num_correction_measurements = num_correction_measurements;

	ad->correction_measurement_in_progress = TRUE;

	mx_status = (*measure_correction_fn)( ad );

	ad->correction_measurement_in_progress = FALSE;

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Some frame types need special things to happen after
	 * they are measured.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_frame != NULL ) {
			mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );
		}

		mx_free( ad->dark_current_offset_array );
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		if ( ad->flood_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flood_field_frame, ad->mask_frame,
					&(ad->flood_field_average_intensity) );
		}

		mx_free( ad->flood_field_scale_array );
		break;
	}

	return mx_status;
}

MX_API mx_status_type
mx_area_detector_get_use_scaled_dark_current_flag( MX_RECORD *record,
					mx_bool_type *use_scaled_dark_current )
{
	static const char fname[] =
			"mx_area_detector_get_use_scaled_dark_current_flag()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_USE_SCALED_DARK_CURRENT;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( use_scaled_dark_current != NULL ) {
		*use_scaled_dark_current = ad->use_scaled_dark_current;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_API mx_status_type
mx_area_detector_set_use_scaled_dark_current_flag( MX_RECORD *record,
					mx_bool_type use_scaled_dark_current )
{
	static const char fname[] =
			"mx_area_detector_set_use_scaled_dark_current_flag()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Discard any existing dark current offset array, since
	 * turning on or off the use_scaled_dark_current flag can
	 * affect the computed exposure scale factor.
	 */

	mx_free( ad->dark_current_offset_array );

	ad->parameter_type = MXLV_AD_USE_SCALED_DARK_CURRENT;
	ad->use_scaled_dark_current = use_scaled_dark_current;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_get_sequence_start_delay( MX_RECORD *record,
					double *sequence_start_delay )
{
	static const char fname[] =
			"mx_area_detector_get_sequence_start_delay()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_SEQUENCE_START_DELAY;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sequence_start_delay != (double *) NULL ) {
		*sequence_start_delay = ad->sequence_start_delay;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_sequence_start_delay( MX_RECORD *record,
					double sequence_start_delay )
{
	static const char fname[] =
			"mx_area_detector_set_sequence_start_delay()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_SEQUENCE_START_DELAY;

	ad->sequence_start_delay = sequence_start_delay;

	mx_status = (*get_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_total_acquisition_time( MX_RECORD *record,
					double *total_acquisition_time )
{
	static const char fname[] =
			"mx_area_detector_get_total_acquisition_time()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_TOTAL_ACQUISITION_TIME;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( total_acquisition_time != (double *) NULL ) {
		*total_acquisition_time = ad->total_acquisition_time;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_detector_readout_time( MX_RECORD *record,
					double *detector_readout_time )
{
	static const char fname[] =
			"mx_area_detector_get_detector_readout_time()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_DETECTOR_READOUT_TIME;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( detector_readout_time != (double *) NULL ) {
		*detector_readout_time = ad->detector_readout_time;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_total_sequence_time( MX_RECORD *record,
					double *total_sequence_time )
{
	static const char fname[] =
			"mx_area_detector_get_total_sequence_time()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_TOTAL_SEQUENCE_TIME;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( total_sequence_time != (double *) NULL ) {
		*total_sequence_time = ad->total_sequence_time;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_sequence_parameters( MX_RECORD *record,
			MX_SEQUENCE_PARAMETERS *sequence_parameters )
{
	static const char fname[] =
			"mx_area_detector_get_sequence_parameters()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	/* Get the sequence type. */

	ad->parameter_type = MXLV_AD_SEQUENCE_TYPE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the number of parameters. */

	ad->parameter_type = MXLV_AD_NUM_SEQUENCE_PARAMETERS;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the parameter array. */

	ad->parameter_type = MXLV_AD_SEQUENCE_PARAMETER_ARRAY;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
	
	if ( ad->sequence_parameters.num_parameters
			> MXU_MAX_SEQUENCE_PARAMETERS )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of sequence parameters %ld reported for "
		"the sequence given to area detector record '%s' "
		"is longer than the maximum allowed length of %d.",
			ad->sequence_parameters.num_parameters, record->name,
			MXU_MAX_SEQUENCE_PARAMETERS );
	}

	/* If the sequence_parameters pointer is NULL, then we are done. */

	if ( sequence_parameters == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If the sequence_parameters pointer points to the same place
	 * as &(ad->sequence_parameters), then we skip copying the data
	 * since it is already in the correct place.
	 */

	if ( sequence_parameters == &(ad->sequence_parameters) ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, copy the parameters to the supplied
	 * MX_SEQUENCE_PARAMETERS structure.
	 */

	sequence_parameters->sequence_type =
				ad->sequence_parameters.sequence_type;

	sequence_parameters->num_parameters =
				ad->sequence_parameters.num_parameters;

	/* FIXME! - For some undetermined reason, the for() that follows
	 * works, but the memcpy() does not.  Please show me the error
	 * of my ways.
	 */
#if 0
	memcpy( sequence_parameters->parameter_array,
				ad->sequence_parameters.parameter_array,
				ad->sequence_parameters.num_parameters );
#else
	{
		long i;

		for ( i = 0; i < ad->sequence_parameters.num_parameters; i++ ) {
			sequence_parameters->parameter_array[i]
				= ad->sequence_parameters.parameter_array[i];
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_sequence_parameters( MX_RECORD *record,
			MX_SEQUENCE_PARAMETERS *sequence_parameters )
{
	static const char fname[] =
			"mx_area_detector_set_sequence_parameters()";

	MX_AREA_DETECTOR *ad;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	double exposure_time;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Save the parameters, which will be used the next time that
	 * the area detector "arms".
	 */

	sp = &(ad->sequence_parameters);

	/* If needed, we copy the parameters to the
	 * ad->sequence_parameters structure.
	 */

	if ( sequence_parameters == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		/* If the sequence_parameter pointer passed to us
		 * is NULL, then we just use the values already
		 * in ad->sequence_parameters.
		 */

	} else if ( sequence_parameters == sp ) {
		/* If the sequence_parameters pointer passed to us points
		 * to the same place as &(ad->sequence_parameters), then
		 * we do nothing, since the data are already in the right
		 * place.
		 */

	} else {
		/* If we get here, then we must copy the values. */

		sp->num_parameters = sequence_parameters->num_parameters;

		sp->sequence_type = sequence_parameters->sequence_type;

		memcpy( sp->parameter_array,
			sequence_parameters->parameter_array,
			(sp->num_parameters) * sizeof(double) );
	}

	if ( sp->num_parameters > MXU_MAX_SEQUENCE_PARAMETERS ) {

		long num_parameters;

		num_parameters = sp->num_parameters;

		sp->num_parameters = MXU_MAX_SEQUENCE_PARAMETERS;

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of sequence parameters %ld requested for "
		"the sequence given to area detector record '%s' "
		"is longer than the maximum allowed length of %d.",
			num_parameters, record->name,
			MXU_MAX_SEQUENCE_PARAMETERS );
	}

	/* If the new sequence has a different exposure time than
	 * was used by the previous sequence, then discard any
	 * existing dark current offset array.
	 *
	 * Do not modify the value of ad->old_exposure_time here,
	 * since that is done just after calling the routine that
	 * calculates the new dark current offset array.
	 */

	mx_status = mx_sequence_get_exposure_time( sp, 0, &exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( mx_difference( exposure_time, ad->old_exposure_time ) > 0.001 ) {
		mx_free( ad->dark_current_offset_array );
	}

	/* Set the sequence type. */

	ad->parameter_type = MXLV_AD_SEQUENCE_TYPE;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the number of parameters. */

	ad->parameter_type = MXLV_AD_NUM_SEQUENCE_PARAMETERS;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the parameter array. */

	ad->parameter_type = MXLV_AD_SEQUENCE_PARAMETER_ARRAY;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_one_shot_mode( MX_RECORD *record, double exposure_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_ONE_SHOT;
	seq_params.num_parameters = 1;
	seq_params.parameter_array[0] = exposure_time;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_continuous_mode( MX_RECORD *record, double exposure_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_CONTINUOUS;
	seq_params.num_parameters = 1;
	seq_params.parameter_array[0] = exposure_time;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_multiframe_mode( MX_RECORD *record,
					long num_frames,
					double exposure_time,
					double frame_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_MULTIFRAME;
	seq_params.num_parameters = 3;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = frame_time;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_circular_multiframe_mode( MX_RECORD *record,
						long num_frames,
						double exposure_time,
						double frame_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_CIRCULAR_MULTIFRAME;
	seq_params.num_parameters = 3;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = frame_time;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_strobe_mode( MX_RECORD *record,
				long num_frames,
				double exposure_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_STROBE;
	seq_params.num_parameters = 2;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_bulb_mode( MX_RECORD *record,
				long num_frames )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_BULB;
	seq_params.num_parameters = 1;
	seq_params.parameter_array[0] = num_frames;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_set_geometrical_mode( MX_RECORD *record,
					long num_frames,
					double exposure_time,
					double frame_time,
					double exposure_multiplier,
					double gap_multiplier )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_GEOMETRICAL;
	seq_params.num_parameters = 5;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = frame_time;
	seq_params.parameter_array[3] = exposure_multiplier;
	seq_params.parameter_array[4] = gap_multiplier;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_streak_camera_mode( MX_RECORD *record,
					long num_lines,
					double exposure_time_per_line,
					double total_time_per_line )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_STREAK_CAMERA;
	seq_params.num_parameters = 3;
	seq_params.parameter_array[0] = num_lines;
	seq_params.parameter_array[1] = exposure_time_per_line;
	seq_params.parameter_array[2] = total_time_per_line;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_subimage_mode( MX_RECORD *record,
					long num_lines_per_subimage,
					long num_subimages,
					double exposure_time,
					double subimage_time,
					double exposure_multiplier,
					double gap_multiplier )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_SUBIMAGE;
	seq_params.num_parameters = 6;
	seq_params.parameter_array[0] = num_lines_per_subimage;
	seq_params.parameter_array[1] = num_subimages;
	seq_params.parameter_array[2] = exposure_time;
	seq_params.parameter_array[3] = subimage_time;
	seq_params.parameter_array[4] = exposure_multiplier;
	seq_params.parameter_array[5] = gap_multiplier;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_get_trigger_mode( MX_RECORD *record, long *trigger_mode )
{
	static const char fname[] = "mx_area_detector_get_trigger_mode()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_TRIGGER_MODE; 

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trigger_mode != (long *) NULL ) {
		*trigger_mode = ad->trigger_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_trigger_mode( MX_RECORD *record, long trigger_mode )
{
	static const char fname[] = "mx_area_detector_set_trigger_mode()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_TRIGGER_MODE; 

	ad->trigger_mode = trigger_mode;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_get_shutter_enable( MX_RECORD *record,
					mx_bool_type *shutter_enable )
{
	static const char fname[] = "mx_area_detector_get_shutter_enable()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_SHUTTER_ENABLE; 

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( shutter_enable != (mx_bool_type *) NULL ) {
		*shutter_enable = ad->shutter_enable;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_shutter_enable( MX_RECORD *record,
					mx_bool_type shutter_enable )
{
	static const char fname[] = "mx_area_detector_set_shutter_enable()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_SHUTTER_ENABLE; 

	ad->shutter_enable = shutter_enable;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_arm( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_arm()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *arm_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->stop = FALSE;
	ad->abort = FALSE;

	arm_fn = flist->arm;

	if ( arm_fn != NULL ) {
		mx_status = (*arm_fn)( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Compute image frame parameters for later use. */

	mx_status = mx_area_detector_get_image_format( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_framesize( ad->record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_pixel( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_bytes_per_frame( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: image_format = %ld, framesize = (%ld,%ld)",
		fname, ad->image_format, ad->framesize[0], ad->framesize[1]));

	MX_DEBUG(-2,("%s: bytes_per_pixel = %g, bytes_per_frame = %ld",
		fname, ad->bytes_per_pixel, ad->bytes_per_frame));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_trigger( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_trigger()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *trigger_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger_fn = flist->trigger;

	if ( trigger_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Triggering the taking of image frames has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

	mx_status = (*trigger_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_start( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_area_detector_arm( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_trigger( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_stop( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_stop()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *stop_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->stop = TRUE;

	stop_fn = flist->stop;

	if ( stop_fn != NULL ) {
		mx_status = (*stop_fn)( ad );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_abort( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_abort()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *abort_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->abort = TRUE;

	abort_fn = flist->abort;

	if ( abort_fn != NULL ) {
		mx_status = (*abort_fn)( ad );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_is_busy( MX_RECORD *record, mx_bool_type *busy )
{
	static const char fname[] = "mx_area_detector_is_busy()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	unsigned long status_flags;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_status( record, &status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy != NULL ) {
		if ( ad->status & MXSF_AD_IS_BUSY ) {
			*busy = TRUE;
		} else {
			*busy = FALSE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_maximum_frame_number( MX_RECORD *record,
					unsigned long *maximum_frame_number )
{
	static const char fname[] =
				"mx_area_detector_get_maximum_frame_number()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	ad->parameter_type = MXLV_AD_MAXIMUM_FRAME_NUMBER;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( maximum_frame_number != NULL ) {
		*maximum_frame_number = ad->maximum_frame_number;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_last_frame_number( MX_RECORD *record,
			long *last_frame_number )
{
	static const char fname[] = "mx_area_detector_get_last_frame_number()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_last_frame_number_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_extended_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_frame_number_fn = flist->get_last_frame_number;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_last_frame_number_fn != NULL ) {
		mx_status = (*get_last_frame_number_fn)( ad );
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the last frame number for area detector '%s' "
		"is unsupported.", record->name );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld",
		fname, ad->last_frame_number));
#endif

	if ( last_frame_number != NULL ) {
		*last_frame_number = ad->last_frame_number;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_total_num_frames( MX_RECORD *record,
			long *total_num_frames )
{
	static const char fname[] = "mx_area_detector_get_total_num_frames()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_total_num_frames_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_extended_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_total_num_frames_fn  = flist->get_total_num_frames;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_total_num_frames_fn != NULL ) {
		mx_status = (*get_total_num_frames_fn)( ad );
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the total number of frames for area detector '%s' "
		"is unsupported.", record->name );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: total_num_frames = %ld",
		fname, ad->total_num_frames));
#endif

	if ( total_num_frames != NULL ) {
		*total_num_frames = ad->total_num_frames;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_status( MX_RECORD *record,
			unsigned long *status_flags )
{
	static const char fname[] = "mx_area_detector_get_status()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_extended_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn            = flist->get_status;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_status_fn != NULL ) {
		mx_status = (*get_status_fn)( ad );
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the detector status for area detector '%s' "
		"is unsupported.", record->name );
	}

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: status = %#lx", fname, ad->status));
#endif

	if ( status_flags != NULL ) {
		*status_flags = ad->status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_extended_status( MX_RECORD *record,
			long *last_frame_number,
			long *total_num_frames,
			unsigned long *status_flags )
{
	static const char fname[] = "mx_area_detector_get_extended_status()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_last_frame_number_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_total_num_frames_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_extended_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_frame_number_fn = flist->get_last_frame_number;
	get_total_num_frames_fn  = flist->get_total_num_frames;
	get_status_fn            = flist->get_status;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );
	} else {
		if ( get_last_frame_number_fn == NULL ) {
			ad->last_frame_number = -1;
		} else {
			mx_status = (*get_last_frame_number_fn)( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		}

		if ( get_total_num_frames_fn == NULL ) {
			ad->total_num_frames = -1;
		} else {
			mx_status = (*get_total_num_frames_fn)( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( get_status_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the detector status for area detector '%s' "
			"is unsupported.", record->name );
		} else {
			mx_status = (*get_status_fn)( ad );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status));
#endif

	if ( last_frame_number != NULL ) {
		*last_frame_number = ad->last_frame_number;
	}
	if ( total_num_frames != NULL ) {
		*total_num_frames = ad->total_num_frames;
	}
	if ( status_flags != NULL ) {
		*status_flags = ad->status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_setup_frame( MX_RECORD *record,
				MX_IMAGE_FRAME **image_frame )
{
	static const char fname[] = "mx_area_detector_setup_frame()";

	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING setup_frame_timing;
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the frame is big enough. */

#if MX_AREA_DETECTOR_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,("%s: Invoking mx_image_alloc() for (*image_frame) = %p",
		fname, (*image_frame) ));
#endif

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_START(setup_frame_timing);
#endif

	mx_status = mx_image_alloc( image_frame,
					ad->framesize[0],
					ad->framesize[1],
					ad->image_format,
					ad->byte_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXIF_ROW_BINSIZE(*image_frame)    = ad->binsize[0];
	MXIF_COLUMN_BINSIZE(*image_frame) = ad->binsize[1];
	MXIF_BITS_PER_PIXEL(*image_frame) = ad->bits_per_pixel;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(setup_frame_timing);
	MX_HRT_RESULTS(setup_frame_timing, fname, "for frame setup.");
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_readout_frame( MX_RECORD *record, long frame_number )
{
	static const char fname[] = "mx_area_detector_readout_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *readout_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING readout_frame_timing;
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_START(readout_frame_timing);
#endif

	/* Does this driver implement a readout_frame function? */

	readout_frame_fn = flist->readout_frame;

	if ( readout_frame_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
    "Reading out an image frame for record '%s' has not yet been implemented.",
			record->name );
	}

	/* If the frame number is (-1L), then the caller is asking for
	 * the most recently acquired frame.
	 */

	if ( frame_number == (-1L) ) {
		/* Find out the frame number of the most recently 
		 * acquired frame.
		 */

		mx_status = mx_area_detector_get_last_frame_number( record,
								&frame_number );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If the frame number is _still_ (-1L), then no frames
		 * have been captured yet by the detector.
		 */

		if ( frame_number == (-1L) ) {
			return mx_error( MXE_TRY_AGAIN, fname,
		    "No frames have been captured yet by area detector '%s'.",
				record->name );
		}
	}

	ad->frame_number  = frame_number;
	ad->readout_frame = frame_number;

	mx_status = (*readout_frame_fn)( ad );

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(readout_frame_timing);
	MX_HRT_RESULTS(readout_frame_timing, fname, "for frame readout");
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_correct_frame( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_correct_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *correct_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	correct_frame_fn = flist->correct_frame;

	if ( correct_frame_fn == NULL ) {
		correct_frame_fn = mx_area_detector_default_correct_frame;
	}

	mx_status = (*correct_frame_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_transfer_frame( MX_RECORD *record,
				long frame_type,
				MX_IMAGE_FRAME *destination_frame )
{
	static const char fname[] = "mx_area_detector_transfer_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *transfer_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( destination_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination_frame pointer passed was NULL." );
	}

	ad->transfer_destination_frame = destination_frame;

	transfer_frame_fn = flist->transfer_frame;

	if ( transfer_frame_fn == NULL ) {
		transfer_frame_fn = mx_area_detector_default_transfer_frame;
	}

	ad->transfer_frame = frame_type;

	mx_status = (*transfer_frame_fn)( ad );

#if MX_AREA_DETECTOR_DEBUG
	{
		uint16_t *destination_data_array;
		long i;

		destination_data_array = destination_frame->image_data;

		for ( i = 0; i < 10; i++ ) {
			MX_DEBUG(-2,("%s: destination_data_array[%ld] = %d",
			fname, i, destination_data_array[i]));
		}
	}
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Some frame types need special things to happen after
	 * they are transferred.
	 */

	switch( frame_type ) {
	case MXFT_AD_BIAS_FRAME:
		if ( ad->bias_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->bias_frame, ad->mask_frame,
					&(ad->bias_average_intensity) );
		}

		mx_free( ad->dark_current_offset_array );
		mx_free( ad->flood_field_scale_array );
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_frame != NULL ) {
			mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );
		}

		mx_free( ad->dark_current_offset_array );
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		if ( ad->flood_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flood_field_frame, ad->mask_frame,
					&(ad->flood_field_average_intensity) );
		}

		mx_free( ad->flood_field_scale_array );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_load_frame( MX_RECORD *record,
				long frame_type,
				char *frame_filename )
{
	static const char fname[] = "mx_area_detector_load_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *load_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES
	MX_DEBUG(-2,("%s: Invoked for frame_type = %ld, frame_filename = '%s'",
		fname, frame_type, frame_filename));
#endif

#if MX_AREA_DETECTOR_DEBUG_FRAME_PARAMETERS
	MX_DEBUG(-2,("%s:   ad->maximum_framesize = [%ld,%ld]", fname,
		ad->maximum_framesize[0],
		ad->maximum_framesize[1]));
	MX_DEBUG(-2,("%s:   ad->framesize = [%ld,%ld]",
		fname, ad->framesize[0], ad->framesize[1]));
	MX_DEBUG(-2,("%s:   ad->binsize = [%ld,%ld]",
		fname, ad->binsize[0], ad->binsize[1]));
	MX_DEBUG(-2,("%s:   ad->image_format_name = '%s'",
		fname, ad->image_format_name));
	MX_DEBUG(-2,("%s:   ad->byte_order = %lu", fname, ad->byte_order));
	MX_DEBUG(-2,("%s:   ad->bits_per_pixel = %lu",
		fname, ad->bits_per_pixel));
	MX_DEBUG(-2,("%s:   ad->bytes_per_pixel = %g",
		fname, ad->bytes_per_pixel));
	MX_DEBUG(-2,("%s:   ad->bytes_per_frame = %lu",
		fname, ad->bytes_per_frame));
	MX_DEBUG(-2,("%s:   ad->trigger_mode = %lu", fname, ad->trigger_mode));
#endif

	/* Area detector image correction is currently only supported
	 * for 16-bit greyscale images (MXT_IMAGE_FORMAT_GREY16).
	 */

	if ( ad->image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Area detector '%s' is not configured for a 16-bit greyscale image.",
			record->name );
	}

	load_frame_fn = flist->load_frame;

	if ( load_frame_fn == NULL ) {
		load_frame_fn = mx_area_detector_default_load_frame;
	}

	ad->load_frame = frame_type & MXFT_AD_ALL;

	strlcpy( ad->frame_filename, frame_filename, MXU_FILENAME_LENGTH );

	mx_status = (*load_frame_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* MX network clients do not need to do anything further here,
	 * since the server takes care of all the work.
	 */

	if ( record->mx_type == MXT_AD_NETWORK ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Additional things must be done for image correction frames. */

	switch( frame_type ) {
	case MXFT_AD_IMAGE_FRAME:
		/* Nothing to do for primary image frames. */

		break;

	case MXFT_AD_MASK_FRAME:
		if ( ad->rebinned_mask_frame != NULL ) {
			mx_image_free( ad->rebinned_mask_frame );

			ad->rebinned_mask_frame = NULL;
		}
		strlcpy( ad->mask_filename, frame_filename,
					sizeof(ad->mask_filename) );
		break;

	case MXFT_AD_BIAS_FRAME:
		if ( ad->bias_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->bias_frame, ad->mask_frame,
					&(ad->bias_average_intensity) );
		}
		if ( ad->rebinned_bias_frame != NULL ) {
			mx_image_free( ad->rebinned_bias_frame );

			ad->rebinned_bias_frame = NULL;
		}
		strlcpy( ad->bias_filename, frame_filename,
					sizeof(ad->bias_filename) );

		mx_free( ad->dark_current_offset_array );
		mx_free( ad->flood_field_scale_array );
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_frame != NULL ) {
			mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );
		}
		if ( ad->rebinned_dark_current_frame != NULL ) {
			mx_image_free( ad->rebinned_dark_current_frame );

			ad->rebinned_dark_current_frame = NULL;
		}
		strlcpy( ad->dark_current_filename, frame_filename,
					sizeof(ad->dark_current_filename) );

		mx_free( ad->dark_current_offset_array );
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		if ( ad->flood_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flood_field_frame, ad->mask_frame,
					&(ad->flood_field_average_intensity) );
		}
		if ( ad->rebinned_flood_field_frame != NULL ) {
			mx_image_free( ad->rebinned_flood_field_frame );

			ad->rebinned_flood_field_frame = NULL;
		}
		strlcpy( ad->flood_field_filename, frame_filename,
					sizeof(ad->flood_field_filename) );

		mx_free( ad->flood_field_scale_array );
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported frame type %lu requested for area detector '%s'",
			frame_type, record->name );
		break;
	}

#if MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_save_frame( MX_RECORD *record,
				long frame_type,
				char *frame_filename )
{
	static const char fname[] = "mx_area_detector_save_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *save_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES
	MX_DEBUG(-2,("%s: Invoked for frame_type = %ld, frame_filename = '%s'",
		fname, frame_type, frame_filename));
#endif

	save_frame_fn = flist->save_frame;

	if ( save_frame_fn == NULL ) {
		save_frame_fn = mx_area_detector_default_save_frame;
	}

	ad->save_frame = frame_type & MXFT_AD_ALL;

	strlcpy( ad->frame_filename, frame_filename, MXU_FILENAME_LENGTH );

	mx_status = (*save_frame_fn)( ad );

	/* Additional things must be done for image correction frames. */

	switch( frame_type ) {
	case MXFT_AD_IMAGE_FRAME:
		/* Nothing to do for primary image frames. */

		break;

	case MXFT_AD_MASK_FRAME:
		strlcpy( ad->mask_filename, frame_filename,
					sizeof(ad->mask_filename) );
		break;

	case MXFT_AD_BIAS_FRAME:
		strlcpy( ad->bias_filename, frame_filename,
					sizeof(ad->bias_filename) );
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		strlcpy( ad->dark_current_filename, frame_filename,
					sizeof(ad->dark_current_filename) );
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		strlcpy( ad->flood_field_filename, frame_filename,
					sizeof(ad->flood_field_filename) );
		break;

	case MXFT_AD_REBINNED_MASK_FRAME:
	case MXFT_AD_REBINNED_BIAS_FRAME:
	case MXFT_AD_REBINNED_DARK_CURRENT_FRAME:
	case MXFT_AD_REBINNED_FLOOD_FIELD_FRAME:
		/* Nothing to do for the rebinned frames. */

		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported frame type %lu requested for area detector '%s'",
			frame_type, record->name );
		break;
	}

#if MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_copy_frame( MX_RECORD *record,
				long source_frame_type,
				long destination_frame_type )
{
	static const char fname[] = "mx_area_detector_copy_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	char *source_filename, *destination_filename;
	mx_status_type ( *copy_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	char empty_filename[5] = "";

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES
	MX_DEBUG(-2,("%s: Invoked for source = %#lx, destination = %#lx",
		fname, source_frame_type, destination_frame_type));
#endif

	switch( source_frame_type ) {
	case MXFT_AD_IMAGE_FRAME:
		source_filename = empty_filename;
		break;
	case MXFT_AD_MASK_FRAME:
		source_filename = ad->mask_filename;
		break;
	case MXFT_AD_BIAS_FRAME:
		source_filename = ad->bias_filename;
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		source_filename = ad->dark_current_filename;
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		source_filename = ad->flood_field_filename;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported source frame type %lu requested "
		"for area detector '%s'",
			source_frame_type, record->name );
		break;
	}

	copy_frame_fn = flist->copy_frame;

	if ( copy_frame_fn == NULL ) {
		copy_frame_fn = mx_area_detector_default_copy_frame;
	}

	ad->copy_frame[0] = source_frame_type & MXFT_AD_ALL;

	ad->copy_frame[1] = destination_frame_type & MXFT_AD_ALL;

	mx_status = (*copy_frame_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Additional things must be done for image correction frames. */

	switch( destination_frame_type ) {
	case MXFT_AD_IMAGE_FRAME:
		destination_filename = NULL;
		break;

	case MXFT_AD_MASK_FRAME:
		if ( ad->rebinned_mask_frame != NULL ) {
			mx_image_free( ad->rebinned_mask_frame );

			ad->rebinned_mask_frame = NULL;
		}
		destination_filename = ad->mask_filename;
		break;

	case MXFT_AD_BIAS_FRAME:
		if ( ad->bias_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->bias_frame, ad->mask_frame,
					&(ad->bias_average_intensity) );
		}
		if ( ad->rebinned_bias_frame != NULL ) {
			mx_image_free( ad->rebinned_bias_frame );

			ad->rebinned_bias_frame = NULL;
		}
		destination_filename = ad->bias_filename;

		mx_free( ad->dark_current_offset_array );
		mx_free( ad->flood_field_scale_array );
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_frame != NULL ) {
			mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );
		}
		if ( ad->rebinned_dark_current_frame != NULL ) {
			mx_image_free( ad->rebinned_dark_current_frame );

			ad->rebinned_dark_current_frame = NULL;
		}
		destination_filename = ad->dark_current_filename;

		mx_free( ad->dark_current_offset_array );
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		if ( ad->flood_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flood_field_frame, ad->mask_frame,
					&(ad->flood_field_average_intensity) );
		}
		if ( ad->rebinned_flood_field_frame != NULL ) {
			mx_image_free( ad->rebinned_flood_field_frame );

			ad->rebinned_flood_field_frame = NULL;
		}
		destination_filename = ad->flood_field_filename;

		mx_free( ad->flood_field_scale_array );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported destination frame type %lu requested "
		"for area detector '%s'",
			destination_frame_type, record->name );
		break;
	}

	if ( ( source_filename != NULL )
	  && ( destination_filename != NULL ) )
	{
		strlcpy( destination_filename, source_filename,
				MXU_FILENAME_LENGTH );
	}

#if MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}


/*---*/

MX_EXPORT mx_status_type
mx_area_detector_get_frame( MX_RECORD *record,
			long frame_number,
			MX_IMAGE_FRAME **image_frame )
{
	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

	mx_status = mx_area_detector_setup_frame( record, image_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_readout_frame( record, frame_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad = record->record_class_struct;

	mx_status = mx_area_detector_correct_frame( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_transfer_frame( record,
				MXFT_AD_IMAGE_FRAME, ad->image_frame );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_sequence( MX_RECORD *record,
			long num_frames,
			MX_IMAGE_SEQUENCE **sequence )
{
	static const char fname[] = "mx_area_detector_get_sequence()";

	long i, old_num_frames;
	mx_status_type mx_status;

	if ( sequence == (MX_IMAGE_SEQUENCE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_SEQUENCE pointer passed was NULL." );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: *sequence = %p", fname, *sequence));
#endif

	/* We either reuse an existing MX_IMAGE_SEQUENCE or create a new one. */

	if ( (*sequence) == (MX_IMAGE_SEQUENCE *) NULL ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Allocating a new MX_IMAGE_SEQUENCE.", fname));
#endif
		/* Allocate a new MX_IMAGE_SEQUENCE. */

		*sequence = malloc( sizeof(MX_IMAGE_SEQUENCE) );

		if ( (*sequence) == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a new "
			"MX_IMAGE_SEQUENCE structure." );
		}

		(*sequence)->num_frames  = 0;
		(*sequence)->frame_array = NULL;

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: allocated new sequence.", fname));
#endif
	}

	/* See if the frame array already has enough entries. */

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: (*sequence)->num_frames = %ld, (*sequence)->frame_array = %p",
		fname, (*sequence)->num_frames, (*sequence)->frame_array));
#endif

	old_num_frames = (*sequence)->num_frames;

	if ( (*sequence)->frame_array == NULL ) {
		(*sequence)->frame_array =
			malloc( num_frames * sizeof(MX_IMAGE_FRAME *) );

		if ( (*sequence)->frame_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate a %ld element "
			"array of MX_IMAGE_FRAME structures.", num_frames );
		}

		(*sequence)->num_frames = num_frames;

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Allocated a new frame array with %ld elements.",
			fname, num_frames));
#endif
	} else
	if ( num_frames > old_num_frames ) {
		(*sequence)->frame_array =
				realloc( (*sequence)->frame_array,
					num_frames * sizeof(MX_IMAGE_FRAME *) );

		if ( (*sequence)->frame_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to increase the number of "
			"MX_IMAGE_FRAMEs in MX_IMAGE_SEQUENCE %p "
			"from %ld to %ld.",  *sequence,
				old_num_frames, num_frames );
		}

		(*sequence)->num_frames = num_frames;

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Increased the size of the frame array "
			"from %ld elements to %ld elements.",
			fname, old_num_frames, num_frames));
#endif
	}

	/* NULL out the pointers to any frame array members that did not
	 * already exist.
	 */

	for ( i = old_num_frames; i < num_frames; i++ ) {
		(*sequence)->frame_array[i] = NULL;
	}

	/* Update the contents of all of the image frames. */

	for ( i = 0; i < num_frames; i++ ) {
		mx_status = mx_area_detector_get_frame( record, i,
					&( (*sequence)->frame_array[i] ) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_roi_frame( MX_RECORD *record,
			MX_IMAGE_FRAME *image_frame,
			unsigned long roi_number,
			MX_IMAGE_FRAME **roi_frame )
{
	static const char fname[] = "mx_area_detector_get_roi_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_roi_frame_fn ) ( MX_AREA_DETECTOR * );
	long roi_row_width, roi_column_height, image_format, byte_order;
	long row_width, column_height;
	long roi_row, roi_column, row_offset, column_offset;
	long dimension[2];
	size_t element_size[2];
	uint16_t **image_array_u16, **roi_array_u16;
	void *array_ptr;
	double bytes_per_pixel;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the record has a 'get_roi_frame' function. */

	get_roi_frame_fn = flist->get_roi_frame;

	/* If no image frame pointer was supplied as a function argument,
	 * then figure out what to do about this.
	 */

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {

		if ( get_roi_frame_fn != NULL ) {
			
			/* If the driver for the record has a 'get_roi_frame'
			 * function, then we assume that the driver function
			 * will take care of fetching the ROI data for us.
			 */

		} else {
			if ( ad->image_frame != (MX_IMAGE_FRAME *) NULL ) {

				/* If the area detector record already has an
				 * allocated image frame, then we will use that
				 * frame as the source of the data.
				 */

				image_frame = ad->image_frame;
			} else {
				/* If we get here, then there is no identified
				 * place to get an image frame from and we
				 * must return an error.
				 */

				return mx_error(
					MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
				"No method has been found for finding the ROI "
				"data for record '%s'.  If you pass a non-NULL "
				"image frame pointer or read a full image "
				"frame, then this problem will go away.",
						record->name );
			}
		}
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: image_frame = %p, *roi_frame = %p",
		fname, image_frame, *roi_frame));
#endif

	/* We either reuse an existing MX_IMAGE_FRAME or create a new one. */

	if ( (*roi_frame) == (MX_IMAGE_FRAME *) NULL ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new ROI MX_IMAGE_FRAME.",fname));
#endif
		/* Allocate a new MX_IMAGE_FRAME. */

		*roi_frame = malloc( sizeof(MX_IMAGE_FRAME) );

		if ( (*roi_frame) == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a new ROI MX_IMAGE_FRAME structure." );
		}

		(*roi_frame)->header_length = 0;
		(*roi_frame)->header_data = NULL;

		(*roi_frame)->image_length = 0;
		(*roi_frame)->image_data = NULL;
	}

	/* Fill in some parameters. */

	roi_row_width     = ad->roi[1] - ad->roi[0] + 1;
	roi_column_height = ad->roi[3] - ad->roi[2] + 1;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		image_format    = ad->image_format;
		byte_order      = ad->byte_order;
		bytes_per_pixel = ad->bytes_per_pixel;
	} else {
		image_format    = MXIF_IMAGE_FORMAT(image_frame);
		byte_order      = MXIF_BYTE_ORDER(image_frame);
		bytes_per_pixel = MXIF_BYTES_PER_PIXEL(image_frame);
	}

	/* Make sure that the frame is big enough to hold the image data. */

#if MX_AREA_DETECTOR_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,("%s: Invoking mx_image_alloc() for (*roi_frame) = %p",
		fname, (*roi_frame) ));
#endif

	mx_status = mx_image_alloc( roi_frame,
				roi_row_width,
				roi_column_height,
				image_format,
				byte_order,
				bytes_per_pixel,
				0, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Fill in some more parameters. */

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
	    time_t time_buffer;

	    MXIF_ROW_BINSIZE(*roi_frame)        = 1;
	    MXIF_COLUMN_BINSIZE(*roi_frame)     = 1;
	    MXIF_BITS_PER_PIXEL(*roi_frame)     = ad->bits_per_pixel;
	    MXIF_EXPOSURE_TIME_SEC(*roi_frame)  = 0;
	    MXIF_EXPOSURE_TIME_NSEC(*roi_frame) = 0;
		
	    /* Set the timestamp to the current time. */

	    MXIF_TIMESTAMP_SEC(*roi_frame) = time( &time_buffer );
	    MXIF_TIMESTAMP_NSEC(*roi_frame) = 0;
	} else {
	    MXIF_ROW_BINSIZE(*roi_frame)     = MXIF_ROW_BINSIZE(image_frame);
	    MXIF_COLUMN_BINSIZE(*roi_frame)  = MXIF_COLUMN_BINSIZE(image_frame);
	    MXIF_BITS_PER_PIXEL(*roi_frame)  = MXIF_BITS_PER_PIXEL(image_frame);
	    MXIF_EXPOSURE_TIME_SEC(*roi_frame)
					= MXIF_EXPOSURE_TIME_SEC(image_frame);
	    MXIF_EXPOSURE_TIME_NSEC(*roi_frame)
					= MXIF_EXPOSURE_TIME_NSEC(image_frame);
	    MXIF_TIMESTAMP_SEC(*roi_frame) = MXIF_TIMESTAMP_SEC(image_frame);
	    MXIF_TIMESTAMP_NSEC(*roi_frame) = MXIF_TIMESTAMP_NSEC(image_frame);
	}

	ad->roi_bytes_per_frame = (*roi_frame)->image_length;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: ROI image_length = %ld, ad->roi_bytes_per_frame = %ld",
	fname, (long) (*roi_frame)->image_length, ad->roi_bytes_per_frame));
#endif

	ad->roi_number = roi_number;

	ad->roi_frame = *roi_frame;

	/* If the driver has a get_roi_frame method, invoke it. */

	if ( get_roi_frame_fn != NULL ) {
		mx_status = (*get_roi_frame_fn)( ad );
	} else {
		/***********************************************
		 * Otherwise, we do the ROI data copy ourself. *
		 ***********************************************/

		if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {

			/* It should be impossible to get here with an
			 * image_frame pointer equal to NULL, but we check
			 * for this anyway in the interest of paranoia.
			 */

			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The image_frame pointer is NULL for "
				"area detector '%s', although apparently "
				"it was _not_ NULL earlier in this function.",
					record->name );
		}

		row_width     = MXIF_ROW_FRAMESIZE(image_frame);
		column_height = MXIF_COLUMN_FRAMESIZE(image_frame);

		switch( ad->image_format ) {
		case MXT_IMAGE_FORMAT_GREY16:

			/* Overlay the image data buffer with
			 * a two dimensional array.
			 */

			dimension[0] = column_height;
			dimension[1] = row_width;

			element_size[0] = sizeof(uint16_t);
			element_size[1] = sizeof(uint16_t *);

			mx_status = mx_array_add_overlay(
						image_frame->image_data,
						2, dimension, element_size,
						&array_ptr );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			image_array_u16 = array_ptr;

			/* Overlay the ROI frame's data buffer with
			 * another two dimensional array.
			 */

			dimension[0] = roi_column_height;
			dimension[1] = roi_row_width;

			element_size[0] = sizeof(uint16_t);
			element_size[1] = sizeof(uint16_t *);

			mx_status = mx_array_add_overlay(
						(*roi_frame)->image_data,
						2, dimension, element_size,
						&array_ptr );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_array_free_overlay( image_array_u16, 2 );
				return mx_status;
			}

			roi_array_u16 = array_ptr;

			/* Copy the contents of the ROI to the ROI frame. */

			for ( roi_row = 0;
			    roi_row < roi_column_height;
			    roi_row++ ) 
			{
				row_offset = ad->roi[2];

				for ( roi_column = 0;
				    roi_column < roi_row_width;
				    roi_column++ )
				{
					column_offset = ad->roi[0];

					roi_array_u16[ roi_row ][ roi_column ]
	= image_array_u16[ roi_row + row_offset ][ roi_column + column_offset ];
				}
			}

			mx_array_free_overlay( image_array_u16, 2 );
			mx_array_free_overlay( roi_array_u16, 2 );
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' does not support image_format %ld.",
				ad->record->name, ad->image_format );
			break;
		}
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_get_correction_frame( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					unsigned long frame_type,
					char *frame_name,
					MX_IMAGE_FRAME **correction_frame )
{
	static const char fname[] = "mx_area_detector_get_correction_frame()";

	MX_IMAGE_FRAME **rebinned_frame;
	unsigned long image_width, image_height;
	unsigned long correction_width, correction_height;
	unsigned long rebinned_width, rebinned_height;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_TIMING rebin_timing;
#endif

	/* Suppress stupid GCC uninitialized variable warning. */

	rebinned_frame = NULL;

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("\n----------------------------------------------------"));
	MX_DEBUG(-2,
	("%s invoked, image_frame = %p, frame_type = %lu, frame_name = '%s'",
		fname, image_frame, frame_type, frame_name));
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AREA_DETECTOR pointer passed was NULL." );
	}
	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The image_frame pointer passed was NULL." );
	}
	if ( correction_frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The correction_frame pointer passed was NULL." );
	}

	switch( frame_type ) {
	case MXFT_AD_MASK_FRAME:
		*correction_frame = ad->mask_frame;
		break;

	case MXFT_AD_BIAS_FRAME:
		*correction_frame = ad->bias_frame;
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		*correction_frame = ad->dark_current_frame;
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		*correction_frame = ad->flood_field_frame;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image frame type %lu is not supported for area detector '%s'.",
			frame_type, ad->record->name );
		break;
	}

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("%s: initial (*correction_frame) = %p",
		fname, *correction_frame));
#endif

	if ( *correction_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No %s frame has been loaded for area detector '%s'.",
			frame_name, ad->record->name );
	}

	image_width  = MXIF_ROW_FRAMESIZE(image_frame);
	image_height = MXIF_COLUMN_FRAMESIZE(image_frame);

	correction_width  = MXIF_ROW_FRAMESIZE(*correction_frame);
	correction_height = MXIF_COLUMN_FRAMESIZE(*correction_frame);

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("%s: image_width = %lu, image_height = %lu",
		fname, image_width, image_height));

	MX_DEBUG(-2,("%s: correction_width = %lu, correction_height = %lu",
		fname, correction_width, correction_height));
#endif

	/* If the image frame and the default correction frame have
	 * the same dimensions, then we can return the correction frame
	 * as is.
	 */

	if ( (image_width == correction_width)
	  && (image_height == correction_height) )
	{

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
		MX_DEBUG(-2,
		("%s: Returning since the image sizes match.", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we check to see if the correction frame dimensions
	 * are an integer multiple of the image frame dimensions.
	 */

	if ( ((correction_width % image_width) != 0)
	  || ((correction_height % image_height) != 0) )
	{
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The dimensions of the %s frame (%lu,%lu) are not an integer "
		"multiple of the dimensions of the image frame (%lu,%lu) for "
		"area detector '%s'.  The %s frame cannot be used for image "
		"correction as long as this is the case.",
			frame_name, correction_width, correction_height,
			image_width, image_height, ad->record->name,
			frame_name );
	}

	/* Do we already have a rebinned correction frame with the right
	 * dimensions?  If so, we can reuse that frame.
	 */

	switch( frame_type ) {
	case MXFT_AD_MASK_FRAME:
		rebinned_frame = &(ad->rebinned_mask_frame);
		break;

	case MXFT_AD_BIAS_FRAME:
		rebinned_frame = &(ad->rebinned_bias_frame);
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		rebinned_frame = &(ad->rebinned_dark_current_frame);
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		rebinned_frame = &(ad->rebinned_flood_field_frame);
		break;
	}

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,("%s: initial (*rebinned_frame) = %p",
		fname, *rebinned_frame));
#endif

	if ( *rebinned_frame != (MX_IMAGE_FRAME *) NULL ) {

		/* See if we can reuse the rebinned frame. */

		rebinned_width = MXIF_ROW_FRAMESIZE(*rebinned_frame);
		rebinned_height = MXIF_COLUMN_FRAMESIZE(*rebinned_frame);

		if ( (rebinned_width == image_width)
		  && (rebinned_height == image_height) )
		{

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
			MX_DEBUG(-2,
		    ("%s: Returning since we are reusing the rebinned frame.",
		    		fname));
#endif
			/* Yes, we can reuse the existing rebinned frame. */

			*correction_frame = *rebinned_frame;

			return MX_SUCCESSFUL_RESULT;
		}

		/* No, we cannot reuse the existing rebinned frame as is. */
	}

	/* Rebin the correction frame using 'rebinned_frame'
	 * as the destination.
	 */

#if MX_AREA_DETECTOR_DEBUG_GET_CORRECTION_FRAME
	MX_DEBUG(-2,
  ("%s: Rebinning the correction frame.  Width scale = %lu, Height scale = %lu",
  		fname, correction_width / image_width,
		correction_height / image_height));
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( rebin_timing );
#endif
	mx_status = mx_image_rebin( rebinned_frame, *correction_frame,
					image_width, image_height );

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( rebin_timing );
	MX_HRT_RESULTS( rebin_timing, fname, "Rebinning correction image." );
#endif

	if ( mx_status.code != MXE_SUCCESS ) 
		return mx_status;

	/* Return the rebinned frame. */

	*correction_frame = *rebinned_frame;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_correct_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_correct_frame()";

	MX_IMAGE_FRAME *image_frame, *mask_frame, *bias_frame;
	MX_IMAGE_FRAME *dark_current_frame, *flood_field_frame;
	unsigned long flags;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	image_frame = ad->image_frame;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
	    "Area detector '%s' has not yet read out its first image frame.",
			ad->record->name );
	}

	/* Find the frame pointers for the image frames to be used. */

	flags = ad->correction_flags;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', correction_flags = %#lx",
		fname, ad->record->name, flags));
#endif

	if ( ( ad->correction_flags & MXFT_AD_MASK_FRAME ) == 0 ) {
		mask_frame = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
							ad, image_frame,
							MXFT_AD_MASK_FRAME,
							"mask",
							&mask_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( ( ad->correction_flags & MXFT_AD_BIAS_FRAME ) == 0 ) {
		bias_frame = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
							ad, image_frame,
							MXFT_AD_BIAS_FRAME,
							"bias",
							&bias_frame );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( ( ad->correction_flags & MXFT_AD_DARK_CURRENT_FRAME ) == 0 ) {
		dark_current_frame = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
						ad, image_frame,
						MXFT_AD_DARK_CURRENT_FRAME,
						"dark current",
						&dark_current_frame );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( ( ad->correction_flags & MXFT_AD_FLOOD_FIELD_FRAME ) == 0 ) {
		flood_field_frame = NULL;
	} else {
		mx_status = mx_area_detector_get_correction_frame(
						ad, image_frame,
						MXFT_AD_FLOOD_FIELD_FRAME,
						"flood field",
						&flood_field_frame );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_area_detector_frame_correction( ad->record,
				image_frame, mask_frame, bias_frame,
				dark_current_frame, flood_field_frame );

#if MX_AREA_DETECTOR_DEBUG
	{
		uint16_t *image_data_array;
		long i;

		image_data_array = image_frame->image_data;

		for ( i = 0; i < 10; i++ ) {
			MX_DEBUG(-2,("%s: image_data_array[%ld] = %d",
				fname, i, image_data_array[i]));
		}
	}
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_default_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_transfer_frame()";

	MX_IMAGE_FRAME *destination_frame, *source_frame;
	mx_status_type mx_status;

	destination_frame = ad->transfer_destination_frame;

	switch( ad->transfer_frame ) {
	case MXFT_AD_IMAGE_FRAME:
		source_frame = ad->image_frame;
		break;
	case MXFT_AD_MASK_FRAME:
		source_frame = ad->mask_frame;
		break;
	case MXFT_AD_BIAS_FRAME:
		source_frame = ad->bias_frame;
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		source_frame = ad->dark_current_frame;
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		source_frame = ad->flood_field_frame;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Frame type %lx is not supported for frame transfer.",
				ad->transfer_frame );
	}

	if ( source_frame == NULL ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
"An image frame of type %#lx has not been configured for area detector '%s'.",
			ad->transfer_frame, ad->record->name );
	}

	if ( destination_frame == source_frame ) {
		/* If the destination and the source are the same,
		 * then we do not need to do any copying.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_image_copy_frame( source_frame, &destination_frame );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_default_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_load_frame()";

	MX_IMAGE_FRAME **frame_ptr;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s'.", fname, ad->record->name ));

	MX_DEBUG(-2,("%s: load_frame = %#lx, frame_filename = '%s'",
		fname, ad->load_frame, ad->frame_filename));
#endif

	switch( ad->load_frame ) {
	case MXFT_AD_IMAGE_FRAME:
		frame_ptr = &(ad->image_frame);
		break;
	case MXFT_AD_MASK_FRAME:
		frame_ptr = &(ad->mask_frame);
		break;
	case MXFT_AD_BIAS_FRAME:
		frame_ptr = &(ad->bias_frame);
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		frame_ptr = &(ad->dark_current_frame);
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		frame_ptr = &(ad->flood_field_frame);
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported frame type %ld requested for area detector '%s'.",
			ad->load_frame, ad->record->name );
	}

#if MX_AREA_DETECTOR_DEBUG_MX_IMAGE_ALLOC
	MX_DEBUG(-2,("%s: Invoking mx_image_alloc() for frame_ptr = %p",
		fname, frame_ptr));
#endif

	mx_status = mx_image_alloc( frame_ptr,
					ad->framesize[0],
					ad->framesize[1],
					ad->image_format,
					ad->byte_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_image_read_file( frame_ptr,
					ad->frame_file_format,
					ad->frame_filename );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does the frame we just loaded have the expected image format? */

	if ( MXIF_IMAGE_FORMAT(*frame_ptr) != ad->image_format ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The image format %ld for file '%s' does not match "
		"the expected image format %ld for area detector '%s'.",
			(long) MXIF_IMAGE_FORMAT(*frame_ptr),
			ad->frame_filename, ad->image_format,
			ad->record->name );
	}

	/* Does the frame we just loaded have the expected image dimensions? */

	if ( ( MXIF_ROW_FRAMESIZE(*frame_ptr)    != ad->framesize[0] )
	  || ( MXIF_COLUMN_FRAMESIZE(*frame_ptr) != ad->framesize[1] ) )
	{
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The dimensions (%ld,%ld) of image file '%s' do not match the "
		"expected image dimensions (%ld,%ld) for area detector '%s.'",
			(long) MXIF_ROW_FRAMESIZE(*frame_ptr),
			(long) MXIF_COLUMN_FRAMESIZE(*frame_ptr),
			ad->frame_filename,
			ad->framesize[0], ad->framesize[1],
			ad->record->name );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: frame file '%s' successfully loaded to %#lx",
		fname, ad->frame_filename, ad->load_frame));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_save_frame()";

	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s'.", fname, ad->record->name ));

	MX_DEBUG(-2,("%s: save_frame = %ld, frame_filename = '%s'",
		fname, ad->save_frame, ad->frame_filename));
#endif

	switch( ad->save_frame ) {
	case MXFT_AD_IMAGE_FRAME:
		frame = ad->image_frame;
		break;
	case MXFT_AD_MASK_FRAME:
		frame = ad->mask_frame;
		break;
	case MXFT_AD_BIAS_FRAME:
		frame = ad->bias_frame;
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		frame = ad->dark_current_frame;
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		frame = ad->flood_field_frame;
		break;

	case MXFT_AD_REBINNED_MASK_FRAME:
		frame = ad->rebinned_mask_frame;
		break;
	case MXFT_AD_REBINNED_BIAS_FRAME:
		frame = ad->rebinned_bias_frame;
		break;
	case MXFT_AD_REBINNED_DARK_CURRENT_FRAME:
		frame = ad->rebinned_dark_current_frame;
		break;
	case MXFT_AD_REBINNED_FLOOD_FIELD_FRAME:
		frame = ad->rebinned_flood_field_frame;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported frame type %ld requested for area detector '%s'.",
			ad->save_frame, ad->record->name );
	}

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"Image frame type %ld has not yet been initialized "
		"for area detector '%s'.", ad->save_frame, ad->record->name );
	}

	mx_status = mx_image_write_file( frame,
					ad->frame_file_format,
					ad->frame_filename );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: frame %#lx successfully saved to frame file '%s'.",
		fname, ad->save_frame, ad->frame_filename));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_copy_frame()";

	MX_IMAGE_FRAME *src_frame;
	MX_IMAGE_FRAME** dest_frame_ptr;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s invoked for area detector '%s'.", fname, ad->record->name ));

	MX_DEBUG(-2,("%s: copy_frame = (%ld,%ld), frame_filename = '%s'",
	    fname, ad->copy_frame[0], ad->copy_frame[1], ad->frame_filename));
#endif

	switch( ad->copy_frame[0] ) {
	case MXFT_AD_IMAGE_FRAME:
		src_frame = ad->image_frame;
		break;
	case MXFT_AD_MASK_FRAME:
		src_frame = ad->mask_frame;
		break;
	case MXFT_AD_BIAS_FRAME:
		src_frame = ad->bias_frame;
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		src_frame = ad->dark_current_frame;
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		src_frame = ad->flood_field_frame;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
	"Unsupported source frame type %ld requested for area detector '%s'.",
			ad->copy_frame[0], ad->record->name );
	}

	if ( src_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"Source image frame type %ld has not yet been initialized "
		"for area detector '%s'.",ad->copy_frame[0], ad->record->name );
	}

	switch( ad->copy_frame[1] ) {
	case MXFT_AD_IMAGE_FRAME:
		dest_frame_ptr = &(ad->image_frame);
		break;
	case MXFT_AD_MASK_FRAME:
		dest_frame_ptr = &(ad->mask_frame);
		break;
	case MXFT_AD_BIAS_FRAME:
		dest_frame_ptr = &(ad->bias_frame);
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		dest_frame_ptr = &(ad->dark_current_frame);
		break;
	case MXFT_AD_FLOOD_FIELD_FRAME:
		dest_frame_ptr = &(ad->flood_field_frame);
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Unsupported destination frame type %ld requested for area detector '%s'.",
			ad->copy_frame[1], ad->record->name );
	}

	mx_status = mx_image_copy_frame( src_frame, dest_frame_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: frame %#lx successfully copied to frame %#lx.",
		fname, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

static mx_status_type
mxp_area_detector_find_register_field( MX_RECORD *record,
						char *register_name,
						MX_RECORD_FIELD **field )
{
	static const char fname[] = "mxp_area_detector_find_register_field()";

	mx_status_type mx_status;

	mx_status = mx_find_record_field( record, register_name, field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does the field have a datatype that is compatible with
	 * this function?
	 */

	switch( (*field)->datatype ) {
	case MXFT_STRING:
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Parameter '%s' has data type '%s' which is incompatible "
		"with this function.", register_name,
			mx_get_field_type_string( (*field)->datatype ) );
	}

	/* Only 0-dimensional fields and 1-dimensional fields of length 1
	 * are supported here.
	 */

	if ( (*field)->num_dimensions == 0 ) {
		/* Supported */
	} else
	if ( (*field)->num_dimensions == 1 ) {
		if ( (*field)->dimension[0] == 1 ) {
			/* Supported */
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
			"1-dimensional field '%s' has length %ld which "
			"is longer than the maximum allowed length of 1.",
				(*field)->name, (*field)->dimension[0] );
		}
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"%ld-dimensional field '%s' is not supported.  "
		"Only 0-dimensional and 1-dimensional fields are supported.",
			(*field)->num_dimensions, register_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_get_register( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_get_register()";

	MX_RECORD_FIELD *field;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	void *value_ptr;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(ad->record,
						NULL, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	mx_status = mxp_area_detector_find_register_field( ad->record,
					ad->register_name, &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_ptr = mx_get_field_value_pointer( field );

	/* Read the register value from the detector. */

	ad->parameter_type = field->label_value;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Convert the value returned to a long integer. */

	switch( field->datatype ){
	case MXFT_CHAR:
		ad->register_value = *((char *) value_ptr);
		break;
	case MXFT_UCHAR:
		ad->register_value = *((unsigned char *) value_ptr);
		break;
	case MXFT_SHORT:
		ad->register_value = *((short *) value_ptr);
		break;
	case MXFT_USHORT:
		ad->register_value = *((unsigned short *) value_ptr);
		break;
	case MXFT_BOOL:
		ad->register_value = *((mx_bool_type *) value_ptr);
		break;
	case MXFT_LONG:
		ad->register_value = *((long *) value_ptr);
		break;
	case MXFT_ULONG:
	case MXFT_HEX:
		ad->register_value = *((unsigned long *) value_ptr);
		break;
	case MXFT_INT64:
		ad->register_value = *((int64_t *) value_ptr);
		break;
	case MXFT_UINT64:
		ad->register_value = *((uint64_t *) value_ptr);
		break;
	case MXFT_FLOAT:
		ad->register_value = mx_round( *((float *) value_ptr) );
		break;
	case MXFT_DOUBLE:
		ad->register_value = mx_round( *((double *) value_ptr) );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Unsupported data type %ld requested for parameter '%s'.",
			field->datatype, ad->register_name );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: register_name = '%s', register_value = %ld",
		fname, ad->register_name, ad->register_value ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_set_register( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_set_register()";

	MX_RECORD_FIELD *field;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	void *value_ptr;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( ad->record,
						NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	mx_status = mxp_area_detector_find_register_field( ad->record,
					ad->register_name, &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: register_name = '%s'",
			fname, register_name ));
#endif
	/* We must copy the register value to the field. */

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: register_name = '%s', register_value = %ld",
		fname, register_name, *register ));
#endif

	value_ptr = mx_get_field_value_pointer( field );

	switch( field->datatype ){
	case MXFT_CHAR:
		*((char *) value_ptr) = (char) ad->register_value;
		break;
	case MXFT_UCHAR:
		*((unsigned char *) value_ptr)
				= (unsigned char) ad->register_value;
		break;
	case MXFT_SHORT:
		*((short *) value_ptr) = (short) ad->register_value;
		break;
	case MXFT_USHORT:
		*((unsigned short *) value_ptr)
				= (unsigned short) ad->register_value;
		break;
	case MXFT_BOOL:
		*((mx_bool_type *) value_ptr)
				= (mx_bool_type) ad->register_value;
		break;
	case MXFT_LONG:
		*((long *) value_ptr) = ad->register_value;
		break;
	case MXFT_ULONG:
	case MXFT_HEX:
		*((unsigned long *) value_ptr)
				= (unsigned long) ad->register_value;
		break;
	case MXFT_INT64:
		*((int64_t *) value_ptr) = (int64_t) ad->register_value;
		break;
	case MXFT_UINT64:
		*((uint64_t *) value_ptr) = (uint64_t) ad->register_value;
		break;
	case MXFT_FLOAT:
		*((float *) value_ptr) = (float) ad->register_value;
		break;
	case MXFT_DOUBLE:
		*((double *) value_ptr) = (double) ad->register_value;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		    "Unsupported data type %ld requested for parameter '%s'.",
				field->datatype, ad->register_name );
	}

	/* Send the new register value to the detector. */

	ad->parameter_type = field->label_value;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_default_get_parameter_handler( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_get_parameter_handler()";

	MX_SEQUENCE_PARAMETERS seq;
	unsigned long i;
	double double_value;
	long num_frames;
	double exposure_time, frame_time;
	mx_status_type mx_status;

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAMESIZE:
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
	case MXLV_AD_BITS_PER_PIXEL:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_TRIGGER_MODE:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_ROI_NUMBER:
	case MXLV_AD_CORRECTION_FLAGS:
	case MXLV_AD_SHUTTER_ENABLE:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;

	case MXLV_AD_REGISTER_VALUE:
		mx_status = mx_area_detector_default_get_register( ad );
		break;

	case MXLV_AD_ROI:
		i = ad->roi_number;

		ad->roi[0] = ad->roi_array[i][0];
		ad->roi[1] = ad->roi_array[i][1];
		ad->roi[2] = ad->roi_array[i][2];
		ad->roi[3] = ad->roi_array[i][3];
		break;

	case MXLV_AD_BYTES_PER_FRAME:
		double_value = ad->bytes_per_pixel
		    * ((double) ad->framesize[0]) * ((double) ad->framesize[0]);

		ad->bytes_per_frame = mx_round( ceil(double_value) );
		break;

	case MXLV_AD_MAXIMUM_FRAME_NUMBER:
		ad->maximum_frame_number = 0;
		break;

	case MXLV_AD_SEQUENCE_START_DELAY:
		ad->sequence_start_delay = 0;
		break;

	case MXLV_AD_DETECTOR_READOUT_TIME:
		ad->detector_readout_time = 0;
		break;

	case MXLV_AD_TOTAL_ACQUISITION_TIME:
	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		mx_status = mx_area_detector_get_sequence_parameters(
							ad->record, &seq );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_get_sequence_start_delay(
							ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->total_sequence_time = 0.0;

		switch( seq.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
			exposure_time = seq.parameter_array[0];

			ad->total_acquisition_time = exposure_time;

			ad->total_sequence_time = ad->sequence_start_delay
						+ ad->total_acquisition_time
						+ ad->detector_readout_time;
			break;
		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_CIRCULAR_MULTIFRAME:
			num_frames = seq.parameter_array[0];
			exposure_time = seq.parameter_array[1];
			frame_time = seq.parameter_array[2];

			if ( frame_time <
				(exposure_time + ad->detector_readout_time) )
			{
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"The frame time (%g seconds) is shorter than "
				"the sum of the exposure time (%g seconds) and "
				"the detector readout time (%g seconds) for "
				"area detector '%s'.  It is impossible for "
				"the frame as a whole to take less time than "
				"the events that happen within the frame.",
					frame_time, exposure_time,
					ad->detector_readout_time,
					ad->record->name );
			}

			ad->total_acquisition_time =
				frame_time * (double) num_frames;

			ad->total_sequence_time = ad->sequence_start_delay
						+ ad->total_acquisition_time;
			break;
		case MXT_SQ_STROBE:
			num_frames = seq.parameter_array[0];
			exposure_time = seq.parameter_array[1];

			ad->total_acquisition_time =
				( exposure_time + ad->detector_readout_time )
					* (double) num_frames;

			ad->total_sequence_time = ad->sequence_start_delay
						+ ad->total_acquisition_time;
			break;
		case MXT_SQ_BULB:
			num_frames = seq.parameter_array[0];

			ad->total_acquisition_time = ad->detector_readout_time
							* (double) num_frames;

			ad->total_sequence_time = ad->sequence_start_delay
						+ ad->total_acquisition_time;
			break;
		default:
			ad->total_acquisition_time = 0.0;
			ad->total_sequence_time = 0.0;

			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' is configured for unsupported "
			"sequence type %ld.",
				ad->record->name,
				seq.sequence_type );
		}

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: ad->total_sequence_time = %g",
			fname, ad->total_sequence_time));
#endif
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for area detector '%s'.",
			mx_get_field_label_string( ad->record,
						ad->parameter_type ),
			ad->parameter_type,
			ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_set_parameter_handler( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_set_parameter_handler()";

	unsigned long i;
	double bytes_per_frame;
	mx_status_type mx_status;

	switch( ad->parameter_type ) {
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_TRIGGER_MODE:
	case MXLV_AD_ROI_NUMBER:
	case MXLV_AD_CORRECTION_FLAGS:
	case MXLV_AD_SHUTTER_ENABLE:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;

	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
		/* If we change the framesize or the binsize, then we must
		 * update the number of bytes per frame to match.
		 */

		bytes_per_frame =
		    ad->bytes_per_pixel * ad->framesize[0] * ad->framesize[1];

		ad->bytes_per_frame = mx_round( bytes_per_frame );
		break;

	case MXLV_AD_REGISTER_VALUE:
		mx_status = mx_area_detector_default_set_register( ad );
		break;

	case MXLV_AD_ROI:
		i = ad->roi_number;

		ad->roi_array[i][0] = ad->roi[0];
		ad->roi_array[i][1] = ad->roi[1];
		ad->roi_array[i][2] = ad->roi[2];
		ad->roi_array[i][3] = ad->roi[3];
		break;

	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_MAXIMUM_FRAMESIZE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Changing the parameter '%s' for record '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for area detector '%s'.",
			mx_get_field_label_string( ad->record,
						ad->parameter_type ),
			ad->parameter_type,
			ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#if MX_AREA_DETECTOR_USE_DEZINGER
#  define MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION \
	do {								      \
		for ( i = 0; i < num_exposures; i++ ) {			      \
			if ( dezinger_frame_array[i] != NULL ) {	      \
				mx_image_free(dezinger_frame_array[i]);       \
			}						      \
		}							      \
		(void) mx_area_detector_set_shutter_enable(ad->record, TRUE); \
	} while (0)
#else
#  define MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION \
	do {				                                      \
		free( sum_array );	                                      \
		(void) mx_area_detector_set_shutter_enable(ad->record, TRUE); \
	} while (0)
#endif

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_default_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_measure_correction()";

	MX_IMAGE_FRAME *dest_frame;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *geometrical_correction_fn ) ( MX_AREA_DETECTOR *,
							MX_IMAGE_FRAME * );
	unsigned long saved_correction_flags, desired_correction_flags;
	double exposure_time;
	struct timespec exposure_timespec;
	long i, n, num_exposures, pixels_per_frame;
	time_t time_buffer;
	unsigned long ad_status;
	mx_status_type mx_status, mx_status2;

#if MX_AREA_DETECTOR_USE_DEZINGER
	MX_IMAGE_FRAME **dezinger_frame_array;

#  if MX_AREA_DETECTOR_DEBUG_DEZINGER
	MX_HRT_TIMING measurement;
#  endif
#else
	void *void_image_data_pointer;
	uint16_t *src_array, *dest_array;
	double *sum_array;
	double temp_double;
	size_t image_length;
#endif

	mx_status = mx_area_detector_get_pointers( ad->record,
						NULL, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));

	MX_DEBUG(-2,("%s: type = %ld, time = %g, num measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif

	geometrical_correction_fn = flist->geometrical_correction;

	if ( geometrical_correction_fn == NULL ) {
		geometrical_correction_fn =
			mx_area_detector_default_geometrical_correction;
	}

	if ( ad->image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The area detector is currently using an image format of %ld.  "
		"At present, only GREY16 image format is supported.",
			ad->image_format );
	}

	pixels_per_frame = ad->framesize[0] * ad->framesize[1]; 

	exposure_time = ad->correction_measurement_time;

	num_exposures = ad->num_correction_measurements;

	if ( num_exposures <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of exposures (%ld) requested for "
		"area detector '%s'.  The minimum number of exposures "
		"allowed is 1.",  num_exposures, ad->record->name );
	}

	/* The correction frames will originally be read into the image frame.
	 * We must make sure that the image frame is big enough to hold
	 * the image data.
	 */

	mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the destination image frame is already big enough
	 * to hold the image frame that we are going to put in it.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:

		mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->dark_current_frame) );

		dest_frame = ad->dark_current_frame;
		desired_correction_flags =
			MXFT_AD_MASK_FRAME | MXFT_AD_BIAS_FRAME;
		break;
	
	case MXFT_AD_FLOOD_FIELD_FRAME:
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->flood_field_frame) );

		dest_frame = ad->flood_field_frame;

		desired_correction_flags = 
	MXFT_AD_MASK_FRAME | MXFT_AD_BIAS_FRAME | MXFT_AD_DARK_CURRENT_FRAME;

		if ( ( ad->geom_corr_after_flood == FALSE )
		  && ( ad->correction_frame_geom_corr_last == FALSE )
		  && ( ad->correction_frame_no_geom_corr == FALSE ) )
		{
			desired_correction_flags
				|= MXFT_AD_GEOMETRICAL_CORRECTION;
		}
		break;

	default:
		desired_correction_flags = 0;

		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );

		dest_frame = NULL;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_USE_DEZINGER

	/* Allocate an array of dezinger frames. */

	dezinger_frame_array = calloc(num_exposures, sizeof(MX_IMAGE_FRAME *));

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: dezinger_frame_array = %p",
		fname, dezinger_frame_array));
#endif

	if ( dezinger_frame_array == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a %ld element array of MX_IMAGE_FRAME pointers.",
			num_exposures );
	}

#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

	/* Get a pointer to the destination array. */

	mx_status = mx_image_get_image_data_pointer( dest_frame,
						&image_length,
						&void_image_data_pointer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#  if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: dest_frame = %p, void_image_data_pointer = %p",
		fname, dest_frame, void_image_data_pointer));
#  endif

	dest_array = void_image_data_pointer;

#  if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: dest_array = %p", fname, dest_array));
#  endif

	/* Allocate a double precision array to store intermediate sums. */

	sum_array = calloc( pixels_per_frame, sizeof(double) );

#  if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: sum_array = %p", fname, sum_array));
#  endif

	if ( sum_array == (double *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a %ld element array of doubles.", pixels_per_frame );
	}
#endif

	/* Put the area detector in One-shot mode. */

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
							exposure_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
		return mx_status;
	}

	/* If this is a dark current correction, disable the shutter. */

	if ( ad->correction_measurement_type == MXFT_AD_DARK_CURRENT_FRAME ) {
		mx_status = mx_area_detector_set_shutter_enable(
							ad->record, FALSE );

		if ( mx_status.code != MXE_SUCCESS ) {
			MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
			return mx_status;
		}
	}

	/* Take the requested number of exposures and sum together
	 * the pixels from each exposure.
	 */

	for ( n = 0; n < num_exposures; n++ ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Exposure %ld", fname, n));
#endif

		/* Start the exposure. */

		mx_status = mx_area_detector_start( ad->record );

		if ( mx_status.code != MXE_SUCCESS ) {
			MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
			return mx_status;
		}

		/* Wait for the exposure to end. */

		for(;;) {
			mx_status = mx_area_detector_get_status( ad->record,
								&ad_status );

			if ( mx_status.code != MXE_SUCCESS ) {
				MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
				return mx_status;
			}

#if 0
			if ( mx_kbhit() ) {
				(void) mx_getch();

				MX_DEBUG(-2,("%s: INTERRUPTED", fname));

				MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;

				return mx_area_detector_stop( ad->record );
			}
#endif

			if ((ad_status & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0)
			{

				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		/* Readout the frame into ad->image_frame. */

		mx_status = mx_area_detector_readout_frame( ad->record, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
			return mx_status;
		}

#if 0
		{
			char filename[MXU_FILENAME_LENGTH+1];

			snprintf( filename, sizeof(filename),
				"rawdark%d.smv", n );

			MX_DEBUG(-10,("%s: Saving raw dark frame '%s'",
				fname, filename));

			(void) mx_image_write_smv_file( ad->image_frame,
							filename );
		}
#endif

		/* Perform any necessary image corrections. */

		if ( desired_correction_flags != 0 ) {

			mx_status = mx_area_detector_get_correction_flags(
					ad->record, &saved_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
				return mx_status;
			}

			mx_status = mx_area_detector_set_correction_flags(
					ad->record, desired_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
				return mx_status;
			}

			mx_status = mx_area_detector_correct_frame(ad->record);

			mx_status2 = mx_area_detector_set_correction_flags(
					ad->record, saved_correction_flags );

			if ( mx_status2.code != MXE_SUCCESS ) {
				MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
				return mx_status2;
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
				return mx_status;
			}
		}

#if MX_AREA_DETECTOR_USE_DEZINGER
		/* Copy the image frame to the dezinger frame array. */

		mx_status = mx_image_copy_frame( ad->image_frame,
					&(dezinger_frame_array[n]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
			return mx_status;
		}

#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

		/* Get the image data pointer. */

		mx_status = mx_image_get_image_data_pointer( ad->image_frame,
						&image_length,
						&void_image_data_pointer );

		if ( mx_status.code != MXE_SUCCESS ) {
			MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
			return mx_status;
		}

		src_array = void_image_data_pointer;

		/* Add the pixels in this image to the sum array. */

		for ( i = 0; i < pixels_per_frame; i++ ) {
			sum_array[i] += (double) src_array[i];
		}
#endif /* not MX_AREA_DETECTOR_USE_DEZINGER */

		mx_msleep(10);
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Calculating normalized pixels.", fname));
#endif

#if MX_AREA_DETECTOR_USE_DEZINGER

#  if MX_AREA_DETECTOR_DEBUG_DEZINGER
	MX_HRT_START( measurement );
#  endif

	mx_status = mx_image_dezinger( &dest_frame,
					num_exposures,
					dezinger_frame_array,
					fabs(ad->dezinger_threshold) );

#  if MX_AREA_DETECTOR_DEBUG_DEZINGER
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "Total image dezingering time." );
#  endif

	if ( mx_status.code != MXE_SUCCESS ) {
		MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;
		return mx_status;
	}

#else /* not MX_AREA_DETECTOR_USE_DEZINGER */

	/* Copy normalized pixels to the destination array. */

	for ( i = 0; i < pixels_per_frame; i++ ) {
		temp_double = sum_array[i] / num_exposures;

		dest_array[i] = mx_round( temp_double );

#  if MX_AREA_DETECTOR_DEBUG
		if ( i < 5 ) {
			MX_DEBUG(-2,
			("src_array[%ld] = %d, dest_array[%ld] = %d",
				i, (int) src_array[i], i, (int) dest_array[i]));
		}
#  endif
	}

#endif /* not MX_AREA_DETECTOR_USE_DEZINGER */

	MXP_AREA_DETECTOR_CLEANUP_AFTER_CORRECTION;

	/* If requested, perform a delayed geometrical correction on a
	 * flood field frame after averaging or dezingering the source
	 * frames.
	 */

	if ( ( ad->correction_measurement_type == MXFT_AD_FLOOD_FIELD_FRAME )
	  && ( ad->correction_frame_geom_corr_last == TRUE )
	  && ( ad->correction_frame_no_geom_corr == FALSE ) )
	{
		MX_DEBUG(-2,("%s: Invoking delayed geometrical correction "
				"on measured flood field frame", fname ));

		mx_status = (*geometrical_correction_fn)( ad, dest_frame );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the image frame exposure time. */

	exposure_timespec =
		mx_convert_seconds_to_high_resolution_time( exposure_time );

	MXIF_EXPOSURE_TIME_SEC(dest_frame)  = exposure_timespec.tv_sec;
	MXIF_EXPOSURE_TIME_NSEC(dest_frame) = exposure_timespec.tv_nsec;

	/* Set the image frame timestamp to the current time. */

	MXIF_TIMESTAMP_SEC(dest_frame) = time( &time_buffer );
	MXIF_TIMESTAMP_NSEC(dest_frame) = 0;

#if MX_AREA_DETECTOR_DEBUG
	mx_status = mx_image_get_exposure_time(ad->image_frame, &exposure_time);

	MX_DEBUG(-2,("%s: correction measurement complete.", fname));
#endif
	
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

#define FREE_DEZINGER_ARRAYS \
	do {								\
		if ( image_frame_array != NULL ) {			\
			for ( z = 0; z < num_exposures; z++ ) {		\
				mx_image_free( image_frame_array[z] );	\
			}						\
		}							\
	} while(0)

MX_EXPORT mx_status_type
mx_area_detector_default_dezinger_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_dezinger_correction()";

	MX_IMAGE_FRAME *dest_frame;
	MX_IMAGE_FRAME **image_frame_array;
	unsigned long saved_correction_flags, desired_correction_flags;
	long i, n, z, num_exposures, pixels_per_frame;
	double exposure_time;
	unsigned long ad_status;
	mx_status_type mx_status, mx_status2;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));

	MX_DEBUG(-2,("%s: type = %ld, time = %g, num measurements = %ld",
		fname, ad->correction_measurement_type,
		ad->correction_measurement_time,
		ad->num_correction_measurements ));
#endif

	if ( ad->image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The area detector is currently using an image format of %ld.  "
		"At present, only GREY16 image format is supported.",
			ad->image_format );
	}

	pixels_per_frame = ad->framesize[0] * ad->framesize[1]; 

	exposure_time = ad->correction_measurement_time;

	num_exposures = ad->num_correction_measurements;

	if ( num_exposures <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of exposures (%ld) requested for "
		"area detector '%s'.  The minimum number of exposures "
		"allowed is 1.",  num_exposures, ad->record->name );
	}

	/* The correction frames will originally be read into the image frame.
	 * We must make sure that the image frame is big enough to hold
	 * the image data.
	 */

	mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure that the destination image frame is already big enough
	 * to hold the image frame that we are going to put in it.
	 */

	switch( ad->correction_measurement_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->dark_current_frame) );

		dest_frame = ad->dark_current_frame;

		desired_correction_flags = 0;
		break;
	
	case MXFT_AD_FLOOD_FIELD_FRAME:
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(ad->flood_field_frame) );

		dest_frame = ad->flood_field_frame;

		desired_correction_flags = MXFT_AD_DARK_CURRENT_FRAME;
		break;

	default:
		desired_correction_flags = 0;

		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Correction measurement type %ld is not supported "
		"for area detector '%s'.",
			ad->correction_measurement_type, ad->record->name );

		dest_frame = NULL;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Allocate an array of images to store the images to be used
	 * for dezingering.
	 */

	image_frame_array = malloc( num_exposures * sizeof(MX_IMAGE_FRAME *) );

	if ( image_frame_array == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Could not allocate a %lu element array of MX_IMAGE_FRAME pointers.",
			num_exposures );
	}

	for ( i = 0; i < num_exposures; i++ ) {
		mx_status = mx_area_detector_setup_frame( ad->record,
						&(image_frame_array[i]) );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}
	}

	/* Put the area detector in One-shot mode. */

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
							exposure_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		FREE_DEZINGER_ARRAYS;
		return mx_status;
	}

	/* Take the requested number of exposures. */

	for ( n = 0; n < num_exposures; n++ ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Exposure %ld", fname, n));
#endif

		/* Start the exposure. */

		mx_status = mx_area_detector_start( ad->record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}

		/* Wait for the exposure to end. */

		for(;;) {
			mx_status = mx_area_detector_get_status( ad->record,
								&ad_status );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}

#if 0
			if ( mx_kbhit() ) {
				(void) mx_getch();

				MX_DEBUG(-2,("%s: INTERRUPTED", fname));

				FREE_DEZINGER_ARRAYS;
				return mx_area_detector_stop( ad->record );
			}
#endif

			if ((ad_status & MXSF_AD_ACQUISITION_IN_PROGRESS) == 0)
			{

				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		/* Readout the frame into ad->image_frame. */

		mx_status = mx_area_detector_readout_frame( ad->record, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}

		/* Perform any necessary image corrections. */

		if ( desired_correction_flags != 0 ) {
			mx_status = mx_area_detector_get_correction_flags(
					ad->record, &saved_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}

			mx_status = mx_area_detector_set_correction_flags(
					ad->record, desired_correction_flags );

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}

			mx_status = mx_area_detector_correct_frame( ad->record);

			mx_status2 = mx_area_detector_set_correction_flags(
					ad->record, saved_correction_flags );

			if ( mx_status2.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status2;
			}

			if ( mx_status.code != MXE_SUCCESS ) {
				FREE_DEZINGER_ARRAYS;
				return mx_status;
			}
		}

		/* Transfer the frame to the image frame array. */

		mx_status = mx_area_detector_transfer_frame( ad->record,
							MXFT_AD_IMAGE_FRAME,
							image_frame_array[n] );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEZINGER_ARRAYS;
			return mx_status;
		}
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Dezingering images.", fname));
#endif

	mx_status = mx_image_dezinger( &dest_frame,
					num_exposures,
					image_frame_array,
					ad->dezinger_threshold );
	FREE_DEZINGER_ARRAYS;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: correction measurement complete.", fname));
#endif
	
	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_default_geometrical_correction( MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *frame )
{
	static const char fname[] =
		"mx_area_detector_default_geometrical_correction()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Geometrical correction has not yet "
	"been implemented for area detector '%s'.", ad->record->name );
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_check_correction_framesize( MX_AREA_DETECTOR *ad,
						MX_IMAGE_FRAME *image_frame,
						MX_IMAGE_FRAME *test_frame,
						const char *frame_name )
{
	static const char fname[] =
		"mxp_area_detector_check_correction_framesize()";

	unsigned long image_row_framesize, image_column_framesize;
	unsigned long test_row_framesize, test_column_framesize;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	/* It is OK for test_frame to be NULL. */

	if ( test_frame == (MX_IMAGE_FRAME *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	image_row_framesize = MXIF_ROW_FRAMESIZE(image_frame);
	image_column_framesize = MXIF_COLUMN_FRAMESIZE(image_frame);

	test_row_framesize = MXIF_ROW_FRAMESIZE(test_frame);
	test_column_framesize = MXIF_COLUMN_FRAMESIZE(test_frame);

	if ( (image_row_framesize != test_row_framesize)
	  || (image_column_framesize != test_column_framesize) )
	{
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The current %s framesize (%lu,%lu) for area detector '%s' is "
		"different from the framesize (%lu,%lu) of the image frame.",
			frame_name, test_row_framesize,
			test_column_framesize, ad->record->name,
			image_row_framesize, image_column_framesize );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_use_low_memory_methods( MX_AREA_DETECTOR *ad,
				mx_bool_type *use_low_memory_methods )
{
#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	static const char fname[] =
		"mxp_area_detector_use_low_memory_methods()";
#endif

	MX_SYSTEM_MEMINFO system_meminfo;
	MX_PROCESS_MEMINFO process_meminfo;
	long row_framesize, column_framesize;
	mx_memsize_type bytes_per_float_array;
	mx_memsize_type process_memory_needed, extra_memory_needed;
	double process_to_system_ratio;
	mx_status_type mx_status;

	mx_status = mx_get_system_meminfo( &system_meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_info("--------------------------------");
	mx_display_system_meminfo( &system_meminfo );
	mx_info("--------------------------------");
#endif

	mx_status = mx_get_process_meminfo( MXF_PROCESS_ID_SELF,
						&process_meminfo );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	mx_display_process_meminfo( &process_meminfo );
	mx_info("--------------------------------");
#endif

	/* If requested in the MX database file, we force the use
	 * of a particular method here.
	 */

	if ( ad->initial_correction_flags & MXFT_AD_USE_LOW_MEMORY_METHODS ) {

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
		MX_DEBUG(-2,
		("%s: forced *use_low_memory_methods to TRUE", fname));
#endif
		*use_low_memory_methods = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ad->initial_correction_flags & MXFT_AD_USE_HIGH_MEMORY_METHODS ) {

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
		MX_DEBUG(-2,
		("%s: forced *use_low_memory_methods to FALSE", fname));
#endif
		*use_low_memory_methods = FALSE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* See if we will need to have more memory. */

	mx_status = mx_area_detector_get_framesize( ad->record,
					&row_framesize, &column_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_per_float_array =
		row_framesize * column_framesize * sizeof(float);

	extra_memory_needed = 0;

	if ( ad->dark_current_offset_array == NULL ) {
		extra_memory_needed += bytes_per_float_array;
	}
	if ( ad->flood_field_scale_array == NULL ) {
		extra_memory_needed += bytes_per_float_array;
	}

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: extra_memory_needed = %lu",
		fname, extra_memory_needed));
#endif

	process_memory_needed = process_meminfo.total_bytes
					+ extra_memory_needed;

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: process_memory_needed = %lu, system_memory = %lu",
		fname, process_memory_needed, system_meminfo.total_bytes ));
#endif

	process_to_system_ratio = mx_divide_safely( process_memory_needed,
					system_meminfo.total_bytes );

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: process_to_system_ratio = %g",
		fname, process_to_system_ratio));
#endif

	/*---*/

	if ( process_to_system_ratio < 0.8 ) {
		*use_low_memory_methods = FALSE;
	} else {
		*use_low_memory_methods = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_USE_LOWMEM_METHOD
	MX_DEBUG(-2,("%s: *use_low_memory_methods = %d",
		fname, *use_low_memory_methods));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mxp_area_detector_u16_highmem_dark_correction() is for use when enough
 * free memory is available that page swapping will not be required.
 */

static mx_status_type
mxp_area_detector_u16_highmem_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mxp_area_detector_u16_highmem_dark_correction()";

	long i, num_pixels;
	double image_pixel, image_exposure_time;
	float *dark_current_offset_array;
	uint16_t *image_data_array, *mask_data_array;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_data_array = %p", fname, image_data_array));
#endif

	/* Discard the old dark current offset array if the exposure time
	 * has changed significantly.
	 */

	mx_status = mx_image_get_exposure_time( image_frame,
						&image_exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,
	("%s: image_exposure_time = %g", fname, image_exposure_time));
#endif

	if ( mx_difference( image_exposure_time,
				ad->old_exposure_time ) > 0.001 )
	{
		mx_free( ad->dark_current_offset_array );
	}

	ad->old_exposure_time = image_exposure_time;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	/*---*/

	/* Get the dark current offset array, creating a new one if necessary.*/

	if ( dark_current_frame == NULL ) {
		dark_current_offset_array = NULL;
	} else {
		if ( ad->dark_current_offset_array == NULL ) {
		    mx_status = mx_area_detector_compute_dark_current_offset(
					ad, bias_frame, dark_current_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		dark_current_offset_array = ad->dark_current_offset_array;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_offset_array = %p",
			fname, dark_current_offset_array));
#endif
	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"image_data_array[%lu] = %d\n",
						i, (int) image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* If requested, apply the dark current correction. */

		if ( dark_current_offset_array != NULL ) {

			/* Get the raw image pixel. */

			image_pixel = (double) image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "BEFORE image_pixel = %g, ",
						image_pixel );
				fprintf( stderr,
				"dark_current_offset_array[%lu] = %g, ",
					i, dark_current_offset_array[i] );
			}
#endif
			image_pixel = image_pixel
					+ dark_current_offset_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "AFTER image_pixel = %g, ",
						image_pixel );
			}
#endif

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			if ( image_pixel < 0.0 ) {
				image_data_array[i] = 0;
			} else {
				image_data_array[i] = image_pixel + 0.5;
			}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"Final image_data_array[%lu] = %d, ",
					i, (int) image_data_array[i]);
			}
#endif
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "\n" );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mxp_area_detector_u16_lowmem_dark_correction() is for use to avoid
 * swapping when the amount of free memory available is small.
 */

static mx_status_type
mxp_area_detector_u16_lowmem_dark_correction( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *image_frame,
					MX_IMAGE_FRAME *mask_frame,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mxp_area_detector_u16_lowmem_dark_correction()";

	long i, num_pixels;
	uint16_t *image_data_array, *mask_data_array, *bias_data_array;
	uint16_t *dark_current_data_array;
	double image_exposure_time, dark_current_exposure_time;
	double exposure_time_ratio;
	double image_pixel, raw_dark_current, scaled_dark_current;
	unsigned long bias_offset;
	mx_bool_type use_scaled_dark_current;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_data_array = image_frame->image_data;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: image_data_array = %p", fname, image_data_array));
#endif

	/* Discard the dark current offset array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->dark_current_offset_array );

	/* Get pointers to the mask, bias, and dark current image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: mask_data_array = %p", fname, mask_data_array));
#endif

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: bias_data_array = %p", fname, bias_data_array));
#endif

	if ( dark_current_frame == NULL ) {
		dark_current_data_array = NULL;
	} else {
		dark_current_data_array = dark_current_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: dark_current_frame = %p",
			fname, dark_current_frame));
	MX_DEBUG(-2,("%s: dark_current_data_array = %p",
			fname, dark_current_data_array));
#endif

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		use_scaled_dark_current = FALSE;
	} else
	if ( dark_current_frame == NULL ) {
		use_scaled_dark_current = FALSE;
	} else {
		use_scaled_dark_current = TRUE;
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: use_scaled_dark_current = %d",
		fname, (int) use_scaled_dark_current));
#endif

	if ( use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		mx_status = mx_image_get_exposure_time( image_frame,
							&image_exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_image_get_exposure_time( dark_current_frame,
						&dark_current_exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( image_exposure_time,
						dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	/* Do the mask, bias, and dark current corrections. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	for ( i = 0; i < num_pixels; i++ ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf(stderr, "i = %lu, ", i);
		}
#endif
		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf(stderr, "mask_data_array[%lu] = %d, ",
				    i, (int) mask_data_array[i] );
			}
#endif
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				image_data_array[i] = 0;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
				if ( i < 10 ) {
					fprintf(stderr,
					"image_data_array[%lu] = %d\n",
						i, (int) image_data_array[i]);
				}
#endif
				continue;
			}
		}

		/* Get the bias offset for this pixel. */

		if ( bias_data_array != NULL ) {
			bias_offset = bias_data_array[i];
		} else {
			bias_offset = 0;
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "bias_offset = %d, ",
					(int) bias_offset );
		}
#endif
		/* Get the raw image pixel. */

		image_pixel = (double) image_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "BEFORE image_pixel = %g, ",
				image_pixel );
		}
#endif

		/* If requested, apply the dark current correction. */

		if ( dark_current_data_array != NULL ) {
			raw_dark_current = (double) dark_current_data_array[i];

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "raw_dark_current = %g, ",
					raw_dark_current );
			}
#endif
			scaled_dark_current = exposure_time_ratio
			    * (raw_dark_current - bias_offset) + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr, "scaled_dark_current = %g, ",
					scaled_dark_current );
			}
#endif
			image_pixel = image_pixel + bias_offset;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
				"AFTER adding bias, image_pixel = %g, ",
					image_pixel );
			}
#endif
			image_pixel = image_pixel - scaled_dark_current;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
			if ( i < 10 ) {
				fprintf( stderr,
			"AFTER subtracting dark current, image_pixel = %g, ",
					image_pixel );
			}
#endif
		}

		/* Round to the nearest integer by adding 0.5 and
		 * then truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function
		 * calls have too high of an overhead to be used
		 * in this loop.
		 */

		if ( image_pixel < 0.0 ) {
			image_data_array[i] = 0;
		} else {
			image_data_array[i] = image_pixel + 0.5;
		}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr, "image_data_array[%lu] = %d\n",
				i, (int) image_data_array[i] );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_u16_highmem_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mxp_area_detector_u16_highmem_flood_field()";

	long i, num_pixels;
	double image_pixel, bias_offset;
	float *flood_field_scale_array;
	uint16_t *image_data_array, *mask_data_array, *bias_data_array;
	mx_status_type mx_status;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_data_array = image_frame->image_data;

	/*---*/

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	/*---*/

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	/*---*/

	/* Get the flood field scale array, creating a new one if necessary. */

	if ( flood_field_frame == NULL ) {
		flood_field_scale_array = NULL;
	} else {
		if ( ad->flood_field_scale_array == NULL ) {
		    mx_status = mx_area_detector_compute_flood_field_scale(
					ad, bias_frame, flood_field_frame );

		    if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		}

		flood_field_scale_array = ad->flood_field_scale_array;
	}

	/* If requested, do the flood field correction. */

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

	if ( flood_field_scale_array != NULL ) {

		num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
				* MXIF_COLUMN_FRAMESIZE(image_frame);

		for ( i = 0; i < num_pixels; i++ ) {

			if ( mask_data_array != NULL ) {
				if ( mask_data_array[i] == 0 ) {

					/* If the pixel is masked off, skip
					 * this pixel and return to the top
					 * of the loop for the next pixel.
					 */

					continue;
				}
			}

			if ( bias_data_array != NULL ) {
				bias_offset = bias_data_array[i];
			} else {
				bias_offset = 0;
			}

			image_pixel = (double) image_data_array[i];

			image_pixel = image_pixel - bias_offset;

			image_pixel = image_pixel * flood_field_scale_array[i];

			image_pixel = image_pixel + bias_offset;

			/* Round to the nearest integer by adding 0.5 and
			 * then truncating.
			 *
			 * We _must_ _not_ use mx_round() here since function
			 * calls have too high of an overhead to be used
			 * in this loop.
			 */

			image_data_array[i] = image_pixel + 0.5;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_u16_lowmem_flood_field( MX_AREA_DETECTOR *ad,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mxp_area_detector_u16_lowmem_flood_field()";

	long i, num_pixels;
	uint16_t *image_data_array, *mask_data_array, *bias_data_array;
	uint16_t *flood_field_data_array;
	unsigned long raw_flood_field, bias_offset;
	double image_pixel, flood_field_scale_factor;
	double ffs_numerator, ffs_denominator;
	double flood_field_scale_max, flood_field_scale_min;
	mx_bool_type flood_less_than_or_equal_to_bias;

	/* Return now if we have not been provided with a flood field frame. */

	if ( flood_field_frame == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	flood_field_data_array = flood_field_frame->image_data;

	if ( image_frame == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The image_frame pointer passed was NULL." );
	}

	image_data_array = image_frame->image_data;

	/* Discard the flood field scale array if it is in memory,
	 * since we will not use it.
	 */

	mx_free( ad->flood_field_scale_array );

	/* Get pointers to the mask and bias image data. */

	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	num_pixels = MXIF_ROW_FRAMESIZE(image_frame)
			* MXIF_COLUMN_FRAMESIZE(image_frame);

	if ( bias_data_array == NULL ) {
		flood_less_than_or_equal_to_bias = FALSE;
	} else {
		flood_less_than_or_equal_to_bias = TRUE;
	}

	flood_field_scale_max = ad->flood_field_scale_max;
	flood_field_scale_min = ad->flood_field_scale_min;

	/* Now do the flood field correction. */

	for ( i = 0; i < num_pixels; i++ ) {

		/* See if the pixel is masked off. */

		if ( mask_data_array != NULL ) {
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				image_data_array[i] = 0;
				continue;
			}
		}

		image_pixel = (double) image_data_array[i];

		raw_flood_field = flood_field_data_array[i];

		if ( bias_data_array != NULL ) {
			bias_offset = bias_data_array[i];
		} else {
			bias_offset = 0;
		}

		/* We _must_ _not_ use mx_divide_safely() here since function
		 * calls have too high of an overhead to be used in this loop.
		 */

		if ( raw_flood_field <= bias_offset ) {
			image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		} else {
			flood_less_than_or_equal_to_bias = FALSE;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- ad->bias_average_intensity;

		flood_field_scale_factor = ffs_numerator / ffs_denominator;

		if ( flood_field_scale_factor > flood_field_scale_max ) {
			image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}
		if ( flood_field_scale_factor < flood_field_scale_min ) {
			image_data_array[i] = 0;
			continue;		/* Skip to the next pixel. */
		}

		image_pixel = image_pixel - bias_offset;

		image_pixel = image_pixel * flood_field_scale_factor;

		image_pixel = image_pixel + bias_offset;

		/* Round to the nearest integer by adding 0.5 and then
		 * truncating.
		 *
		 * We _must_ _not_ use mx_round() here since function calls
		 * have too high of an overhead to be used in this loop.
		 */

		if ( image_pixel < 0.5 ) {
			image_data_array[i] = 0;
		} else {
			image_data_array[i] = image_pixel + 0.5;
		}
	}

	if ( flood_less_than_or_equal_to_bias ) {
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The entire contents of the corrected image frame for "
		"area detector '%s' have been set to 0 since each pixel in "
		"the flood field frame '%s' is less than or equal to the "
		"value of the corresponding pixel in the bias frame '%s'.",
			ad->record->name,
			ad->flood_field_filename,
			ad->bias_filename );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/* mx_area_detector_frame_correction() requires that all of the frames have
 * the same framesize.
 */

MX_EXPORT mx_status_type
mx_area_detector_frame_correction( MX_RECORD *record,
				MX_IMAGE_FRAME *image_frame,
				MX_IMAGE_FRAME *mask_frame,
				MX_IMAGE_FRAME *bias_frame,
				MX_IMAGE_FRAME *dark_current_frame,
				MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] = "mx_area_detector_frame_correction()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *geometrical_correction_fn ) ( MX_AREA_DETECTOR *,
							MX_IMAGE_FRAME * );
	unsigned long flags;
	mx_bool_type use_low_memory_methods = FALSE;
	mx_bool_type geom_corr_before_flood;
	mx_bool_type geom_corr_after_flood;
	mx_bool_type correction_measurement_in_progress;
	mx_bool_type geometrical_correction_requested;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_TIMING initial_timing = MX_HRT_ZERO;
	MX_HRT_TIMING geometrical_timing = MX_HRT_ZERO;
	MX_HRT_TIMING flood_timing = MX_HRT_ZERO;
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("\n%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No primary image frame was provided." );
	}

	flags = ad->correction_flags;

#if 1
	MX_DEBUG(-2,("%s: ad->correction_flags = %#lx",
		fname, ad->correction_flags));
#endif

	if ( flags == 0 ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		MX_DEBUG(-2,("%s: No corrections requested.", fname));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Under what circumstances will we perform a geometrical correction? */

	correction_measurement_in_progress =
		ad->correction_measurement_in_progress;

	geometrical_correction_requested =
		ad->correction_flags & MXFT_AD_GEOMETRICAL_CORRECTION;

	if ( correction_measurement_in_progress ) {
		/* We are measuring a correction_frame. */

		if ( ad->correction_frame_no_geom_corr ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = FALSE;
		} else
		if ( ad->correction_frame_geom_corr_last ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = FALSE;
		} else
		if ( ad->geom_corr_after_flood ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = TRUE;
		} else {
			geom_corr_before_flood = TRUE;
			geom_corr_after_flood = FALSE;
		}
	} else {
		/* We are _not_ measuring a correction_frame. */

		if ( geometrical_correction_requested == FALSE ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = FALSE;
		} else
		if ( ad->geom_corr_after_flood ) {
			geom_corr_before_flood = FALSE;
			geom_corr_after_flood = TRUE;
		} else {
			geom_corr_before_flood = TRUE;
			geom_corr_after_flood = FALSE;
		}
	}

#if 1
	MX_DEBUG(-2,("%s: correction_measurement_in_progress = %d",
		fname, (int) correction_measurement_in_progress));
	MX_DEBUG(-2,("%s: geometrical_correction_requested = %d",
		fname, (int) geometrical_correction_requested));
	MX_DEBUG(-2,("%s: ad->correction_frame_no_geom_corr = %d",
		fname, (int) ad->correction_frame_no_geom_corr));
	MX_DEBUG(-2,("%s: ad->correction_frame_geom_corr_last = %d",
		fname, (int) ad->correction_frame_geom_corr_last));
	MX_DEBUG(-2,("%s: ad->geom_corr_after_flood = %d",
		fname, (int) ad->geom_corr_after_flood));
	MX_DEBUG(-2,
	("%s: geom_corr_before_flood = %d, geom_corr_after_flood = %d",
		fname, geom_corr_before_flood, geom_corr_after_flood));
#endif

	/*---*/

	/* If we have a lot of free memory available, then we can use
	 * floating point arrays of correction values to speed up the
	 * corrections.  We call mxp_area_detector_use_low_memory_methods()
	 * to see if enough memory is available.
	 */

	mx_status = mxp_area_detector_use_low_memory_methods( ad,
						&use_low_memory_methods );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

	geometrical_correction_fn = flist->geometrical_correction;

	if ( geometrical_correction_fn == NULL ) {
		geometrical_correction_fn =
			mx_area_detector_default_geometrical_correction;
	}

	/* Area detector image correction is currently only supported
	 * for 16-bit greyscale images (MXT_IMAGE_FORMAT_GREY16).
	 */

	if ( MXIF_IMAGE_FORMAT(image_frame) != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The primary image frame is not a 16-bit greyscale image." );
	}

	/*---*/

	mx_status = mxp_area_detector_check_correction_framesize( ad,
					    image_frame, mask_frame, "mask" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxp_area_detector_check_correction_framesize( ad,
					    image_frame, bias_frame, "bias" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxp_area_detector_check_correction_framesize( ad,
			    image_frame, dark_current_frame, "dark current" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxp_area_detector_check_correction_framesize( ad,
			    image_frame, flood_field_frame, "flood field" );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*---*/

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_START( initial_timing );
#endif

	/******* Dark current correction *******/

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: use_low_memory_methods = %d",
		fname, (int) use_low_memory_methods));
#endif

#if 1
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
#endif

	if ( use_low_memory_methods ) {
		mx_status = mxp_area_detector_u16_lowmem_dark_correction( ad,
							image_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
	} else {
		mx_status = mxp_area_detector_u16_highmem_dark_correction( ad,
							image_frame,
							mask_frame,
							bias_frame,
							dark_current_frame );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( initial_timing );

	MX_HRT_START( geometrical_timing );
#endif

	/******* Geometrical correction *******/

	if ( geom_corr_before_flood ) {

		/* If requested, do the geometrical correction. */

		if ( flags & MXFT_AD_GEOMETRICAL_CORRECTION ) {
			mx_status = (*geometrical_correction_fn)( ad,
								image_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( geometrical_timing );

	MX_HRT_START( flood_timing );
#endif

#if 1
	MX_DEBUG(-2,("%s: flood_field_frame = %p", fname, flood_field_frame));
#endif

	/******* Flood field correction *******/

	if ( use_low_memory_methods ) {
		mx_status = mxp_area_detector_u16_lowmem_flood_field( ad,
							image_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
	} else {
		mx_status = mxp_area_detector_u16_highmem_flood_field( ad,
							image_frame,
							mask_frame,
							bias_frame,
							flood_field_frame );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_HRT_END( flood_timing );
#endif

	/******* Delayed geometrical correction (if requested) *******/

	if ( geom_corr_after_flood ) {

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
		/* If invoked, this code overwrites the earlier
		 * measured value for the timing.
		 */

		MX_HRT_START( geometrical_timing );
#endif
		/* If requested, do the geometrical correction. */

		if ( flags & MXFT_AD_GEOMETRICAL_CORRECTION ) {
			mx_status = (*geometrical_correction_fn)( ad,
								image_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
		MX_HRT_END( geometrical_timing );
#endif
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION_TIMING
	MX_DEBUG(-2,(" "));	/* Print an empty line. */

	MX_HRT_RESULTS( initial_timing, fname,
				"Mask, bias, and dark correction time." );

	if ( ad->do_geometrical_correction_last ) {
		MX_HRT_RESULTS( flood_timing, fname, "Flood correction time." );
		MX_HRT_RESULTS( geometrical_timing, fname,
				"Geometrical correction time." );
	} else {
		MX_HRT_RESULTS( geometrical_timing, fname,
				"Geometrical correction time." );
		MX_HRT_RESULTS( flood_timing, fname, "Flood correction time." );
	}
#endif

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_vctest_extended_status( MX_RECORD_FIELD *record_field,
					mx_bool_type *value_changed_ptr )
{
	static const char fname[] = "mx_area_detector_vctest_extended_status()";

	MX_RECORD *record;
	MX_RECORD_FIELD *extended_status_field;
	MX_RECORD_FIELD *last_frame_number_field;
	MX_RECORD_FIELD *total_num_frames_field;
	MX_RECORD_FIELD *status_field;
	mx_bool_type last_frame_number_changed;
	mx_bool_type total_num_frames_changed;
	mx_bool_type status_changed;
	mx_status_type mx_status;

	if ( record_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The record field pointer passed was NULL." );
	}
	if ( value_changed_ptr == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The value_changed_ptr passed was NULL." );
	}

	record = record_field->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for record field %p is NULL.",
			record_field );
	}

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s invoked for record field '%s.%s'",
			fname, record->name, record_field->name));
#endif
	/* What we do here depends on whether or not this is the
	 * area detector's 'extended_status' field.
	 */

	if ( record_field->label_value != MXLV_AD_EXTENDED_STATUS ) {

		/* This is _not_ the 'extended_status' field. */

		/* If the 'extended_status' field has a callback list,
		 * then we skip this test, since the 'extended_status'
		 * field callback will take care of sending callbacks
		 * to the current field for us.
		 */

		mx_status = mx_find_record_field( record, "extended_status",
						&extended_status_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( extended_status_field->callback_list != NULL ) {
#if MX_AREA_DETECTOR_DEBUG_VCTEST
			MX_DEBUG(-2,
	("%s: Skipping test of '%s' due to existing test of 'extended_status'",
				fname, record_field->name ));
#endif
			*value_changed_ptr = FALSE;

			return MX_SUCCESSFUL_RESULT;
		}

		/* If we get here, then this is not the extended status
		 * field and we only need to check the field supplied
		 * by the caller.
		 */

		mx_status = mx_default_test_for_value_changed( record_field,
							value_changed_ptr );

		/* This is all we need to do for fields that are not
		 * the 'extended_status' field, so we return now.
		 */

		return mx_status;
	}

	/* If we get here, then this _is_ the 'extended_status' field. */

	*value_changed_ptr = FALSE;

	/* Test the 'last_frame_number' field. */

	mx_status = mx_find_record_field( record, "last_frame_number",
						&last_frame_number_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_default_test_for_value_changed( last_frame_number_field,
						&last_frame_number_changed );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Test the 'total_num_frames' field. */

	mx_status = mx_find_record_field( record, "total_num_frames",
						&total_num_frames_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_default_test_for_value_changed( total_num_frames_field,
						&total_num_frames_changed );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Test the 'status' field. */

	mx_status = mx_find_record_field( record, "status",
						&status_field );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_default_test_for_value_changed( status_field,
						&status_changed );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: last_frame_number_changed = %d.",
		fname, last_frame_number_changed));
	MX_DEBUG(-2,("%s: total_num_frames_changed = %d.",
		fname, total_num_frames_changed));
	MX_DEBUG(-2,("%s: status_changed = %d.", fname, status_changed));
#endif

	*value_changed_ptr = last_frame_number_changed 
		| total_num_frames_changed | status_changed;

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: 'get_extended_status' value_changed = %d.",
		fname, *value_changed_ptr));
#endif

	/* If the extended_status value did not change, then we are done. */

	if ( *value_changed_ptr == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then one or more of the fields 'last_frame_number',
	 * 'total_num_frames', or 'status' changed its value.  For each field,
	 * see if its value changed.  If the value changed, then invoke the
	 * field's callback list, if it has one.
	 */

	/* Check 'last_frame_number'. */

	if ( last_frame_number_changed ) {
		if ( last_frame_number_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				last_frame_number_field, MXCBT_NONE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Check 'total_num_frames'. */

	if ( total_num_frames_changed ) {
		if ( total_num_frames_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				total_num_frames_field, MXCBT_NONE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Check 'status'. */

	if ( status_changed ) {
		if ( status_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				status_field, MXCBT_NONE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

/************************************************************************
 * The following functions are intended for use only in device drivers. *
 * They should not be called directly by application programs.          *
 ************************************************************************/

static long
mx_area_detector_compute_binsize( double original_binsize,
				int num_allowed_binsizes,
				long *allowed_binsize )
{
	int i, last;
	long new_binsize, binsize_below, binsize_above;
	double allowed_binsize_ratio, binsize_ratio;

	MX_DEBUG( 2,("original_binsize = %g", original_binsize));

	new_binsize = allowed_binsize[0];

	last = num_allowed_binsizes - 1;

	if ( original_binsize <= allowed_binsize[0] ) {
			new_binsize = allowed_binsize[0];

	} else
	if ( original_binsize >= allowed_binsize[last] ) {
			new_binsize = allowed_binsize[last];

	} else {
		for ( i = 1; i < num_allowed_binsizes; i++ ) {

			MX_DEBUG( 2,("binsize[%d] = %ld, binsize[%d] = %ld",
				i-1, allowed_binsize[i-1],
				i, allowed_binsize[i]));

			if ( original_binsize == (double) allowed_binsize[i] ) {
				new_binsize = original_binsize;

				MX_DEBUG( 2,("binsize match = %ld",
					new_binsize));

				break;		/* Exit the for() loop. */
			}

			if ( original_binsize < allowed_binsize[i] ) {

				binsize_below = allowed_binsize[i-1];
				binsize_above = allowed_binsize[i];

				MX_DEBUG( 2,
				("binsize_below = %ld, binsize_above = %ld",
					binsize_below, binsize_above));

				allowed_binsize_ratio = mx_divide_safely(
					binsize_above, binsize_below );

				binsize_ratio = mx_divide_safely(
					original_binsize, binsize_below );
				
				MX_DEBUG( 2,
			("allowed_binsize_ratio = %g, binsize_ratio = %g",
					allowed_binsize_ratio, binsize_ratio));

				if (binsize_ratio > sqrt(allowed_binsize_ratio))
				{
					new_binsize = binsize_above;
				} else {
					new_binsize = binsize_below;
				}

				break;		/* Exit the for() loop. */
			}
		}
	}

	MX_DEBUG( 2,("new_binsize = %ld", new_binsize));

	return new_binsize;
}

MX_EXPORT mx_status_type
mx_area_detector_compute_new_binning( MX_AREA_DETECTOR *ad,
				long parameter_type,
				int num_allowed_binsizes,
				long *allowed_binsize )
{
	static const char fname[] = "mx_area_detector_compute_new_binning()";

	double x_binsize, y_binsize, x_framesize, y_framesize;

	if ( parameter_type == MXLV_AD_FRAMESIZE ) {
		x_binsize = mx_divide_safely( ad->maximum_framesize[0],
						ad->framesize[0] );

		y_binsize = mx_divide_safely( ad->maximum_framesize[1],
						ad->framesize[1] );
	} else
	if ( parameter_type == MXLV_AD_BINSIZE ) {
		x_binsize = ad->binsize[0];
		y_binsize = ad->binsize[1];
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Parameter type %ld is not supported for area detector '%s'.",
			parameter_type, ad->record->name );
	}

	/* Compute the new binsizes. */

	ad->binsize[0] = mx_area_detector_compute_binsize( x_binsize,
					num_allowed_binsizes, allowed_binsize );

	ad->binsize[1] = mx_area_detector_compute_binsize( y_binsize,
					num_allowed_binsizes, allowed_binsize );

	/* Compute the matching frame sizes. */

	x_framesize = mx_divide_safely( ad->maximum_framesize[0],
						ad->binsize[0] );

	y_framesize = mx_divide_safely( ad->maximum_framesize[1],
						ad->binsize[1] );

	ad->framesize[0] = x_framesize;
	ad->framesize[1] = y_framesize;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_compute_dark_current_offset( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *dark_current_frame )
{
	static const char fname[] =
		"mx_area_detector_compute_dark_current_offset()";

	uint8_t  *dark_current_data8  = NULL;
	uint16_t *dark_current_data16 = NULL;
	uint32_t *dark_current_data32 = NULL;
	uint8_t  *bias_data8  = NULL;
	uint16_t *bias_data16 = NULL;
	uint32_t *bias_data32 = NULL;
	double raw_dark_current = 0.0;
	double bias_offset      = 0.0;
	unsigned long i, num_pixels, image_format;
	double scaled_dark_current, exposure_time_ratio, exposure_time;
	float *dark_current_offset_array;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING compute_dark_current_timing;
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* If present, discard the old version
	 * of the dark current offset array.
	 */

	if ( ad->dark_current_offset_array != NULL ) {
		mx_free( ad->dark_current_offset_array );
	}

	/* If no dark current frame is currently loaded, then we just
	 * skip over the computation of the dark current offset.
	 */

	if ( dark_current_frame == NULL ) {
		MX_DEBUG(-2,
		("%s: dark_current_frame is NULL.  Skipping...", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	image_format = MXIF_IMAGE_FORMAT(dark_current_frame);

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		dark_current_data8 = dark_current_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		dark_current_data16 = dark_current_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		dark_current_data32 = dark_current_frame->image_data;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Dark current correction is not supported for image format %lu "
		"used by area detector '%s'.", image_format, ad->record->name );
	}

	if ( bias_frame != NULL ) {
		if ( image_format != MXIF_IMAGE_FORMAT(bias_frame) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The bias frame image format %lu for area detector '%s'"
			" does not match the dark current image format %lu.",
				(unsigned long) MXIF_IMAGE_FORMAT(bias_frame),
				ad->record->name,
				image_format );
		}

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			bias_data8 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			bias_data16 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			bias_data32 = bias_frame->image_data;
			break;
		}
	}

	/* Compute the exposure time ratio. */

	if ( ad->use_scaled_dark_current == FALSE ) {
		exposure_time_ratio = 1.0;
	} else {
		/* Get the exposure time for the currently selected sequence. */

		sp = &(ad->sequence_parameters);

		mx_status = mx_sequence_get_exposure_time( sp, 0,
							&exposure_time );
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		exposure_time_ratio = mx_divide_safely( exposure_time,
					ad->dark_current_exposure_time );
	}

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	MX_DEBUG(-2,("%s: exposure_time_ratio = %g",
		fname, exposure_time_ratio));
#endif

	/* Allocate memory for the dark current offset array. */

	num_pixels = MXIF_ROW_FRAMESIZE(dark_current_frame)
			* MXIF_COLUMN_FRAMESIZE(dark_current_frame);
	
	dark_current_offset_array = malloc( num_pixels * sizeof(float) );

	if ( dark_current_offset_array == (float *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu-element array "
		"of dark current offset values for area detector '%s'.",
			num_pixels, ad->record->name );
	}

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_START(compute_dark_current_timing);
#endif

	for ( i = 0; i < num_pixels; i++ ) {

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			raw_dark_current = dark_current_data8[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			raw_dark_current = dark_current_data16[i];
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			raw_dark_current = dark_current_data32[i];
			break;
		}

		if ( bias_frame == NULL ) {
			bias_offset = 0;
		} else {
			switch( image_format ) {
			case MXT_IMAGE_FORMAT_GREY8:
				bias_offset = bias_data8[i];
				break;
			case MXT_IMAGE_FORMAT_GREY16:
				bias_offset = bias_data16[i];
				break;
			case MXT_IMAGE_FORMAT_GREY32:
				bias_offset = bias_data32[i];
				break;
			}
		}

		scaled_dark_current = exposure_time_ratio
			* ( raw_dark_current - bias_offset ) + bias_offset;

		dark_current_offset_array[i] =
			bias_offset - scaled_dark_current;
			
#if MX_AREA_DETECTOR_DEBUG_CORRECTION
		if ( i < 10 ) {
			fprintf( stderr,
	    "i = %lu, scaled_dark_current = %g = %g * ( %g - %g ) + %g, ",
		    		i, scaled_dark_current,
				exposure_time_ratio, raw_dark_current,
				bias_offset, bias_offset );
			fprintf( stderr,
			"dark_current_offset_array[%lu] = %g\n",
				i, dark_current_offset_array[i] );
		}
#endif
	}

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(compute_dark_current_timing);
	MX_HRT_RESULTS(compute_dark_current_timing, fname,
			"for computing new dark current offset array.");
#endif

	/* Save a pointer to the new dark current offset array. */

	ad->dark_current_offset_array = dark_current_offset_array;

#if MX_AREA_DETECTOR_DEBUG_CORRECTION
	fprintf(stderr, "ad->dark_current_offset_array = ");

	for ( i = 0; i < 10; i++ ) {
		fprintf(stderr, "%g ", ad->dark_current_offset_array[i]);
	}
	fprintf(stderr, "\n");
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_compute_flood_field_scale( MX_AREA_DETECTOR *ad,
					MX_IMAGE_FRAME *bias_frame,
					MX_IMAGE_FRAME *flood_field_frame )
{
	static const char fname[] =
		"mx_area_detector_compute_flood_field_scale()";

	uint8_t  *flood_field_data8  = NULL;
	uint16_t *flood_field_data16 = NULL;
	uint32_t *flood_field_data32 = NULL;
	uint8_t  *bias_data8  = NULL;
	uint16_t *bias_data16 = NULL;
	uint32_t *bias_data32 = NULL;
	double raw_flood_field = 0.0;
	double bias_offset     = 0.0;
	unsigned long i, num_pixels, image_format;
	double ffs_numerator, ffs_denominator, bias_average;
	float *flood_field_scale_array;
	mx_bool_type flood_less_than_or_equal_to_bias;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING compute_flood_field_timing;
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* If present, discard the old version
	 * of the flood field scale array.
	 */

	if ( ad->flood_field_scale_array != NULL ) {
		mx_free( ad->flood_field_scale_array );
	}

	/* If no flood field frame is currently loaded, then we just
	 * skip over the computation of the flood field scale.
	 */

	if ( flood_field_frame == NULL ) {
		MX_DEBUG(-2,
		("%s: flood_field_frame is NULL.  Skipping...", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	image_format = MXIF_IMAGE_FORMAT(flood_field_frame);

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		flood_field_data8 = flood_field_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		flood_field_data16 = flood_field_frame->image_data;
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		flood_field_data32 = flood_field_frame->image_data;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Flood field correction is not supported for image format %lu "
		"used by area detector '%s'.", image_format, ad->record->name );
	}

	if ( bias_frame == NULL ) {
		flood_less_than_or_equal_to_bias = FALSE;
	} else {
		flood_less_than_or_equal_to_bias = TRUE;

		if ( image_format != MXIF_IMAGE_FORMAT(bias_frame) ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"The bias frame image format %lu for area detector "
			"'%s' does not match the flood field image format %lu.",
				(unsigned long) MXIF_IMAGE_FORMAT(bias_frame),
				ad->record->name,
				image_format );
		}

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			bias_data8 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			bias_data16 = bias_frame->image_data;
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			bias_data32 = bias_frame->image_data;
			break;
		}
	}

	num_pixels = MXIF_ROW_FRAMESIZE(flood_field_frame)
			* MXIF_COLUMN_FRAMESIZE(flood_field_frame);
	
	flood_field_scale_array = malloc( num_pixels * sizeof(float) );

	if ( flood_field_scale_array == (float *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu-element array "
		"of flood field scale factors for area detector '%s'.",
			num_pixels, ad->record->name );
	}

	/* This loop _must_ _not_ invoke any functions.  Function calls
	 * have too high an overhead to be used in a loop that may loop
	 * 32 million times or more.
	 */

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_START(compute_flood_field_timing);
#endif

	for ( i = 0; i < num_pixels; i++ ) {

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			raw_flood_field = flood_field_data8[i];
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			raw_flood_field = flood_field_data16[i];
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			raw_flood_field = flood_field_data32[i];
			break;
		}

		if ( bias_frame == NULL ) {
			bias_offset = 0;
			bias_average = 0;
		} else {
			switch( image_format ) {
			case MXT_IMAGE_FORMAT_GREY8:
				bias_offset = bias_data8[i];
				break;
			case MXT_IMAGE_FORMAT_GREY16:
				bias_offset = bias_data16[i];
				break;
			case MXT_IMAGE_FORMAT_GREY32:
				bias_offset = bias_data32[i];
				break;
			}

			bias_average = ad->bias_average_intensity;
		}

		/* We _must_ _not_ use mx_divide_safely() here since
		 * function calls have too high of an overhead to
		 * be used in this loop.
		 */

		if ( raw_flood_field <= bias_offset ) {
			flood_field_scale_array[i] = 0.0;
			continue;	/* Skip to the next pixel. */
		} else {
			flood_less_than_or_equal_to_bias = FALSE;
		}

		ffs_denominator = raw_flood_field - bias_offset;

		ffs_numerator = ad->flood_field_average_intensity
					- bias_average;

		flood_field_scale_array[i]
				= ffs_numerator / ffs_denominator;

		if ( flood_field_scale_array[i] > ad->flood_field_scale_max ){
			flood_field_scale_array[i] = 0.0;
		} else
		if ( flood_field_scale_array[i] < ad->flood_field_scale_min ){
			flood_field_scale_array[i] = 0.0;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(compute_flood_field_timing);
	MX_HRT_RESULTS(compute_flood_field_timing, fname,
			"for computing new flood field scale array.");
#endif

	if ( flood_less_than_or_equal_to_bias ) {
		mx_free( flood_field_scale_array );

		return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"Every pixel in the flood field frame '%s' for "
		"area detector '%s' is smaller than the value of the "
		"corresponding pixel in the bias frame '%s'.",
			ad->flood_field_filename,
			ad->record->name,
			ad->bias_filename );
	}

	/* Save a pointer to the new flood field scale array. */

	ad->flood_field_scale_array = flood_field_scale_array;

#if 0
	fprintf(stderr, "ad->flood_field_scale_array = ");

	for ( i = 0; i < 100; i++ ) {
		if ( (i % 10) == 0 ) {
			fprintf(stderr, "\n");
		}

		fprintf(stderr, "%g ", ad->flood_field_scale_array[i]);
	}
	fprintf(stderr, "...\n");
#endif

	return MX_SUCCESSFUL_RESULT;
}

