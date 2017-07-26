/*
 * Name:    mx_area_detector.c
 *
 * Purpose: Functions for reading frames from an area detector.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG    			FALSE

#define MX_AREA_DETECTOR_DEBUG_MX_IMAGE_ALLOC		FALSE

#define MX_AREA_DETECTOR_DEBUG_FRAME_TIMING		FALSE

#define MX_AREA_DETECTOR_DEBUG_LOAD_SAVE_FRAMES		FALSE

#define MX_AREA_DETECTOR_DEBUG_FRAME_PARAMETERS		FALSE

#define MX_AREA_DETECTOR_DEBUG_IMAGE_FRAME_DATA		FALSE

#define MX_AREA_DETECTOR_DEBUG_STATUS			FALSE

#define MX_AREA_DETECTOR_DEBUG_VCTEST			FALSE

#define MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION	FALSE

/*---*/

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING	TRUE

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE	TRUE

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP	TRUE

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMESTAMP  TRUE

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_FILE	TRUE

#define MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_FAILURE  TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt_debug.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_dirent.h"
#include "mx_bit.h"
#include "mx_array.h"
#include "mx_io.h"
#include "mx_motor.h"
#include "mx_digital_output.h"
#include "mx_relay.h"
#include "mx_rs232.h"
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
mx_area_detector_initialize_driver( MX_DRIVER *driver,
				long *maximum_num_rois_varargs_cookie )
{
	static const char fname[] = "mx_area_detector_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}
	if ( maximum_num_rois_varargs_cookie == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"maximum_num_rois_varargs_cookie pointer passed was NULL." );
	}

	/*** Construct a varargs cookie for 'maximum_num_rois'. ***/

	mx_status = mx_find_record_field_defaults_index( driver,
			"maximum_num_rois", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				maximum_num_rois_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* 'roi_array' depends on 'maximum_num_rois'. */

	mx_status = mx_find_record_field_defaults( driver,
						"roi_array", &field );

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
	MX_RECORD_FIELD *extended_status_field;
	MX_RECORD_FIELD *last_frame_number_field;
	MX_RECORD_FIELD *total_num_frames_field;
	MX_RECORD_FIELD *status_field;
	MX_LIST_HEAD *list_head;
	unsigned long ad_flags;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->correction_flags = ad->initial_correction_flags & MXFT_AD_ALL;

	ad->correction_measurement = NULL;

	ad->correction_frames_are_unbinned = FALSE;

	ad->correction_frames_to_skip = 0;

	ad->correction_measurement_sequence_type = MXT_SQ_ONE_SHOT;

	ad->byte_order = (long) mx_native_byteorder();

	ad->trigger_mode = MXT_IMAGE_NO_TRIGGER;
	ad->exposure_mode = MXF_AD_STILL_MODE;

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = 0;
	ad->status = 0;
	ad->extended_status[0] = '\0';
	ad->latched_status = 0;

	ad->arm     = FALSE;
	ad->trigger = FALSE;
	ad->stop    = FALSE;
	ad->abort   = FALSE;
	ad->busy    = FALSE;

	ad->correction_measurement_in_progress = FALSE;

	ad->current_num_rois = ad->maximum_num_rois;
	ad->roi_number = 0;
	ad->roi[0] = 0;
	ad->roi[1] = 0;
	ad->roi[2] = 0;
	ad->roi[3] = 0;

	ad->roi_frame = NULL;
	ad->roi_frame_buffer = NULL;

	ad->image_data_available = TRUE;

	ad->image_frame = NULL;

	ad->image_frame_header_length = 0;
	ad->image_frame_header = NULL;
	ad->image_frame_data = NULL;

	ad->transfer_destination_frame_ptr = NULL;
	ad->dezinger_threshold = DBL_MAX;

	ad->resolution[0] = 0.0;
	ad->resolution[1] = 0.0;

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

	memset( ad->sequence_one_shot, 0, sizeof(ad->sequence_one_shot) );
	memset( ad->sequence_stream, 0, sizeof(ad->sequence_stream) );
	memset( ad->sequence_multiframe, 0, sizeof(ad->sequence_multiframe) );
	memset( ad->sequence_strobe, 0, sizeof(ad->sequence_strobe) );
	memset( ad->sequence_duration, 0, sizeof(ad->sequence_duration) );
	memset( ad->sequence_gated, 0, sizeof(ad->sequence_gated) );

	ad->dark_current_frame = NULL;
	ad->dark_current_frame_buffer = NULL;

	ad->flat_field_average_intensity = 1.0;
	ad->bias_average_intensity = 1.0;

	ad->flat_field_frame = NULL;
	ad->flat_field_frame_buffer = NULL;

	ad->flat_field_scale_max = DBL_MAX;
	ad->flat_field_scale_min = DBL_MIN;

	ad->rebinned_mask_frame = NULL;
	ad->rebinned_bias_frame = NULL;
	ad->rebinned_dark_current_frame = NULL;
	ad->rebinned_flat_field_frame = NULL;

	ad->dark_current_offset_array = NULL;
	ad->dark_current_offset_can_change = TRUE;
	ad->old_exposure_time = -1.0;

	ad->flat_field_scale_array = NULL;
	ad->flat_field_scale_can_change = TRUE;

	ad->correction_calc_frame = NULL;

	ad->show_image_frame_min = 0;
	ad->show_image_frame_max = 65535;

	ad->datafile_directory[0] = '\0';
	ad->datafile_pattern[0] = '\0';
	ad->datafile_name[0] = '\0';
	ad->datafile_number = -1;

	ad->datafile_allow_overwrite = FALSE;
	ad->datafile_autoselect_number = TRUE;

	ad->datafile_load_format = 0;
	ad->datafile_save_format = 0;
	ad->correction_load_format = 0;
	ad->correction_save_format = 0;

	ad->datafile_load_format_name[0] = '\0';
	ad->datafile_save_format_name[0] = '\0';
	ad->correction_load_format_name[0] = '\0';
	ad->correction_save_format_name[0] = '\0';

	ad->datafile_total_num_frames = 0;
	ad->datafile_last_frame_number = 0;
	ad->datafile_management_handler = NULL;
	ad->datafile_management_callback = NULL;

	ad->inhibit_autosave = FALSE;

	ad->oscillation_motor_name[0] = '\0';
	ad->shutter_name[0] = '\0';

	ad->oscillation_distance = 0.0;
	ad->shutter_time = 0.0;
	ad->trigger_oscillation = FALSE;

	ad->oscillation_motor_record = NULL;
	ad->last_oscillation_motor_name[0] = '\0';

	ad->shutter_record = NULL;
	ad->last_shutter_name[0] = '\0';

	ad->transfer_image_during_scan = TRUE;

	ad->image_log_file = NULL;
	ad->image_log_filename[0] = '\0';

	ad->filename_log_record = NULL;
	ad->filename_log[0] = '\0';

	ad->dictionary_record = NULL;
	ad->dictionary = NULL;

	/*-------*/

	mx_status = mx_find_record_field( record, "extended_status",
						&extended_status_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->extended_status_field_number = extended_status_field->field_number;

	/*---*/

	mx_status = mx_find_record_field( record, "last_frame_number",
						&last_frame_number_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->last_frame_number_field_number =
			last_frame_number_field->field_number;

	/*---*/

	mx_status = mx_find_record_field( record, "total_num_frames",
						&total_num_frames_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->total_num_frames_field_number =
			total_num_frames_field->field_number;

	/*---*/

	mx_status = mx_find_record_field( record, "status", &status_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->status_field_number = status_field->field_number;

	/*-------*/

	strlcpy(ad->image_format_name, "DEFAULT", MXU_IMAGE_FORMAT_NAME_LENGTH);

	mx_status = mx_image_get_image_format_type_from_name(
			ad->image_format_name, &(ad->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->correction_calc_format = ad->image_format;

	ad->mask_image_format = ad->image_format;
	ad->bias_image_format = ad->image_format;
	ad->dark_current_image_format = ad->image_format;
	ad->flat_field_image_format = ad->image_format;

	ad->measure_dark_current_correction_flags = 0x3;
	ad->measure_flat_field_correction_flags  = 0x7;

	ad_flags = ad->area_detector_flags;

	if ( ad_flags & MXF_AD_GEOM_CORR_AFTER_FLAT_FIELD ) {
		ad->geom_corr_after_flat_field = TRUE;
	} else {
		ad->geom_corr_after_flat_field = FALSE;
	}

	if ( ad_flags & MXF_AD_CORRECTION_FRAME_GEOM_CORR_LAST )
	{
		ad->correction_frame_geom_corr_last = TRUE;
	} else {
		ad->correction_frame_geom_corr_last = FALSE;
	}

	if ( ad_flags & MXF_AD_CORRECTION_FRAME_NO_GEOM_CORR )
	{
		ad->correction_frame_no_geom_corr = TRUE;
	} else {
		ad->correction_frame_no_geom_corr = FALSE;
	}

	if ( ad_flags & MXF_AD_BIAS_CORR_AFTER_FLAT_FIELD ) {
		ad->bias_corr_after_flat_field = TRUE;
	} else {
		ad->bias_corr_after_flat_field = FALSE;
	}

	if ( ad_flags & MXF_AD_DEZINGER_CORRECTION_FRAME ) {
		ad->dezinger_correction_frame = TRUE;
	} else {
		ad->dezinger_correction_frame = FALSE;
	}

	/*-------*/

	/* If we are running in single-process mode (no client/server)
	 * and the flag MXF_AD_DO_NOT_SAVE_FRAME_IN_SINGLE_PROCESS_MODE is
	 * set, then we force off the MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
	 * flag here.  This is done to make single process debugging easier.
	 */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the running database is NULL." );
	}

	if ( list_head->is_server == FALSE ) {
		if (ad_flags & MXF_AD_DO_NOT_SAVE_FRAME_IN_SINGLE_PROCESS_MODE)
		{
			ad_flags &= ( ~ MXF_AD_SAVE_FRAME_AFTER_ACQUISITION );

			ad->area_detector_flags = ad_flags;
		}
	}

#if 0
	MX_DEBUG(-2,("%s: ad->area_detector_flags = %#lx",
		fname, ad->area_detector_flags));
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
	if ( strlen( register_name ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The register name passed was of zero length." );
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
	if ( strlen( register_name ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The register name passed was of zero length." );
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

#if MX_AREA_DETECTOR_DEBUG_FRAME_PARAMETERS
	MX_DEBUG(-2,("%s: ad '%s' &maximum_framesize[0] = %p",
		fname, record->name, &(ad->maximum_framesize)[0]));
	MX_DEBUG(-2,("%s: ad '%s' &maximum_framesize[1] = %p",
		fname, record->name, &(ad->maximum_framesize)[1]));

	MX_DEBUG(-2,("%s: ad '%s' maximum_framesize = (%lu,%lu)",
		fname, record->name,
		ad->maximum_framesize[0], ad->maximum_framesize[1] ));
#endif

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

#if MX_AREA_DETECTOR_DEBUG_FRAME_PARAMETERS
	MX_DEBUG(-2,("%s: ad '%s' &framesize[0] = %p",
		fname, record->name, &(ad->framesize)[0]));
	MX_DEBUG(-2,("%s: ad '%s' &framesize[1] = %p",
		fname, record->name, &(ad->framesize)[1]));

	MX_DEBUG(-2,("%s: ad '%s' framesize = (%lu,%lu)",
		fname, record->name, ad->framesize[0], ad->framesize[1] ));
#endif

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

#if MX_AREA_DETECTOR_DEBUG_FRAME_PARAMETERS
	MX_DEBUG(-2,("%s: ad '%s' &binsize[0] = %p",
		fname, record->name, &(ad->binsize)[0]));
	MX_DEBUG(-2,("%s: ad '%s' &binsize[1] = %p",
		fname, record->name, &(ad->binsize)[1]));

	MX_DEBUG(-2,("%s: ad '%s' binsize = (%lu,%lu)",
		fname, record->name, ad->binsize[0], ad->binsize[1] ));
#endif

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

	ad->subframe_size = (long) num_columns;

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
mx_area_detector_get_bits_per_pixel( MX_RECORD *record, long *bits_per_pixel )
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

MX_EXPORT mx_status_type
mx_area_detector_get_bytes_per_image_file( MX_RECORD *record,
					unsigned long datafile_type,
					size_t *bytes_per_image_file )
{
	static const char fname[] =
		"mx_area_detector_get_bytes_per_image_file()";

	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the area detector image frame data structures
	 * by asking the MX server to transfer it's currently loaded
	 * image.  This will cause all the necessary data structures
	 * to be set up correctly here on the client side as a side
	 * effect.
	 */

	mx_status = mx_area_detector_transfer_frame( record,
						MXFT_AD_IMAGE_FRAME,
						&(ad->image_frame) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now extract the predicted datafile size and return it to
	 * our caller.
	 */

	mx_status = mx_image_get_filesize( ad->image_frame,
					datafile_type,
					bytes_per_image_file );

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

	/* As a side effect, set the value of ad->exposure_time.
	 * This gives external code that reads the exposure time an
	 * exposure time that matches the most recently set value.
	 */

	ad->exposure_time = exposure_time;

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

/*--------*/

MX_EXPORT mx_status_type
mx_area_detector_get_exposure_time( MX_RECORD *record,
					double *exposure_time )
{
	static const char fname[] = "mx_area_detector_get_exposure_time()";

	MX_AREA_DETECTOR *ad;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_exposure_time( sp, 0, &(ad->exposure_time));

	if ( exposure_time != (double *) NULL ) {
		*exposure_time = ad->exposure_time;
	}

	return mx_status;
}

/*--------*/

MX_EXPORT mx_status_type
mx_area_detector_get_num_exposures( MX_RECORD *record,
					long *num_exposures )
{
	static const char fname[] = "mx_area_detector_get_num_exposures()";

	MX_AREA_DETECTOR *ad;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sp = &(ad->sequence_parameters);

	mx_status = mx_sequence_get_num_frames( sp, &(ad->num_exposures));

	if ( num_exposures != (long *) NULL ) {
		*num_exposures = ad->num_exposures;
	}

	return mx_status;
}

/*--------*/

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
mx_area_detector_set_stream_mode( MX_RECORD *record, double exposure_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_STREAM;
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
mx_area_detector_set_duration_mode( MX_RECORD *record,
					long num_frames )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_DURATION;
	seq_params.num_parameters = 1;
	seq_params.parameter_array[0] = num_frames;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_gated_mode( MX_RECORD *record,
				long num_frames,
				double exposure_time,
				double gate_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_GATED;
	seq_params.num_parameters = 3;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = gate_time;

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
mx_area_detector_get_exposure_mode( MX_RECORD *record, long *exposure_mode )
{
	static const char fname[] = "mx_area_detector_get_exposure_mode()";

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

	ad->parameter_type = MXLV_AD_EXPOSURE_MODE; 

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( exposure_mode != (long *) NULL ) {
		*exposure_mode = ad->exposure_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_exposure_mode( MX_RECORD *record, long exposure_mode )
{
	static const char fname[] = "mx_area_detector_set_exposure_mode()";

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

	ad->parameter_type = MXLV_AD_EXPOSURE_MODE; 

	ad->exposure_mode = exposure_mode;

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
	unsigned long ad_flags;
	unsigned long corr_flags;
	double dark_current_exposure_time, sequence_exposure_time;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad_flags = ad->area_detector_flags;

	corr_flags = ad->correction_flags;

	if ( (ad_flags & MXF_AD_BYPASS_DARK_CURRENT_EXPOSURE_TIME_TEST) == 0 ) {

	    if ( ad->use_scaled_dark_current == FALSE ) {

		/* If we are not using a scaled dark current, then
		 * check to see if there is an exposure time conflict.
		 */

		if ( ad->dark_current_frame != (MX_IMAGE_FRAME *) NULL ) {

		    /* Only check for exposure time conflict if the user
		     * has requested us to subtract a dark current image.
		     */

		    if ( (corr_flags & MXFT_AD_DARK_CURRENT_FRAME) != 0 ) {

			mx_status = mx_image_get_exposure_time(
						ad->dark_current_frame,
						&dark_current_exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
			    return mx_status;

			mx_status = mx_area_detector_get_exposure_time(
					record, &sequence_exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
			    return mx_status;

			if ( mx_difference( dark_current_exposure_time,
					     sequence_exposure_time ) > 0.001 )
			{
			   ad->latched_status |= MXSF_AD_EXPOSURE_TIME_CONFLICT;

			   return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Area detector '%s' is configured for a "
				"sequence exposure time (%g seconds) that "
				"is different from the exposure time for "
				"the currently loaded dark current "
				"image (%g seconds).",
					record->name,
					sequence_exposure_time,
					dark_current_exposure_time );
			}
		    }
		}
	    }
	}

	if ( ad_flags & MXF_AD_AUTOMATICALLY_CREATE_DIRECTORY_HIERARCHY ) {

	    /* Make sure that the user's requested directory exists
	     * or else abort the 'arm'.
	     */

	    mx_status = mx_make_directory_hierarchy( ad->datafile_directory );

	    if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
	}

	ad->stop = FALSE;
	ad->abort = FALSE;

	ad->latched_status = 0;

	/* If automatic saving or loading of datafiles has been 
	 * configured, then we need to get and save the current
	 * value of 'total_num_frames'.  We do this so that when
	 * the datafile management handler function is invoked,
	 * we will know how many datafiles to save or load.
	 */

	if (ad->datafile_management_handler != NULL ) {
		mx_status = mx_area_detector_get_total_num_frames( record,
					&(ad->datafile_total_num_frames) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		ad->datafile_last_frame_number = 0;

#if ( MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP \
	|| MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMESTAMP )
		MX_DEBUG(-2,
		("%s: area detector '%s', datafile_total_num_frames = %ld",
			fname, record->name, ad->datafile_total_num_frames));
#endif
	}

	/* If requested, write a 'start' message to the image log file
	 * before we arm the detector.
	 */

	if ( ad->image_log_file != (FILE *) NULL ) {
		if ( ( ad->correction_measurement_in_progress == FALSE )
		  && ( ad_flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) )
		{
			unsigned long first_datafile_number;
			long num_exposures;
			char timestamp[40];

			ad->image_log_error_seen = FALSE;

			first_datafile_number = ad->datafile_number + 1;

			mx_status = mx_area_detector_get_num_exposures(
						record, &num_exposures );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_timestamp( timestamp, sizeof(timestamp) );

			fprintf( ad->image_log_file, "%s start %lu %ld %s/%s\n",
				timestamp,
				first_datafile_number,
				num_exposures,
				ad->datafile_directory,
				ad->datafile_pattern );

			fflush( ad->image_log_file );
		}
	}

	/* Arm the area detector. */

	ad->arm = 1;

	arm_fn = flist->arm;

	if ( arm_fn != NULL ) {
		mx_status = (*arm_fn)( ad );

		ad->arm = 0;

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	ad->arm = 0;

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

	ad->trigger = 1;

	trigger_fn = flist->trigger;

	if ( trigger_fn == NULL ) {
		ad->trigger = 0;

		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Triggering the taking of image frames has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

	mx_status = (*trigger_fn)( ad );

	ad->trigger = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_start( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_start()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *start_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = flist->start;

	if ( start_fn != NULL ) {
		ad->start = 1;

		mx_status = (*start_fn)( ad );

		ad->start = 0;
	} else {
		mx_status = mx_area_detector_arm( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_trigger( record );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_stop( MX_RECORD *record )
{
	static const char fname[] = "mx_area_detector_stop()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *stop_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *abort_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->stop = TRUE;

	stop_fn = flist->stop;

	if ( stop_fn == NULL ) {
		abort_fn = flist->abort;

		if ( abort_fn != NULL ) {
			mx_status = (*abort_fn)( ad );
		}
	} else {
		mx_status = (*stop_fn)( ad );
	}

	if ( ad->correction_measurement != NULL ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
	}

	/* If requested, write a message to the image log file. */

	if ( ad->image_log_file != (FILE *) NULL ) {
		char timestamp[40];

		mx_timestamp( timestamp, sizeof(timestamp) );

		fprintf( ad->image_log_file, "%s stop %lu\n",
			timestamp, ad->datafile_number );

		fflush( ad->image_log_file );
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
	mx_status_type ( *stop_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->abort = TRUE;

	abort_fn = flist->abort;

	if ( abort_fn == NULL ) {
		stop_fn = flist->stop;

		if ( stop_fn != NULL ) {
			mx_status = (*stop_fn)( ad );
		}
	} else {
		mx_status = (*abort_fn)( ad );
	}

	if ( ad->correction_measurement != NULL ) {
		mx_area_detector_cleanup_after_correction( ad, NULL );
	}

	/* If requested, write a message to the image log file. */

	if ( ad->image_log_file != (FILE *) NULL ) {
		char timestamp[40];

		mx_timestamp( timestamp, sizeof(timestamp) );

		fprintf( ad->image_log_file, "%s abort %lu\n",
			timestamp, ad->datafile_number );

		fflush( ad->image_log_file );
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

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
		}
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
		}
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the last frame number for area detector '%s' "
		"is unsupported.", record->name );
	}

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

#if MX_AREA_DETECTOR_DEBUG_STATUS
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx, "
	"correction_measurement_in_progress = %d, correction_measurement = %p",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status,
		ad->correction_measurement_in_progress,
		ad->correction_measurement));
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

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
		}
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );

		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
		}
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the total number of frames for area detector '%s' "
		"is unsupported.", record->name );
	}

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

#if MX_AREA_DETECTOR_DEBUG_STATUS
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx, "
	"correction_measurement_in_progress = %d, correction_measurement = %p",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status,
		ad->correction_measurement_in_progress,
		ad->correction_measurement));
#endif

	if ( total_num_frames != NULL ) {
		*total_num_frames = ad->total_num_frames;
	}

	/* If a datafile management handler has been installed, but there is
	 * no datafile management callback that is currently active, then we
	 * we must explicitly invoke the datafile management handler now.
	 */

#if 0 && MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
	MX_DEBUG(-2,("%s: ad->datafile_management_handler = %p",
		fname, ad->datafile_management_handler));
	MX_DEBUG(-2,("%s: ad->datafile_management_callback = %p",
		fname, ad->datafile_management_callback));
	MX_DEBUG(-2,
	("%s: total_num_frames = %ld, datafile_total_num_frames = %ld",
		fname, ad->total_num_frames, ad->datafile_total_num_frames));
#endif

	if ( (ad->datafile_management_handler != NULL)
	  && (ad->datafile_management_callback == NULL)
	  && (ad->total_num_frames > ad->datafile_total_num_frames) )
	{
		mx_status = (*ad->datafile_management_handler)(record);
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

	/* If the image log file is enabled and an error occurred,
	 * then write a message about that error to the log file.
	 */

	if ( ad->image_log_file != (FILE *) NULL ) {
		if ( mx_status.code != MXE_SUCCESS ) {
			(void) mx_area_detector_image_log_show_error(
							ad, mx_status );
		}
	}

	/*---*/

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

#if 0
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, datafile_last_frame_number = %ld",
		fname, ad->last_frame_number, ad->datafile_last_frame_number));
#endif

#if 0
	{
		unsigned long flags;

		flags = ad->area_detector_flags;

		/* FIXME: Having this work would be useful. */

		if ( flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {
			if ( ad->last_frame_number > 
				(ad->datafile_last_frame_number - 1) )
			{
				ad->status |= MXSF_AD_UNSAVED_IMAGE_FRAMES;
			}
		}
	}
#endif

	/* Set the bits from the latched status word in the main status word. */

	ad->status |= ad->latched_status;

#if MX_AREA_DETECTOR_DEBUG_STATUS
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx, "
	"correction_measurement_in_progress = %d, correction_measurement = %p",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status,
		ad->correction_measurement_in_progress,
		ad->correction_measurement));
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

		if ( ad->image_log_file != (FILE *) NULL ) {
			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
			}
		}
	} else {
		if ( get_last_frame_number_fn == NULL ) {
			ad->last_frame_number = -1;
		} else {
			mx_status = (*get_last_frame_number_fn)( ad );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
				return mx_status;
			}
		}

		if ( get_total_num_frames_fn == NULL ) {
			ad->total_num_frames = 0;
		} else {
			mx_status = (*get_total_num_frames_fn)( ad );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
				return mx_status;
			}
		}

		if ( get_status_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the detector status for area detector '%s' "
			"is unsupported.", record->name );
		} else {
			mx_status = (*get_status_fn)( ad );

			if ( mx_status.code != MXE_SUCCESS ) {
				(void) mx_area_detector_image_log_show_error(
								ad, mx_status );
				return mx_status;
			}
		}
	}

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

#if 0
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, datafile_last_frame_number = %ld",
		fname, ad->last_frame_number, ad->datafile_last_frame_number));
#endif

#if 0
	{
		unsigned long flags;

		flags = ad->area_detector_flags;

		MX_DEBUG(-2,("%s: MARKER #1, status = %#lx, flags = %#lx",
			fname, ad->status, flags));

		/* FIXME: Having this work would be useful. */

		if ( flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {
			MX_DEBUG(-2,("%s: MARKER #2", fname));

			if ( ad->last_frame_number > 
				(ad->datafile_last_frame_number - 1) )
			{
				MX_DEBUG(-2,("%s: MARKER #3", fname));

				ad->status |= MXSF_AD_UNSAVED_IMAGE_FRAMES;
			}
		}

		MX_DEBUG(-2,
		("%s: MARKER #4, status = %#lx", fname, ad->status));
	}
#endif

	/* Set the bits from the latched status word in the main status word. */

	ad->status |= ad->latched_status;

#if MX_AREA_DETECTOR_DEBUG_STATUS
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx, "
	"correction_measurement_in_progress = %d, correction_measurement = %p",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status,
		ad->correction_measurement_in_progress,
		ad->correction_measurement));
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

	/* If a datafile management handler has been installed, but there is
	 * no datafile management callback that is currently active, then we
	 * we must explicitly invoke the datafile management handler now.
	 */

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
	MX_DEBUG(-2,("%s: ad->datafile_management_handler = %p",
		fname, ad->datafile_management_handler));
	MX_DEBUG(-2,("%s: ad->datafile_management_callback = %p",
		fname, ad->datafile_management_callback));
	MX_DEBUG(-2,
	("%s: total_num_frames = %ld, datafile_total_num_frames = %ld",
		fname, ad->total_num_frames, ad->datafile_total_num_frames));
#endif

	if ( (ad->datafile_management_handler != NULL)
	  && (ad->datafile_management_callback == NULL)
	  && (ad->total_num_frames > ad->datafile_total_num_frames) )
	{
		mx_status = (*ad->datafile_management_handler)(record);
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_image_log_show_error( MX_AREA_DETECTOR *ad,
					mx_status_type caller_mx_status )
{
	static const char fname[] = "mx_area_detector_image_log_show_error()";

	char timestamp[40];

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* If the image log file is not open, then return without
	 * doing anything.
	 */

	if ( ad->image_log_file == (FILE *) NULL )
		return MX_SUCCESSFUL_RESULT;

	/* If an error has already been displayed since the most recent
	 * arm() of the area detector, then we return without showing
	 * anything to prevent a torrent of error messages in the image log.
	 */

	if ( ad->image_log_error_seen )
		return MX_SUCCESSFUL_RESULT;

	/* Display the error message. */

	ad->image_log_error_seen = TRUE;

	mx_timestamp( timestamp, sizeof(timestamp) );

	fprintf( ad->image_log_file, "%s error %lu %s\n",
		timestamp,
		ad->datafile_number,
		mx_status_code_string( caller_mx_status.code ) );

	fflush( ad->image_log_file );

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_open_filename_log( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_open_filename_log()";

	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	/* If the log was already open, begin by closing it. */

	if ( ad->filename_log_record != (MX_RECORD *) NULL ) {
		mx_status = mx_area_detector_close_filename_log( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* See if we are attempting to discontinue logging. */

	if ( ad->filename_log[0] == '\0' ) {
		ad->filename_log_record = NULL;
	}

	/* Look for the requested filename log record. */

	ad->filename_log_record = mx_get_record( ad->record, ad->filename_log );

	if ( ad->filename_log_record == (MX_RECORD *) NULL ) {
		mx_status = mx_error( MXE_NOT_FOUND, fname,
		"The requested filename log record '%s' was not found "
		"in the MX database.", ad->filename_log );

		ad->filename_log[0] = '\0';

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_close_filename_log( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_close_filename_log()";

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	ad->filename_log_record = NULL;

	ad->filename_log[0] = '\0';

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_write_to_filename_log( MX_AREA_DETECTOR *ad,
					char *filename_log_message )
{
	static const char fname[] = "mx_area_detector_write_to_filename_log()";

	MX_RECORD *log_record;
	mx_bool_type logging_allowed;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	log_record = ad->filename_log_record;

	if ( log_record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	logging_allowed = TRUE;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( log_record->mx_superclass ) {
	case MXR_INTERFACE:
		switch( log_record->mx_class ) {
		case MXI_RS232:
			mx_status = mx_rs232_putline( log_record,
							filename_log_message,
							NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS ) {
				logging_allowed = FALSE;
			}
			break;
		default:
			logging_allowed = FALSE;
			break;
		}
		break;
	default:
		logging_allowed = FALSE;
		break;
	}

	if ( logging_allowed == FALSE ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"The filename log record '%s' that you used for detector '%s' "
		"is not supported for area detector filename logging.",
			log_record->name, ad->record->name );
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_setup_oscillation( MX_RECORD *ad_record,
				MX_RECORD *motor_record,
				MX_RECORD *shutter_record,
				MX_RECORD *trigger_record,
				double oscillation_distance,
				double shutter_time )
{
	static const char fname[] = "mx_area_detector_setup_oscillation()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type (*setup_oscillation)( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( ad_record,
						&ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->oscillation_motor_record = motor_record;

	if ( motor_record == (MX_RECORD *) NULL ) {
		ad->oscillation_motor_name[0] = '\0';
	} else {
		strlcpy( ad->oscillation_motor_name, motor_record->name,
			sizeof(ad->oscillation_motor_name) );
	}

	ad->shutter_record = shutter_record;

	if ( shutter_record == (MX_RECORD *) NULL ) {
		ad->shutter_name[0] = '\0';
	} else {
		strlcpy( ad->shutter_name, shutter_record->name,
			sizeof(ad->shutter_name) );
	}

	ad->oscillation_trigger_record = trigger_record;

	if ( trigger_record == (MX_RECORD *) NULL ) {
		ad->oscillation_trigger_name[0] = '\0';
	} else {
		strlcpy( ad->oscillation_trigger_name, trigger_record->name,
			sizeof(ad->oscillation_trigger_name) );
	}

	ad->oscillation_distance = oscillation_distance;

	ad->shutter_time = shutter_time;

	setup_oscillation = flist->setup_oscillation;

	if ( setup_oscillation != NULL ) {
		mx_status = (*setup_oscillation)( ad );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_trigger_oscillation( MX_RECORD *ad_record )
{
	static const char fname[] = "mx_area_detector_trigger_oscillation()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type (*trigger_oscillation)( MX_AREA_DETECTOR * );
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( ad_record,
						&ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = ad->area_detector_flags;

	if ( flags & MXF_AD_USE_UNSAFE_OSCILLATION ) {
		trigger_oscillation =
			mx_area_detector_trigger_unsafe_oscillation;
	} else {
		trigger_oscillation = flist->trigger_oscillation;
	}

	if ( trigger_oscillation == NULL ) {
		if ( ad->oscillation_trigger_record != (MX_RECORD *) NULL ) {
			trigger_oscillation =
				mx_area_detector_send_oscillation_trigger_pulse;
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' does not support "
			"oscillations coordinated with a shutter and a motor.",
				ad_record->name );
		}
	}

	mx_status = (*trigger_oscillation)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_wait_for_image_complete( MX_RECORD *record, double timeout )
{
	static const char fname[] =
		"mx_area_detector_wait_for_image_complete()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_CLOCK_TICK current_tick, finish_tick, timeout_ticks;
	int comparison;
	mx_bool_type check_for_timeout;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( timeout < 0.0 ) {
		check_for_timeout = FALSE;
	} else {
		check_for_timeout = TRUE;

		current_tick = mx_current_clock_tick();

		timeout_ticks = mx_convert_seconds_to_clock_ticks( timeout );

		finish_tick = mx_add_clock_ticks( current_tick,
						timeout_ticks );
#if 0
		MX_DEBUG(-2,("%s: timeout_ticks = (%lu,%lu)", fname,
			timeout_ticks.high_order, timeout_ticks.low_order));

		MX_DEBUG(-2,
		("%s: current_tick = (%lu,%lu), finish_tick = (%lu,%lu)",
			fname, current_tick.high_order, current_tick.low_order,
			finish_tick.high_order, finish_tick.low_order));
#endif
	}

	/* Wait for the area detector to finish acquiring its image frame. */

	for(;;) {
		mx_status = mx_area_detector_get_status( ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( ad->status & MXSF_AD_IS_BUSY ) == 0 ) {
			break;		/* Exit the for(;;) loop. */
		}

		if ( check_for_timeout ) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								finish_tick );
#if 0
			MX_DEBUG(-2,
    ("%s: finish_tick = (%lu,%lu), current_tick = (%lu,%lu), comparison = %d",
			fname, finish_tick.high_order, finish_tick.low_order,
			current_tick.high_order, current_tick.low_order,
			comparison));
#endif
			if ( comparison > 0 ) {
				return mx_error( MXE_TIMED_OUT, fname,
				"Timed out after waiting %f seconds for the "
				"exposure to end for area detector '%s'.",
					timeout, record->name );
			}
		}

		mx_msleep(10);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_wait_for_oscillation_complete( MX_RECORD *record,
					double timeout )
{
	static const char fname[] =
		"mx_area_detector_wait_for_oscillation_complete()";

	MX_AREA_DETECTOR *ad = NULL;
	MX_DIGITAL_OUTPUT *doutput = NULL;
	MX_RELAY *relay = NULL;
	MX_MOTOR *motor = NULL;

	MX_CLOCK_TICK current_tick, finish_tick, oscillation_ticks;
	int comparison;
	mx_bool_type check_for_timeout, exit_loop;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = ad->oscillation_motor_record->record_class_struct;

	switch( ad->shutter_record->mx_class ) {
	case MXC_DIGITAL_OUTPUT:
		doutput = ad->shutter_record->record_class_struct;
		break;
	case MXC_RELAY:
		relay = ad->shutter_record->record_class_struct;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Area detector '%s' shutter record '%s' is not a "
		"digital output or relay record.",
			record->name, ad->shutter_record->name );
		break;
	}

	if ( timeout < 0.0 ) {
		check_for_timeout = FALSE;
	} else {
		check_for_timeout = TRUE;

		oscillation_ticks =
			mx_convert_seconds_to_clock_ticks( ad->shutter_time );

		current_tick = mx_current_clock_tick();

		finish_tick = mx_add_clock_ticks( current_tick,
						oscillation_ticks );
#if 0
		MX_DEBUG(-2,("%s: oscillation_ticks = (%lu,%lu)", fname,
			oscillation_ticks.high_order,
			oscillation_ticks.low_order));

		MX_DEBUG(-2,
		("%s: current_tick = (%lu,%lu), finish_tick = (%lu,%lu)",
			fname, current_tick.high_order, current_tick.low_order,
			finish_tick.high_order, finish_tick.low_order));
#endif
	}

	/* Wait for the motor to stop moving. */

	for(;;) {
		mx_status = mx_motor_get_status(
				ad->oscillation_motor_record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( motor->status & MXSF_MTR_IS_BUSY ) == 0 ) {
			break;		/* Exit the for(;;) loop. */
		}

		if ( check_for_timeout ) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								finish_tick );

			if ( comparison > 0 ) {
				return mx_error( MXE_TIMED_OUT, fname,
				"Timed out after waiting %f seconds for the "
				"oscillation motor to stop moving for "
				"area detector '%s'.",
					timeout, record->name );
			}
		}

		mx_msleep(10);
	}

	/* Wait for the shutter to finish its pulse. */

	for(;;) {
		exit_loop = FALSE;

		switch( ad->shutter_record->mx_class ) {
		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_read(
					ad->shutter_record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( doutput->value == doutput->pulse_off_value ) {
				exit_loop = TRUE;
			}
			break;
		case MXC_RELAY:
			mx_status = mx_get_relay_status(
					ad->shutter_record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( relay->relay_status == relay->pulse_off_value ) {
				exit_loop = TRUE;
			}
			break;
		}

		if ( exit_loop ) {
			break;		/* Exit the for(;;) loop. */
		}

		if ( check_for_timeout ) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								finish_tick );

			if ( comparison > 0 ) {
				return mx_error( MXE_TIMED_OUT, fname,
				"Timed out after waiting %f seconds for the "
				"shutter pulse to end for area detector '%s'.",
					timeout, record->name );
			}
		}

		mx_msleep(10);
	}

	/* Wait for the area detector to finish acquiring its image frame. */

	for(;;) {
		mx_status = mx_area_detector_get_status( ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( ad->status & MXSF_AD_IS_BUSY ) == 0 ) {
			break;		/* Exit the for(;;) loop. */
		}

		if ( check_for_timeout ) {
			current_tick = mx_current_clock_tick();

			comparison = mx_compare_clock_ticks( current_tick,
								finish_tick );
#if 0
			MX_DEBUG(-2,
    ("%s: finish_tick = (%lu,%lu), current_tick = (%lu,%lu), comparison = %d",
			fname, finish_tick.high_order, finish_tick.low_order,
			current_tick.high_order, current_tick.low_order,
			comparison));
#endif
			if ( comparison > 0 ) {
				return mx_error( MXE_TIMED_OUT, fname,
				"Timed out after waiting %f seconds for the "
				"oscillation to end for area detector '%s'.",
					timeout, record->name );
			}
		}

		mx_msleep(10);
	}

	return MX_SUCCESSFUL_RESULT;
}

/* WARNING: mx_area_detector_trigger_unsafe_oscillation() is just for
 * simulation purposes.  You should not regard it as a good example of
 * implementing oscillations, since there is absolutely no synchronization.
 */

MX_EXPORT mx_status_type
mx_area_detector_trigger_unsafe_oscillation( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_trigger_unsafe_oscillation()";

	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
						ad->shutter_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ad->oscillation_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "No oscillation motor has been specified for area detector '%s'.",
			ad->record->name );
	}

	if ( ad->shutter_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"No shutter has been specified for area detector '%s'.",
			ad->record->name );
	}

	mx_status = mx_motor_move_relative( ad->oscillation_motor_record,
						ad->oscillation_distance, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ad->shutter_record->mx_class ) {
	case MXC_DIGITAL_OUTPUT:
		mx_status = mx_digital_output_pulse( ad->shutter_record,
						1, 0, ad->shutter_time );
		break;
	case MXC_RELAY:
		mx_status = mx_relay_pulse( ad->shutter_record,
						1, 0, ad->shutter_time );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Shutter record '%s' for area detector '%s' is not "
		"a digital output record or a relay record.",
			ad->shutter_record->name, ad->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_start( ad->record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_send_oscillation_trigger_pulse( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_send_oscillation_trigger_pulse()";

	mx_status_type mx_status;

#if 1 || MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name ));
#endif
	if ( ad->oscillation_trigger_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
	    "No oscillation trigger has been specified for area detector '%s'.",
			ad->record->name );
	}

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
						ad->shutter_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_arm( ad->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( ad->oscillation_trigger_record->mx_class ) {
	case MXC_RELAY:
		mx_status = mx_relay_command( ad->oscillation_trigger_record,
						MXF_CLOSE_RELAY );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The oscillation trigger record '%s' for area detector '%s' "
		"is not a relay record.", ad->oscillation_trigger_record->name,
			ad->record->name );
		break;
	}

	return mx_status;
}

/*-------------------------------------------------------------------*/

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
					ad->bytes_per_frame,
					ad->dictionary,
					ad->record );

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

/*-------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_setup_correction_frame( MX_RECORD *record,
				long image_format,
				MX_IMAGE_FRAME **image_frame )
{
	static const char fname[] = "mx_area_detector_setup_correction_frame()";

	MX_AREA_DETECTOR *ad;
	double bytes_per_pixel;
	size_t bytes_per_frame;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_TIMING setup_frame_timing;
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_image_format_get_bytes_per_pixel( image_format,
							&bytes_per_pixel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	bytes_per_frame = mx_round( bytes_per_pixel * ad->framesize[0]
						* ad->framesize[1] );

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
					image_format,
					ad->byte_order,
					bytes_per_pixel,
					ad->header_length,
					bytes_per_frame,
					ad->dictionary,
					ad->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MXIF_ROW_BINSIZE(*image_frame)    = ad->binsize[0];
	MXIF_COLUMN_BINSIZE(*image_frame) = ad->binsize[1];
	MXIF_BITS_PER_PIXEL(*image_frame) = mx_round( 8.0 * bytes_per_pixel );

#if MX_AREA_DETECTOR_DEBUG_FRAME_TIMING
	MX_HRT_END(setup_frame_timing);
	MX_HRT_RESULTS(setup_frame_timing, fname, "for frame setup.");
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------*/

/* FIXME: This function currently produces garbage results rather than
 * the expected ASCII version of the image.
 */

static mx_status_type
mxp_area_detector_display_ascii_debugging_image( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mxp_area_detector_display_ascii_debugging_image()";

	MX_IMAGE_FRAME *image;
	MX_IMAGE_FRAME *rebinned_image;
	unsigned long original_width, original_height;
	unsigned long rebinned_width, rebinned_height;
	double width_temp, rebinned_scale_factor;
	mx_status_type mx_status;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AREA_DETECTOR pointer passed was NULL." );
	}

	mx_warning( "The function %s is broken!", fname );

	rebinned_image = NULL;

	image = ad->image_frame;

	if ( image == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"No images have been taken yet by area detector '%s',.",
			ad->record->name );
	}

#if 1
	mx_image_statistics( image );
#endif

	/**********************************************************
	 * Create a rebinned version of ad->image_frame that will *
	 * fit into an 80 character wide terminal window.         *
	 **********************************************************/

	/* First compute the rebinning factor that we need to make
	 * the width of the image fit.
	 */

	original_width = MXIF_ROW_FRAMESIZE( image );

	if ( original_width < 80 ) {
		mx_status = mx_image_copy_frame( image, &rebinned_image );
	} else {
		/* Keep dividing by 2 until we get a width
		 * that is less than 80.
		 */

		width_temp = original_width;

		while ( width_temp >= 80.0 ) {
			width_temp /= 2.0;
		}

		rebinned_width = mx_round_down( width_temp );

		rebinned_scale_factor =
			mx_divide_safely( rebinned_width,
					original_width );

		original_height = MXIF_COLUMN_FRAMESIZE( image );

		rebinned_height = mx_round_down(
			rebinned_scale_factor * original_height );

		mx_status = mx_image_rebin( &rebinned_image, image,
					rebinned_width,
					rebinned_height );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/**********************************************************
	 * We send to stderr an ASCII representation of the image *
	 * that we just created.                                  *
	 **********************************************************/

#if 1
	mx_image_statistics( rebinned_image );
#endif

	mx_status = mx_image_display_ascii( stderr, rebinned_image, 0, 65535 );

	(void) mx_image_free( rebinned_image );

	return mx_status;
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

	if ( ad->area_detector_flags & MXF_AD_DISPLAY_ASCII_DEBUGGING_IMAGE ) {
		mx_status =
			mxp_area_detector_display_ascii_debugging_image( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if 0
	if ( ad->image_frame == NULL ) {
		MX_DEBUG(-2,("%s: no frame found.", fname));
	} else {
		MX_DEBUG(-2,("%s: frame timestamp = (%lu,%lu)", fname,
			MXIF_TIMESTAMP_SEC( ad->image_frame ),
			MXIF_TIMESTAMP_NSEC( ad->image_frame ) ));
	}
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

static mx_bool_type
mxp_area_detector_all_mask_pixels_are_set( MX_IMAGE_FRAME *frame )
{
	mx_bool_type mask_pixels_are_equal;
	unsigned long image_format, first_pixel_value;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return TRUE;
	}

	mask_pixels_are_equal = mx_image_all_pixels_are_equal( frame );

	if ( mask_pixels_are_equal ) {
		return FALSE;
	} else {
		image_format = MXIF_IMAGE_FORMAT( frame );

		switch( image_format ) {
		case MXT_IMAGE_FORMAT_GREY8:
			first_pixel_value = *( (uint8_t *) frame->image_data );
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			first_pixel_value = *( (uint16_t *) frame->image_data );
			break;
		case MXT_IMAGE_FORMAT_GREY32:
			first_pixel_value = *( (uint32_t *) frame->image_data );
			break;
		default:
			first_pixel_value = 0;
			break;
		}

		if ( first_pixel_value == 0 ) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
}

static double
mxp_area_detector_constant_bias_pixel_offset( MX_IMAGE_FRAME *frame )
{
	double first_pixel_value;
	unsigned long image_format;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return TRUE;
	}

	image_format = MXIF_IMAGE_FORMAT( frame );

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_GREY8:
		first_pixel_value = *( (uint8_t *) frame->image_data );
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		first_pixel_value = *( (uint16_t *) frame->image_data );
		break;
	case MXT_IMAGE_FORMAT_GREY32:
		first_pixel_value = *( (uint32_t *) frame->image_data );
		break;
	case MXT_IMAGE_FORMAT_FLOAT:
		first_pixel_value = *( (float *) frame->image_data );
		break;
	case MXT_IMAGE_FORMAT_DOUBLE:
		first_pixel_value = *( (double *) frame->image_data );
		break;
	default:
		first_pixel_value = 0;
		break;
	}

	return first_pixel_value;
}

MX_EXPORT mx_status_type
mx_area_detector_transfer_frame( MX_RECORD *record,
				long frame_type,
				MX_IMAGE_FRAME **destination_frame )
{
	static const char fname[] = "mx_area_detector_transfer_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *transfer_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

#if 0
	MX_LIST_HEAD *list_head;

	/* If we are running in an MX server, then trigger a breakpoint here. */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head->is_server ) {
		mx_breakpoint();
	}
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( destination_frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination_frame pointer passed was NULL." );
	}

	ad->transfer_destination_frame_ptr = destination_frame;

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

		destination_data_array = (*destination_frame)->image_data;

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
	case MXFT_AD_MASK_FRAME:
		if ( ad->mask_frame == NULL ) {
			ad->all_mask_pixels_are_set = TRUE;
		} else {
			ad->all_mask_pixels_are_set =
				mxp_area_detector_all_mask_pixels_are_set(
							ad->mask_frame );
		}
		break;

	case MXFT_AD_BIAS_FRAME:
		if ( ad->bias_frame == NULL ) {
			ad->all_bias_pixels_are_equal = TRUE;
			ad->constant_bias_pixel_offset = 0.0;
		} else {
			if ( mx_image_all_pixels_are_equal( ad->bias_frame ) ) {
				ad->all_bias_pixels_are_equal = TRUE;

				ad->constant_bias_pixel_offset =
				  mxp_area_detector_constant_bias_pixel_offset(
					ad->bias_frame );
			} else {
				ad->all_bias_pixels_are_equal = FALSE;

				mx_status = mx_image_get_average_intensity(
						ad->bias_frame, ad->mask_frame,
						&(ad->bias_average_intensity) );
			}
		}

		mx_free( ad->dark_current_offset_array );
		mx_free( ad->flat_field_scale_array );
		break;

	case MXFT_AD_DARK_CURRENT_FRAME:
		if ( ad->dark_current_frame != NULL ) {
			mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );
		}

		mx_free( ad->dark_current_offset_array );
		break;

	case MXFT_AD_FLAT_FIELD_FRAME:
		if ( ad->flat_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flat_field_frame, ad->mask_frame,
					&(ad->flat_field_average_intensity) );
		}

		mx_free( ad->flat_field_scale_array );
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
		if ( ad->mask_frame == NULL ) {
			ad->all_mask_pixels_are_set = TRUE;
		} else {
			ad->all_mask_pixels_are_set =
				mxp_area_detector_all_mask_pixels_are_set(
							ad->mask_frame );
		}

		if ( ad->rebinned_mask_frame != NULL ) {
			mx_image_free( ad->rebinned_mask_frame );

			ad->rebinned_mask_frame = NULL;
		}
		strlcpy( ad->mask_filename, frame_filename,
					sizeof(ad->mask_filename) );
		break;

	case MXFT_AD_BIAS_FRAME:
		if ( ad->bias_frame == NULL ) {
			ad->all_bias_pixels_are_equal = TRUE;
			ad->constant_bias_pixel_offset = 0.0;
		} else {
			if ( mx_image_all_pixels_are_equal( ad->bias_frame ) ) {
				ad->all_bias_pixels_are_equal = TRUE;

				ad->constant_bias_pixel_offset =
				  mxp_area_detector_constant_bias_pixel_offset(
					ad->bias_frame );
			} else {
				ad->all_bias_pixels_are_equal = FALSE;

				mx_status = mx_image_get_average_intensity(
						ad->bias_frame, ad->mask_frame,
						&(ad->bias_average_intensity) );
			}
		}

		if ( ad->rebinned_bias_frame != NULL ) {
			mx_image_free( ad->rebinned_bias_frame );

			ad->rebinned_bias_frame = NULL;
		}
		strlcpy( ad->bias_filename, frame_filename,
					sizeof(ad->bias_filename) );

		mx_free( ad->dark_current_offset_array );
		mx_free( ad->flat_field_scale_array );
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

	case MXFT_AD_FLAT_FIELD_FRAME:
		if ( ad->flat_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flat_field_frame, ad->mask_frame,
					&(ad->flat_field_average_intensity) );
		}
		if ( ad->rebinned_flat_field_frame != NULL ) {
			mx_image_free( ad->rebinned_flat_field_frame );

			ad->rebinned_flat_field_frame = NULL;
		}
		strlcpy( ad->flat_field_filename, frame_filename,
					sizeof(ad->flat_field_filename) );

		mx_free( ad->flat_field_scale_array );
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

	case MXFT_AD_FLAT_FIELD_FRAME:
		strlcpy( ad->flat_field_filename, frame_filename,
					sizeof(ad->flat_field_filename) );
		break;

	case MXFT_AD_REBINNED_MASK_FRAME:
	case MXFT_AD_REBINNED_BIAS_FRAME:
	case MXFT_AD_REBINNED_DARK_CURRENT_FRAME:
	case MXFT_AD_REBINNED_FLAT_FIELD_FRAME:
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
	case MXFT_AD_FLAT_FIELD_FRAME:
		source_filename = ad->flat_field_filename;
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
		if ( ad->mask_frame == NULL ) {
			ad->all_mask_pixels_are_set = TRUE;
		} else {
			ad->all_mask_pixels_are_set =
				mxp_area_detector_all_mask_pixels_are_set(
							ad->mask_frame );
		}

		if ( ad->rebinned_mask_frame != NULL ) {
			mx_image_free( ad->rebinned_mask_frame );

			ad->rebinned_mask_frame = NULL;
		}
		destination_filename = ad->mask_filename;
		break;

	case MXFT_AD_BIAS_FRAME:
		if ( ad->bias_frame == NULL ) {
			ad->all_bias_pixels_are_equal = TRUE;
			ad->constant_bias_pixel_offset = 0.0;
		} else {
			if ( mx_image_all_pixels_are_equal( ad->bias_frame ) ) {
				ad->all_bias_pixels_are_equal = TRUE;

				ad->constant_bias_pixel_offset =
				  mxp_area_detector_constant_bias_pixel_offset(
					ad->bias_frame );
			} else {
				ad->all_bias_pixels_are_equal = FALSE;

				mx_status = mx_image_get_average_intensity(
						ad->bias_frame, ad->mask_frame,
						&(ad->bias_average_intensity) );
			}
		}

		if ( ad->rebinned_bias_frame != NULL ) {
			mx_image_free( ad->rebinned_bias_frame );

			ad->rebinned_bias_frame = NULL;
		}
		destination_filename = ad->bias_filename;

		mx_free( ad->dark_current_offset_array );
		mx_free( ad->flat_field_scale_array );
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

	case MXFT_AD_FLAT_FIELD_FRAME:
		if ( ad->flat_field_frame != NULL ) {
			mx_status = mx_image_get_average_intensity(
					ad->flat_field_frame, ad->mask_frame,
					&(ad->flat_field_average_intensity) );
		}
		if ( ad->rebinned_flat_field_frame != NULL ) {
			mx_image_free( ad->rebinned_flat_field_frame );

			ad->rebinned_flat_field_frame = NULL;
		}
		destination_filename = ad->flat_field_filename;

		mx_free( ad->flat_field_scale_array );
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
mx_area_detector_update_frame_pointers( MX_AREA_DETECTOR *ad )
{
#if MX_AREA_DETECTOR_DEBUG_IMAGE_FRAME_DATA
	static const char fname[] = "mx_area_detector_update_frame_pointers()";
#endif

	MX_IMAGE_FRAME *image_frame;
	mx_status_type mx_status;

	image_frame = ad->image_frame;

	if ( image_frame == NULL ) {
		ad->image_frame_header_length = 0;
		ad->image_frame_header        = NULL;
		ad->image_frame_data          = NULL;
	} else {
		ad->image_frame_header_length = image_frame->header_length;
		ad->image_frame_header = (char *) image_frame->header_data;
		ad->image_frame_data          = image_frame->image_data;
	}

#if MX_AREA_DETECTOR_DEBUG_IMAGE_FRAME_DATA
	MX_DEBUG(-2,("%s: mx_pointer_is_valid(%p) = %d",
		fname, ad->image_frame_data,
		mx_pointer_is_valid( ad->image_frame_data,
					sizeof(uint16_t), R_OK | W_OK ) ));
	mx_vm_show_os_info( stderr, ad->image_frame_data, sizeof(uint16_t) );
#endif

	/* Modify the 'image_frame_header' record field to have
	 * the correct length in bytes.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( ad->record,
				"image_frame_header",
				ad->image_frame_header_length );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Modify the 'image_frame_data' record field to have
	 * the correct length in bytes.
	 */

	mx_status = mx_set_1d_field_array_length_by_name( ad->record,
				"image_frame_data",
				ad->bytes_per_frame );

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
				MXFT_AD_IMAGE_FRAME, &(ad->image_frame) );

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
	long roi_row, roi_column;
	unsigned long row_offset, column_offset;
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

	roi_row_width     = (long) ( ad->roi[1] - ad->roi[0] + 1 );
	roi_column_height = (long) ( ad->roi[3] - ad->roi[2] + 1 );

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		image_format    = ad->image_format;
		byte_order      = ad->byte_order;
		bytes_per_pixel = ad->bytes_per_pixel;
	} else {
		image_format    = (long) MXIF_IMAGE_FORMAT(image_frame);
		byte_order      = (long) MXIF_BYTE_ORDER(image_frame);
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
				0, 0, NULL, NULL );

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

	ad->roi_bytes_per_frame = (long) (*roi_frame)->image_length;

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

		row_width     = (long) MXIF_ROW_FRAMESIZE(image_frame);
		column_height = (long) MXIF_COLUMN_FRAMESIZE(image_frame);

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
						MXFT_USHORT,
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
						MXFT_USHORT,
						2, dimension, element_size,
						&array_ptr );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_array_free_overlay( image_array_u16 );
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

			mx_array_free_overlay( image_array_u16 );
			mx_array_free_overlay( roi_array_u16 );
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
mx_area_detector_mark_frame_as_saved( MX_RECORD *ad_record,
					long frame_number )
{
	static const char fname[] = "mx_area_detector_mark_frame_as_saved()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( ad_record,
						&ad, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = fl_ptr->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	ad->mark_frame_as_saved = frame_number;

	ad->parameter_type = MXLV_AD_MARK_FRAME_AS_SAVED;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_get_parameter( MX_RECORD *ad_record, long parameter_type )
{
	static const char fname[] = "mx_area_detector_get_parameter()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_AREA_DETECTOR * );
	mx_status_type status;

	status = mx_area_detector_get_pointers( ad_record,
					&ad, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The get_parameter function is not supported by the driver for record '%s'.",
			ad_record->name );
	}

	ad->parameter_type = parameter_type;

	status = ( *fptr ) ( ad );

	return status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_parameter( MX_RECORD *ad_record, long parameter_type )
{
	static const char fname[] = "mx_area_detector_set_parameter()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_AREA_DETECTOR * );
	mx_status_type status;

	status = mx_area_detector_get_pointers( ad_record,
					&ad, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The set_parameter function is not supported by the driver for record '%s'.",
			ad_record->name );
	}

	ad->parameter_type = parameter_type;

	status = ( *fptr ) ( ad );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_default_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_transfer_frame()";

	MX_IMAGE_FRAME *source_frame;
	mx_bool_type destination_is_primary_image_frame;
	mx_status_type mx_status;

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
	case MXFT_AD_FLAT_FIELD_FRAME:
		source_frame = ad->flat_field_frame;
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

	if ( *(ad->transfer_destination_frame_ptr) == source_frame ) {
		/* If the destination and the source are the same,
		 * then we do not need to do any copying.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( *(ad->transfer_destination_frame_ptr) == (ad->image_frame) ) {
		destination_is_primary_image_frame = TRUE;
	} else {
		destination_is_primary_image_frame = FALSE;
	}

	mx_status = mx_area_detector_copy_and_convert_frame(
					ad->transfer_destination_frame_ptr,
					source_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( destination_is_primary_image_frame ) {
		mx_status = mx_area_detector_update_frame_pointers( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_load_frame()";

	MX_IMAGE_FRAME **frame_ptr;
	unsigned long file_format;
	long image_format, expected_image_format;
#if 0
	double bytes_per_pixel;
	unsigned long bytes_per_frame;
#endif
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
		expected_image_format = ad->image_format;
		break;
	case MXFT_AD_MASK_FRAME:
		frame_ptr = &(ad->mask_frame);
		expected_image_format = ad->mask_image_format;
		break;
	case MXFT_AD_BIAS_FRAME:
		frame_ptr = &(ad->bias_frame);
		expected_image_format = ad->bias_image_format;
		break;
	case MXFT_AD_DARK_CURRENT_FRAME:
		frame_ptr = &(ad->dark_current_frame);
		expected_image_format = ad->dark_current_image_format;
		break;
	case MXFT_AD_FLAT_FIELD_FRAME:
		frame_ptr = &(ad->flat_field_frame);
		expected_image_format = ad->flat_field_image_format;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported frame type %ld requested for area detector '%s'.",
			ad->load_frame, ad->record->name );
		break;
	}

	if ( ad->load_frame == MXFT_AD_IMAGE_FRAME ) {
		file_format = ad->datafile_load_format;
	} else {
		file_format = ad->correction_load_format;
	}

	mx_status = mx_image_read_file( frame_ptr, ad->dictionary,
					file_format, ad->frame_filename );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does the frame we just loaded have the expected image format? */

	image_format = MXIF_IMAGE_FORMAT(*frame_ptr);

	if ( expected_image_format <= 0 ) {
		switch( ad->load_frame ) {
		case MXFT_AD_IMAGE_FRAME:
			ad->image_format = image_format;
			break;
		case MXFT_AD_MASK_FRAME:
			ad->mask_image_format = image_format;
			break;
		case MXFT_AD_BIAS_FRAME:
			ad->bias_image_format = image_format;
			break;
		case MXFT_AD_DARK_CURRENT_FRAME:
			ad->dark_current_image_format = image_format;
			break;
		case MXFT_AD_FLAT_FIELD_FRAME:
			ad->flat_field_image_format = image_format;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported frame type %ld requested for "
			"area detector '%s'.",
				ad->load_frame, ad->record->name );
			break;
		}
	} else
	if ( expected_image_format != image_format ) {
		char image_format_name[20];
		char expected_image_format_name[20];

		(void) mx_image_get_image_format_name_from_type(
						image_format,
						image_format_name,
						sizeof(image_format_name) );


		(void) mx_image_get_image_format_name_from_type(
						expected_image_format,
						expected_image_format_name,
					sizeof(expected_image_format_name) );

		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The image format '%s' for file '%s' does not match "
		"the expected image format '%s' for area detector '%s'.",
			image_format_name,
			ad->frame_filename,
			expected_image_format_name,
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
	unsigned long file_format;
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
	case MXFT_AD_FLAT_FIELD_FRAME:
		frame = ad->flat_field_frame;
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
	case MXFT_AD_REBINNED_FLAT_FIELD_FRAME:
		frame = ad->rebinned_flat_field_frame;
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

	if ( ad->save_frame == MXFT_AD_IMAGE_FRAME ) {
		file_format = ad->datafile_save_format;
	} else {
		file_format = ad->correction_save_format;
	}

	mx_status = mx_image_write_file( frame, ad->dictionary,
					file_format, ad->frame_filename );
	
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
	MX_IMAGE_FRAME **dest_frame_ptr;
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
	case MXFT_AD_FLAT_FIELD_FRAME:
		src_frame = ad->flat_field_frame;
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
	case MXFT_AD_FLAT_FIELD_FRAME:
		dest_frame_ptr = &(ad->flat_field_frame);
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Unsupported destination frame type %ld requested for area detector '%s'.",
			ad->copy_frame[1], ad->record->name );
	}

	mx_status = mx_area_detector_copy_and_convert_frame( dest_frame_ptr,
								src_frame );
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
	case MXFT_RECORD_FIELD:
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
		ad->register_value = (long) *((unsigned long *) value_ptr);
		break;
	case MXFT_INT64:
		ad->register_value = (long) *((int64_t *) value_ptr);
		break;
	case MXFT_UINT64:
		ad->register_value = (long) *((uint64_t *) value_ptr);
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
			fname, ad->register_name ));
#endif
	/* We must copy the register value to the field. */

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: register_name = '%s', register_value = %ld",
		fname, ad->register_name ));
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
	case MXLV_AD_BINSIZE:
	case MXLV_AD_BITS_PER_PIXEL:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_CORRECTION_FLAGS:
	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
	case MXLV_AD_DATAFILE_DIRECTORY:
	case MXLV_AD_DATAFILE_NAME:
	case MXLV_AD_DATAFILE_NUMBER:
	case MXLV_AD_DATAFILE_PATTERN:
	case MXLV_AD_EXPOSURE_MODE:
	case MXLV_AD_FILENAME_LOG:
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_FRAME_FILENAME:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_MAXIMUM_FRAMESIZE:
	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_ROI_NUMBER:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_SHUTTER_ENABLE:
	case MXLV_AD_TRIGGER_MODE:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT:
	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_load_format_name,
					(long *) &(ad->correction_load_format));
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT:
	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_save_format_name,
					(long *) &(ad->correction_save_format));
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT:
	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_load_format_name,
					(long *) &(ad->datafile_load_format));
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT:
	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_save_format_name,
					(long *) &(ad->datafile_save_format));
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

	case MXLV_AD_DISK_SPACE:
		mx_status = mx_area_detector_get_local_disk_space( ad->record,
							&(ad->disk_space[0]),
							&(ad->disk_space[1]));
		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
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
		case MXT_SQ_STREAM:
			exposure_time = seq.parameter_array[0];

			ad->total_acquisition_time = exposure_time;

			ad->total_sequence_time = ad->sequence_start_delay
						+ ad->total_acquisition_time
						+ ad->detector_readout_time;
			break;
		case MXT_SQ_MULTIFRAME:
			num_frames = mx_round( seq.parameter_array[0] );
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
		case MXT_SQ_GATED:
			num_frames = mx_round( seq.parameter_array[0] );
			exposure_time = seq.parameter_array[1];

			ad->total_acquisition_time =
				( exposure_time + ad->detector_readout_time )
					* (double) num_frames;

			ad->total_sequence_time = ad->sequence_start_delay
						+ ad->total_acquisition_time;
			break;
		case MXT_SQ_DURATION:
			num_frames = mx_round( seq.parameter_array[0] );

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

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( ad->parameter_type ) {
	case MXLV_AD_CORRECTION_FLAGS:
	case MXLV_AD_CORRECTION_MEASUREMENT_TIME:
	case MXLV_AD_DATAFILE_ALLOW_OVERWRITE:
	case MXLV_AD_DATAFILE_AUTOSELECT_NUMBER:
	case MXLV_AD_DATAFILE_DIRECTORY:
	case MXLV_AD_DATAFILE_NAME:
	case MXLV_AD_DATAFILE_NUMBER:
	case MXLV_AD_DATAFILE_PATTERN:
	case MXLV_AD_EXPOSURE_MODE:
	case MXLV_AD_FRAME_FILENAME:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_NUM_CORRECTION_MEASUREMENTS:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_ROI_NUMBER:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_SHUTTER_ENABLE:
	case MXLV_AD_TRIGGER_MODE:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;

		/*---*/

	case MXLV_AD_CORRECTION_LOAD_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->correction_load_format,
					ad->correction_load_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_CORRECTION_LOAD_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_load_format_name,
					(long *) &(ad->correction_load_format));

		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->correction_save_format,
					ad->correction_save_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_CORRECTION_SAVE_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->correction_save_format_name,
					(long *) &(ad->correction_save_format));

		break;

		/*---*/

	case MXLV_AD_DATAFILE_LOAD_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->datafile_load_format,
					ad->datafile_load_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_DATAFILE_LOAD_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_load_format_name,
					(long *) &(ad->datafile_load_format));

		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT:
		mx_status = mx_image_get_file_format_name_from_type(
					ad->datafile_save_format,
					ad->datafile_save_format_name,
					MXU_AD_DATAFILE_FORMAT_NAME_LENGTH );
		break;

	case MXLV_AD_DATAFILE_SAVE_FORMAT_NAME:
		mx_status = mx_image_get_file_format_type_from_name(
					ad->datafile_save_format_name,
					(long *) &(ad->datafile_save_format));

		break;

		/*---*/

	case MXLV_AD_CORRECTION_MEASUREMENT_TYPE:
		mx_status = mx_area_detector_measure_correction_frame(
					ad->record,
					ad->correction_measurement_type,
					ad->correction_measurement_time,
					ad->num_correction_measurements );
		break;

	case MXLV_AD_MARK_FRAME_AS_SAVED:

		/* By default, 'mark_frame_as_saved' does nothing.  Only
		 * drivers for detectors with actual circular buffers should
		 * do anything with this field.
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

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_copy_and_convert_frame( MX_IMAGE_FRAME **dest_frame_ptr,
					MX_IMAGE_FRAME *src_frame )
{
	static const char fname[] = "mx_area_detector_copy_and_convert_frame()";

	long dest_image_format, row_framesize, column_framesize;
	double bytes_per_pixel;
	size_t dest_image_size_in_bytes;
	mx_status_type mx_status;

	if ( src_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The source frame pointer passed was NULL." );
	}
	if ( dest_frame_ptr == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The destination frame pointer passed was NULL." );
	}

	/* If present, we get the image format for the new destination frame
	 * from the old destination frame.  If there is not old destination
	 * frame, then we get the image format from the source frame.
	 */

	if ( (*dest_frame_ptr) == (MX_IMAGE_FRAME *) NULL ) {
		dest_image_format = MXIF_IMAGE_FORMAT( src_frame );
	} else {
		dest_image_format = MXIF_IMAGE_FORMAT( (*dest_frame_ptr) );
	}

	mx_status = mx_image_format_get_bytes_per_pixel( dest_image_format,
							&bytes_per_pixel );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The other parameters we get from the source frame. */

	row_framesize    = MXIF_ROW_FRAMESIZE( src_frame );
	column_framesize = MXIF_COLUMN_FRAMESIZE( src_frame );

	dest_image_size_in_bytes = mx_round( bytes_per_pixel
			* (double)( row_framesize * column_framesize ) );

	/* Update the destination frame pointer. */

	mx_status = mx_image_alloc( dest_frame_ptr,
					row_framesize,
					column_framesize,
					dest_image_format,
					MXIF_BYTE_ORDER( src_frame ),
					bytes_per_pixel,
					MXIF_HEADER_BYTES( src_frame ),
					dest_image_size_in_bytes,
					src_frame->dictionary,
					src_frame->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Copy the exposure time and timestamp to the destination frame. */

	MXIF_EXPOSURE_TIME_SEC(*dest_frame_ptr)
				= MXIF_EXPOSURE_TIME_SEC(src_frame);
	MXIF_EXPOSURE_TIME_NSEC(*dest_frame_ptr)
				= MXIF_EXPOSURE_TIME_NSEC(src_frame);

	MXIF_TIMESTAMP_SEC(*dest_frame_ptr) = MXIF_TIMESTAMP_SEC(src_frame);
	MXIF_TIMESTAMP_NSEC(*dest_frame_ptr) = MXIF_TIMESTAMP_NSEC(src_frame);

	/* Now copy the source frame data and convert it to an image format 
	 * that matches the format of the destination frame.
	 */
				
	mx_status = mx_area_detector_copy_and_convert_image_data(
					*dest_frame_ptr, src_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_copy_and_convert_image_data( MX_IMAGE_FRAME *dest_frame,
						MX_IMAGE_FRAME *src_frame )
{
	static const char fname[] =
		"mx_area_detector_copy_and_convert_image_data()";

	uint16_t *uint16_src, *uint16_dest;
	int32_t *int32_src, *int32_dest;
	float *float_src, *float_dest;
	double *double_src, *double_dest;
	int32_t s32_pixel;
	float flt_pixel;
	double dbl_pixel;
	long i;
	long src_format, dest_format;
	long src_pixels, dest_pixels;
	double src_bytes_per_pixel, dest_bytes_per_pixel;
	size_t src_size, dest_size;
	mx_status_type mx_status;
	
	src_pixels = MXIF_ROW_FRAMESIZE(src_frame)
			* MXIF_COLUMN_FRAMESIZE(src_frame );

	src_format = MXIF_IMAGE_FORMAT(src_frame);

	mx_status = mx_image_format_get_bytes_per_pixel( src_format,
							&src_bytes_per_pixel );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	src_size = mx_round( src_pixels * src_bytes_per_pixel );

	dest_pixels = MXIF_ROW_FRAMESIZE(dest_frame)
			* MXIF_COLUMN_FRAMESIZE(dest_frame );

	dest_format = MXIF_IMAGE_FORMAT(dest_frame);

	mx_status = mx_image_format_get_bytes_per_pixel( dest_format,
							&dest_bytes_per_pixel );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dest_size = mx_round( dest_pixels * dest_bytes_per_pixel );

	if ( dest_pixels != src_pixels ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The source frame has a different number of pixels (%lu) "
		"than the destination frame (%lu).",
			(unsigned long) src_size, (unsigned long) dest_size );
	}

	switch( src_format ) {
	case MXT_IMAGE_FORMAT_GREY16:
		uint16_src = src_frame->image_data;

		switch( dest_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			uint16_dest = dest_frame->image_data;

			memcpy( uint16_dest, uint16_src, dest_size );
			break;
		case MXT_IMAGE_FORMAT_INT32:
			int32_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				int32_dest[i] = uint16_src[i];
			}
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			float_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				float_dest[i] = uint16_src[i];
			}
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			double_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				double_dest[i] = uint16_src[i];
			}
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Destination image frame format %ld is not "
				"supported by this function.", dest_format );
			break;
		}
		break;

	case MXT_IMAGE_FORMAT_INT32:
		int32_src = src_frame->image_data;

		switch( dest_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			uint16_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				s32_pixel = int32_src[i];

				if ( s32_pixel < 0 ) {
					uint16_dest[i] = 0;
				} else
				if ( s32_pixel > 65535 ) {
					uint16_dest[i] = 65535;
				} else {
					uint16_dest[i] = int32_src[i];
				}
			}
			break;
		case MXT_IMAGE_FORMAT_INT32:
			int32_dest = dest_frame->image_data;

			memcpy( int32_dest, int32_src, dest_size );
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			float_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				float_dest[i] = int32_src[i];
			}
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			double_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				double_dest[i] = int32_src[i];
			}
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Destination image frame format %ld is not "
				"supported by this function.", dest_format );
			break;
		}
		break;

	case MXT_IMAGE_FORMAT_FLOAT:
		float_src = src_frame->image_data;

		switch( dest_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			uint16_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				flt_pixel = float_src[i];

				if ( flt_pixel < 0.0 ) {
					uint16_dest[i] = 0;
				} else
				if ( flt_pixel > 65535.0 ) {
					uint16_dest[i] = 65535;
				} else {
					uint16_dest[i] =
						(uint16_t)(flt_pixel + 0.5);
				}
			}
			break;
		case MXT_IMAGE_FORMAT_INT32:
			int32_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				int32_dest[i] = mx_round( float_src[i] );
			}
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			float_dest = dest_frame->image_data;

			memcpy( float_dest, float_src, dest_size );
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			double_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				double_dest[i] = float_src[i];
			}
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Destination image frame format %ld is not "
				"supported by this function.", dest_format );
			break;
		}
		break;

	case MXT_IMAGE_FORMAT_DOUBLE:
		double_src = src_frame->image_data;

		switch( dest_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
			uint16_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				dbl_pixel = double_src[i];

				if ( dbl_pixel < 0.0 ) {
					uint16_dest[i] = 0;
				} else
				if ( dbl_pixel > 65535.0 ) {
					uint16_dest[i] = 65535;
				} else {
					uint16_dest[i] = mx_round( dbl_pixel );
				}
			}
			break;
		case MXT_IMAGE_FORMAT_INT32:
			int32_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				int32_dest[i] = mx_round( double_src[i] );
			}
			break;
		case MXT_IMAGE_FORMAT_FLOAT:
			float_dest = dest_frame->image_data;

			for ( i = 0; i < dest_pixels; i++ ) {
				float_dest[i] = double_src[i];
			}
			break;
		case MXT_IMAGE_FORMAT_DOUBLE:
			double_dest = dest_frame->image_data;

			memcpy( double_dest, double_src, dest_size );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Destination image frame format %ld is not "
				"supported by this function.", dest_format );
			break;
		}
		break;

	default:
		switch( dest_format ) {
		case MXT_IMAGE_FORMAT_GREY16:
		case MXT_IMAGE_FORMAT_INT32:
		case MXT_IMAGE_FORMAT_FLOAT:
		case MXT_IMAGE_FORMAT_DOUBLE:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Source image frame format %ld is not "
				"supported by this function.", src_format );
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Neither source image frame format %ldu, nor "
				"destination image frame format %ld is "
				"supported by this function.",
					src_format, dest_format );
			break;
		}
		break;
	}

	/* Copy the exposure time. */

	MXIF_EXPOSURE_TIME_SEC(dest_frame) = MXIF_EXPOSURE_TIME_SEC(src_frame);
	MXIF_EXPOSURE_TIME_NSEC(dest_frame) =
					MXIF_EXPOSURE_TIME_NSEC(src_frame);

	/* Copy the timestamp. */

	MXIF_TIMESTAMP_SEC(dest_frame)  = MXIF_TIMESTAMP_SEC(src_frame);
	MXIF_TIMESTAMP_NSEC(dest_frame) = MXIF_TIMESTAMP_NSEC(src_frame);

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_get_disk_space( MX_RECORD *record,
				uint64_t *total_disk_space,
				uint64_t *free_disk_space )
{
	static const char fname[] = "mx_area_detector_get_disk_space()";

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

	ad->parameter_type = MXLV_AD_DISK_SPACE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( total_disk_space != NULL ) {
		*total_disk_space = ad->disk_space[0];
	}
	if ( free_disk_space != NULL ) {
		*free_disk_space = ad->disk_space[1];
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_local_disk_space( MX_RECORD *record,
				uint64_t *local_total_disk_space,
				uint64_t *local_free_disk_space )
{
	static const char fname[] = "mx_area_detector_get_local_disk_space()";

	MX_AREA_DETECTOR *ad;
	char directory_name[MXU_FILENAME_LENGTH+1];
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strlen( ad->datafile_directory ) == 0 ) {
#if defined(OS_WIN32)
		strlcpy( directory_name, "c:/", sizeof(directory_name) );
#else
		strlcpy( directory_name, "/", sizeof(directory_name) );
#endif
		mx_status = mx_get_disk_space( directory_name,
						local_total_disk_space,
						local_free_disk_space );
	} else {
		mx_status = mx_get_disk_space( ad->datafile_directory,
						local_total_disk_space,
						local_free_disk_space );
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

/* In mx_area_detector_initialize_datafile_number(), we look at all of
 * the files that match the pattern, see which one has the highest datafile
 * number, and then set the current datafile number to one after the
 * existing highest number.
 */

MX_EXPORT mx_status_type
mx_area_detector_initialize_datafile_number( MX_RECORD *record )
{
	static const char fname[] =
		"mx_area_detector_initialize_datafile_number()";

	MX_AREA_DETECTOR *ad;
	char *start_of_varying_number, *trailing_segment;
	int length_of_varying_number, length_of_leading_segment;
	int saved_errno, num_items;
	char format[80];
	char *ptr;
	DIR *dir;
	struct dirent *dirent_ptr;
	char *name_ptr;
	unsigned long datafile_number;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif
	/* If autoselecting datafile numbers is turned off, then we just
	 * return without changing ad->datafile_number.
	 */

	if ( ad->datafile_autoselect_number == FALSE ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If the datafile pattern is empty, then set the datafile number
	 * to zero, since it will not be used.
	 */

	if ( ad->datafile_pattern[0] == '\0' ) {
		ad->datafile_number = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Look for the start of the varying number in the datafile pattern. */

	start_of_varying_number = strchr( ad->datafile_pattern,
				MX_AREA_DETECTOR_DATAFILE_PATTERN_CHAR );

	/* If there is no varying part, then set the datafile number to zero,
	 * since it will not be used.
	 */

	if ( start_of_varying_number == NULL ) {
		ad->datafile_number = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	length_of_leading_segment =
		start_of_varying_number - ad->datafile_pattern + 1;

	/* How long is the varying number? */

	length_of_varying_number = strspn( start_of_varying_number,
				MX_AREA_DETECTOR_DATAFILE_PATTERN_STRING );

	if ( length_of_varying_number <= 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The length of the varying number is %d in the datafile "
		"pattern '%s' for area detector '%s', even though the "
		"datafile pattern char '%c' was found.  "
		"It should be impossible for this to happen.",
			length_of_varying_number,
			ad->datafile_pattern, record->name,
			MX_AREA_DETECTOR_DATAFILE_PATTERN_CHAR );
	}

	/* Find the trailing part of the pattern after
	 * the datafile number section.
	 */

	trailing_segment = start_of_varying_number + length_of_varying_number;

	/* Construct a format string to be used to compare to all of the
	 * existing files in the datafile directory.
	 */

	if ( length_of_leading_segment > sizeof(format) ) {
		length_of_leading_segment = sizeof(format);
	}

	strlcpy( format, ad->datafile_pattern, length_of_leading_segment );

	ptr = format + length_of_leading_segment - 1;

	snprintf( ptr, sizeof(format) - length_of_leading_segment,
		"%%lu%s", trailing_segment );

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
	MX_DEBUG(-2,("%s: format = '%s'", fname, format));
#endif

	ad->datafile_number = 0;

	/* Loop through all of the files in the destination directory. */

	dir = opendir( ad->datafile_directory );

	if ( dir == (DIR *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot access the datafile directory '%s' for "
		"area detector '%s'.  Errno = %d, error message = '%s'",
			ad->datafile_directory, record->name,
			saved_errno, strerror(saved_errno) );
	}

	while (1) {
		errno = 0;

		dirent_ptr = readdir(dir);

		if ( dirent_ptr != NULL ) {
			name_ptr = dirent_ptr->d_name;

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
			MX_DEBUG(-2,("%s: name_ptr = '%s'", fname, name_ptr));
#endif

			if ( strcmp( name_ptr, "." ) == 0 ) {
				continue;
			} else
			if ( strcmp( name_ptr, ".." ) == 0 ) {
				continue;
			}

			num_items = sscanf(name_ptr, format, &datafile_number);

			if ( num_items != 1 ) {
				continue;
			}

			if ( datafile_number > ad->datafile_number ) {
				ad->datafile_number = datafile_number;
			}
		} else {
			if ( errno == 0 ) {
				break;		/* Exit the while() loop. */
			} else {
				saved_errno = errno;

				closedir( dir );

				return mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while examining the "
				"files in the datafile directory '%s' for "
				"area detector '%s'.  "
				"Errno = %d, error message = '%s'",
					ad->datafile_directory, record->name,
					saved_errno, strerror(saved_errno) );
			}
		}
	}

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
	MX_DEBUG(-2,("%s: ad->datafile_number = %lu",
		fname, ad->datafile_number));
#endif

	closedir( dir );

	return MX_SUCCESSFUL_RESULT;
}

/* FIXME: The following should be rewritten to use the generic function
 *        mx_construct_file_name_from_file_pattern().
 */

MX_EXPORT mx_status_type
mx_area_detector_construct_next_datafile_name(
				MX_RECORD *record,
				mx_bool_type autoincrement_datafile_number,
				char *filename_buffer,
				size_t filename_buffer_size )
{
	static const char fname[] =
		"mx_area_detector_construct_next_datafile_name()";

	MX_AREA_DETECTOR *ad;
	MX_RECORD_FIELD *datafile_name_field;
	char *start_of_varying_number, *trailing_segment;
	int length_of_varying_number, length_of_leading_segment;
	char datafile_number_string[40];
	char format[10];
	unsigned long next_datafile_number;
	char *local_filename_buffer_ptr;
	size_t local_filename_buffer_size;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	/* If this record has a datafile_name field, then we set the
	 * value_has_changed_manual_override flag to TRUE.
	 */

	datafile_name_field = mx_get_record_field( record, "datafile_name" );

	if ( datafile_name_field != (MX_RECORD_FIELD *) NULL ) {
		datafile_name_field->value_has_changed_manual_override = TRUE;
	}

	/* If the datafile pattern is empty, we use the existing contents
	 * of datafile_name as is.
	 */

	if ( ad->datafile_pattern[0] == '\0' ) {

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
		MX_DEBUG(-2,
		("%s: Using datafile name '%s' for area detector '%s' as is.",
			fname, ad->datafile_name, record->name ));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* Look for the start of the varying number in the datafile pattern. */

	start_of_varying_number = strchr( ad->datafile_pattern,
				MX_AREA_DETECTOR_DATAFILE_PATTERN_CHAR );

	/* If there is no varying part, then copy the pattern to the filename
	 * and then return.
	 */

	if ( start_of_varying_number == NULL ) {
		strlcpy( ad->datafile_name, ad->datafile_pattern,
			sizeof(ad->datafile_name) );

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
		MX_DEBUG(-2,
		("%s: Using datafile pattern as the datafile name '%s' "
		"for area detector '%s'.",
			fname, ad->datafile_name, record->name ));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	length_of_leading_segment =
		start_of_varying_number - ad->datafile_pattern + 1;

	/* How long is the varying number? */

	length_of_varying_number = strspn( start_of_varying_number,
				MX_AREA_DETECTOR_DATAFILE_PATTERN_STRING );

	if ( length_of_varying_number <= 0 ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The length of the varying number is %d in the datafile "
		"pattern '%s' for area detector '%s', even though the "
		"datafile pattern char '%c' was found.  "
		"It should be impossible for this to happen.",
			length_of_varying_number,
			ad->datafile_pattern, record->name,
			MX_AREA_DETECTOR_DATAFILE_PATTERN_CHAR );
	}

	/* Construct the new varying string. */

	next_datafile_number = ad->datafile_number + 1;

	if ( autoincrement_datafile_number ) {
		ad->datafile_number++;
	}

	snprintf( format, sizeof(format), "%%0%dlu", length_of_varying_number );

	snprintf( datafile_number_string, sizeof(datafile_number_string),
				format, next_datafile_number );

	/* If the caller has provided a buffer, then we write the new
	 * filename to that buffer.  Otherwise, we write it to the
	 * default location of ad->datafile_name.
	 */

	if ( (filename_buffer != NULL) && (filename_buffer_size > 0) ) {
		local_filename_buffer_ptr = filename_buffer;
		local_filename_buffer_size = filename_buffer_size;
	} else {
		local_filename_buffer_ptr = ad->datafile_name;
		local_filename_buffer_size = sizeof( ad->datafile_name );
	}

	/* Protect against buffer overflows. */

	if ( length_of_leading_segment > local_filename_buffer_size ) {
		mx_warning( "The name of the next datafile for "
			"area detector '%s' will be truncated, since it does "
			"fit into the provided filename buffer.",
				record->name );

		length_of_leading_segment = local_filename_buffer_size;
	}

	/* Construct the new datafile name. */

	strlcpy( local_filename_buffer_ptr, ad->datafile_pattern,
					length_of_leading_segment );

	strlcat( local_filename_buffer_ptr, datafile_number_string,
				local_filename_buffer_size );

	trailing_segment = start_of_varying_number + length_of_varying_number;

	strlcat( local_filename_buffer_ptr, trailing_segment,
				local_filename_buffer_size );

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
	MX_DEBUG(-2,("%s: datafile name = '%s'",
		fname, local_filename_buffer_ptr));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_initialize_remote_datafile_number( MX_RECORD *record,
						char *remote_prefix,
						char *local_prefix )
{
	static const char fname[] =
		"mx_area_detector_initialize_remote_datafile_number()";

	MX_AREA_DETECTOR *ad;
	char saved_datafile_directory[2*MXU_FILENAME_LENGTH+20];
	char remote_datafile_directory[2*MXU_FILENAME_LENGTH+20];
	char local_datafile_directory[2*MXU_FILENAME_LENGTH+20];
	char *slash_ptr;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_FILENAME_CONSTRUCTION
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	/* If no datafile directory has been specified, then we assume that
	 * datafiles are to be written to the current directory.
	 */

	if ( strlen( ad->datafile_directory ) == 0 ) {
		mx_status =
		    mx_area_detector_initialize_datafile_number( ad->record );

		return mx_status;
	}

	/* If the datafile pattern is empty, then set the datafile number
	 * to zero, since it will not be used.
	 */

	if ( strlen( ad->datafile_pattern ) == 0 ) {
		ad->datafile_number = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* The user may have specified the complete path of the datafile
	 * in ad->datafile_pattern, so we must construct the complete
	 * datafile directory and pattern in order to successfully 
	 * strip.
	 */

	snprintf( remote_datafile_directory, sizeof(remote_datafile_directory),
		"%s/%s", ad->datafile_directory, ad->datafile_pattern );

	/* Strip off any trailing filename. */

	slash_ptr = strrchr( remote_datafile_directory, '/' );

	if ( slash_ptr != NULL ) {
		*slash_ptr = '\0';
	}

	/* Construct a local datafile directory name from the remote
	 * datafile directory name using the remote and local prefixes.
	 */

	mx_status = mx_change_filename_prefix( remote_datafile_directory,
						remote_prefix,
						local_prefix,
						local_datafile_directory,
					    sizeof(local_datafile_directory) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Save the existing contents of ad->datafile_directory so that
	 * it can be restored after initializing the datafile number.
	 */

	strlcpy( saved_datafile_directory, ad->datafile_directory,
				sizeof(saved_datafile_directory) );

	/* Set the directory name to the local directory name. */

	strlcpy( ad->datafile_directory, local_datafile_directory,
				MXU_FILENAME_LENGTH );

	/* Now we can finally initialize the datafile number. */

	mx_status = mx_area_detector_initialize_datafile_number( record );

	/* Unconditionally restore the value of ad->datafile_directory
	 * and then exit.
	 */

	strlcpy( ad->datafile_directory, saved_datafile_directory,
				MXU_FILENAME_LENGTH );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

static mx_status_type
mxp_area_detector_datafile_management_callback( MX_CALLBACK *callback,
						void *argument )
{
	static const char fname[] =
		"mxp_area_detector_datafile_management_callback()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD *record;
	MX_AREA_DETECTOR *ad;
	mx_status_type (*handler_fn)(MX_RECORD *);
	mx_status_type mx_status;

	record_field = callback->u.record_field;

	record = record_field->record;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	handler_fn = ad->datafile_management_handler;

	if ( handler_fn == NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"No datafile management handler function has been "
		"installed for area detector '%s'.", ad->record->name );
	}

	mx_status = (*handler_fn)( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_setup_datafile_management( MX_RECORD *record,
			mx_status_type (*handler_fn)(MX_RECORD *) )
{
	static const char fname[] =
		"mx_area_detector_setup_datafile_management()";

	MX_AREA_DETECTOR *ad;
	MX_LIST_HEAD *list_head;
	MX_CALLBACK *callback_object;
	MX_RECORD_FIELD *field;
	unsigned long flags, mask;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	flags = ad->area_detector_flags;

	mask = MXF_AD_SAVE_FRAME_AFTER_ACQUISITION
		| MXF_AD_READOUT_FRAME_AFTER_ACQUISITION;

	/* If neither reading out or saving frames has been configure,
	 * then do not install a handler.
	 */

	if ( ( flags & mask ) == 0 ) {
		mx_warning(
		"Skipping datafile management setup for area detector '%s', "
		"since neither reading out frames nor saving frames "
		"have been configured.",
			record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	/* If no handler was specified, use the default handler. */

	if ( handler_fn == NULL ) {
		handler_fn =
		    mx_area_detector_default_datafile_management_handler;
	}

	/* Save a pointer to the handler function. */

	ad->datafile_management_handler = handler_fn;

	/* If we are running in a server with an active callback pipe,
	 * then setup a timer callback with the handler function as the
	 * callback handler.
	 */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD struct for the database containing "
		"area detector '%s' is NULL.  This should never happen.",
			record->name );
	}

	if ( list_head->callback_pipe == NULL ) {
		/* No callback pipe has been created, so we do not need to
		 * set up a callback.  This means that we are done here.
		 */

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
		MX_DEBUG(-2,("%s: No callback pipe, so returning...", fname));
#endif

		return MX_SUCCESSFUL_RESULT;
	}

	/* We need to install a value-changed callback for the
	 * 'total_num_frames' field of the area detector with
	 * a NULL socket handler, since the value-changed
	 * callback is internal to the server.
	 */

	/* Find the MX_RECORD_FIELD structure for the field. */

	mx_status = mx_find_record_field( record,
					"total_num_frames",
					&field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
	MX_DEBUG(-2,("%s: Installing value changed callback for field '%s.%s'",
		fname, record->name, field->name));
#endif

	mx_status = mx_local_field_add_socket_handler_to_callback(
				field,
				MXCBT_VALUE_CHANGED,
				mxp_area_detector_datafile_management_callback,
				NULL,
				&callback_object );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_default_datafile_management_handler( MX_RECORD *record )
{
	static const char fname[] =
		"mx_area_detector_default_datafile_management_handler()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	char filename[MXU_FILENAME_LENGTH+1];
	unsigned long flags;
	int os_status, saved_errno;
	mx_bool_type save_frame_after_acquisition;
	mx_bool_type readout_frame_after_acquisition;
	mx_bool_type new_frames;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers( record, &ad, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = MX_SUCCESSFUL_RESULT;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP
	MX_DEBUG(-2,("**** %s invoked for area detector '%s' ****",
		fname, record->name ));
#endif

	/* Get the current value of the total number of frames by calling the
	 * driver function directly.  mx_area_detector_get_total_num_frames()
	 * is not used here since that would result in another recursive call
	 * to the datafile management handler and the program would crash when
	 * the resulting infinite series of recursive calls exceeded the
	 * maximum stack size.
	 */

	if ( flist->get_total_num_frames != NULL ) {
		mx_status = (flist->get_total_num_frames)( ad );
	} else
	if ( flist->get_extended_status != NULL ) {
		mx_status = (flist->get_extended_status)( ad );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Area detector '%s' does not provide a way to get the "
		"total number of frames since startup.  This means that "
		"the default datafile management handler cannot be used "
		"for this detector.", record->name );
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		(void) mx_area_detector_image_log_show_error( ad, mx_status );
	}

	if ( ad->total_num_frames <= ad->datafile_total_num_frames ) {
		new_frames = FALSE;
	} else {
		new_frames = TRUE;
	}

#if ( MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP \
	|| MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMESTAMP )

	MX_DEBUG(-2,("%s: total_num_frames = %lu, "
			"datafile_total_num_frames = %lu, "
			"new_frames = %d",
		fname, ad->total_num_frames,
		ad->datafile_total_num_frames,
		new_frames ));
#endif

	if ( new_frames == FALSE ) {
#if 0
		MX_DEBUG(-2,("%s: No new image frames are available to be "
		"saved or loaded.", fname));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* Figure out which operations need to be performed during this call. */

	flags = ad->area_detector_flags;

	if ( ad->inhibit_autosave ) {
		save_frame_after_acquisition = FALSE;
	} else
	if ( (flags & MXF_AD_SAVE_CORRECTION_FRAME_AFTER_ACQUISITION)
			  && ad->correction_measurement_in_progress )
	{
		save_frame_after_acquisition = TRUE;
	} else
	if ( flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ) {
		if ( ad->correction_measurement_in_progress ) {
			save_frame_after_acquisition = FALSE;
		} else {
			save_frame_after_acquisition = TRUE;
		}
	} else {
		save_frame_after_acquisition = FALSE;
	}

	if ( save_frame_after_acquisition ) {
		readout_frame_after_acquisition = TRUE;
	} else
	if ( flags & MXF_AD_READOUT_FRAME_AFTER_ACQUISITION ) {
		readout_frame_after_acquisition = TRUE;
	} else {
		readout_frame_after_acquisition = FALSE;
	}

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
	MX_DEBUG(-2,("%s: flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION = %lu",
		fname, flags & MXF_AD_SAVE_FRAME_AFTER_ACQUISITION ));
	MX_DEBUG(-2,("%s: ad->correction_measurement_in_progress = %d",
		fname, (int) ad->correction_measurement_in_progress));
	MX_DEBUG(-2,("%s: save_frame_after_acquisition = %d",
		fname, (int) save_frame_after_acquisition));
	MX_DEBUG(-2,("%s: readout_frame_after_acquisition = %d",
		fname, (int) readout_frame_after_acquisition));
#endif

	if ( readout_frame_after_acquisition ) {

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_TIMING total_measurement;
		MX_HRT_TIMING setup_measurement;
		MX_HRT_TIMING readout_measurement;
		MX_HRT_TIMING correct_measurement;

		MX_HRT_START( total_measurement );
		MX_HRT_START( setup_measurement );
#endif

		if ( ad->image_frame == NULL ) {

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
			MX_DEBUG(-2,("%s: Setting up initial image frame "
			"for area detector '%s'.", fname, record->name ));
#endif
			mx_status = mx_area_detector_setup_frame( record,
							&(ad->image_frame) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( setup_measurement );
		MX_HRT_START( readout_measurement );
#endif

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP
		MX_DEBUG(-2,("%s: Reading out image frame %lu",
			fname, ad->datafile_last_frame_number));
#endif
		mx_status = mx_area_detector_readout_frame( record,
						ad->datafile_last_frame_number);

		ad->datafile_last_frame_number++;

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( readout_measurement );
		MX_HRT_START( correct_measurement );
#endif

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP
		MX_DEBUG(-2,("%s: Correcting the image frame.", fname));
#endif
		mx_status = mx_area_detector_correct_frame( record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( correct_measurement );
		MX_HRT_END( total_measurement );

		MX_HRT_RESULTS( setup_measurement, fname, "for setup" );
		MX_HRT_RESULTS( readout_measurement, fname, "for readout" );
		MX_HRT_RESULTS( correct_measurement, fname, "for correction" );
		MX_HRT_RESULTS( total_measurement, fname, "for readout TOTAL" );
#endif
	}

	if ( save_frame_after_acquisition ) {

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_TIMING total_measurement;
		MX_HRT_TIMING filename_measurement;
		MX_HRT_TIMING overwrite_measurement;
		MX_HRT_TIMING write_file_measurement;
		MX_HRT_TIMING status_measurement;

		MX_HRT_START( total_measurement );
		MX_HRT_START( filename_measurement );
#endif

		/* If a datafile pattern has been specified, then construct
		 * the next filename that fits the pattern.
		 */

		if ( ad->datafile_pattern[0] != '\0' ) {
#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
			MX_DEBUG(-2,("%s: Constructing new filename for "
			"area detector '%s' using pattern '%s'.",
				fname, record->name, ad->datafile_pattern));
#endif
			mx_status =
	mx_area_detector_construct_next_datafile_name( record, TRUE, NULL, 0 );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_SETUP
			MX_DEBUG(-2,("%s: New filename = '%s'.",
				fname, ad->datafile_name));
#endif
		}

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE
		MX_DEBUG(-2,("%s: datafile_directory = '%s'",
			fname, ad->datafile_directory));
		MX_DEBUG(-2,("%s: datafile_pattern = '%s'",
			fname, ad->datafile_pattern));
		MX_DEBUG(-2,("%s: datafile_name = '%s'",
			fname, ad->datafile_name));
		MX_DEBUG(-2,("%s: datafile_number = %lu",
			fname, ad->datafile_number));
#endif

		/* Construct the full pathname of the file that we will save
		 * the image to.
		 */

		if ( strlen(ad->datafile_directory) == 0 ) {
	
			if ( strlen(ad->datafile_name) == 0 ) {
				ad->datafile_total_num_frames++;
	
				return mx_error(MXE_INITIALIZATION_ERROR, fname,
			    "The image frame directory and filename have not "
			    "yet been specified for area detector '%s'.",
					record->name );
			} else {
				strlcpy( filename, ad->datafile_name,
					sizeof(filename) );
			}
		} else
		if ( strlen(ad->datafile_name) == 0 ) {
			ad->datafile_total_num_frames++;
	
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"No image frame filename has been specified for image "
			"frame directory '%s' used by area detector '%s'.",
				ad->datafile_directory, record->name );
		} else {
			snprintf( filename, sizeof(filename),
			  "%s/%s", ad->datafile_directory, ad->datafile_name );
		}

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( filename_measurement );
		MX_HRT_START( overwrite_measurement );
#endif

		if ( ad->datafile_allow_overwrite == FALSE ) {
			/* If datafile overwriting is not allowed, then
			 * we must check first to see if a file with
			 * this filename already exists.
			 */

			os_status = access( filename, F_OK );

			saved_errno = errno;

			if ( os_status == 0 ) {
				mx_status = mx_error( MXE_ALREADY_EXISTS, fname,
				"Cannot write to image file '%s', since a file "
				"with that name already exists and the "
				"'%s.datafile_allow_overwrite' flag is set "
				"to FALSE.", filename, ad->record->name );

				(void) mx_area_detector_abort( record );

				return mx_status;
			} else
			if ( saved_errno != ENOENT ) {
				mx_status = mx_error( MXE_FILE_IO_ERROR, fname,
				"An error occurred while testing for the "
				"presence of the file '%s'.  "
				"Errno = %d, error message = '%s'",
					filename,
					saved_errno, strerror(saved_errno) );

				(void) mx_area_detector_abort( record );

				return mx_status;
			}
		}

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( overwrite_measurement );
		MX_HRT_START( write_file_measurement );
#endif

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_FILE
		MX_DEBUG(-2,("%s: Saving '%s' image frame to '%s'.",
			fname, record->name, filename));
#endif
		/* Write out the image file. */

		mx_status = mx_image_write_file( ad->image_frame, NULL,
						ad->datafile_save_format,
						filename );

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( write_file_measurement );
		MX_HRT_START( status_measurement );
#endif

#if 0
		MX_DEBUG(-2,("%s: mx_image_write_file() mx_status.code = %lu",
			fname, mx_status.code));
		MX_DEBUG(-2,("%s: ad->filename_log_record = %p",
			fname, ad->filename_log_record));
#endif

		if ( ( mx_status.code == MXE_SUCCESS )
		  && ( ad->filename_log_record != (MX_RECORD *) NULL ) )
		{
			/* If we are logging individual filenames,
			 * then do that now.
			 */

			(void) mx_area_detector_write_to_filename_log( ad,
								filename );
		} else
		if ( mx_status.code != MXE_SUCCESS ) {

			mx_area_detector_image_log_show_error( ad, mx_status );

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_FAILURE
			MX_DEBUG(-2,("%s: Autosave of '%s' by '%s' failed "
			"with MX error code %ld.", fname,
			filename, ad->record->name,
			mx_status.code ));
#endif
			switch ( mx_status.code ) {
			case MXE_FILE_IO_ERROR:
				ad->latched_status |= MXSF_AD_FILE_IO_ERROR;
				break;
			case MXE_PERMISSION_DENIED:
				ad->latched_status |= MXSF_AD_PERMISSION_DENIED;
				break;
			case MXE_DISK_FULL:
				ad->latched_status |= MXSF_AD_DISK_FULL;
				break;
			default:
				break;
			}

			ad->latched_status |= MXSF_AD_ERROR;

			/* Abort the running sequence. */

			(void) mx_area_detector_abort( record );
		}

		/* For area detectors that read their frames into a
		 * circular buffer, we must indicate that this frame
		 * has been saved.  Note that datafile_last_frame_number
		 * has already been incremented above, so we must 
		 * subtract 1 from it in our call to the function
		 * mx_area_detector_mark_frame_as_saved().
		 */

		mx_status = mx_area_detector_mark_frame_as_saved( record,
					ad->datafile_last_frame_number - 1 );

#if MX_AREA_DETECTOR_DEBUG_DATAFILE_AUTOSAVE_TIMING
		MX_HRT_END( status_measurement );
		MX_HRT_END( total_measurement );

		MX_HRT_RESULTS( filename_measurement, fname, "for filename" );
		MX_HRT_RESULTS( overwrite_measurement, fname, "for overwrite" );
		MX_HRT_RESULTS( write_file_measurement,
						fname, "for write file" );
		MX_HRT_RESULTS( status_measurement, fname, "for status" );
		MX_HRT_RESULTS( total_measurement, fname, "for save TOTAL" );
#endif
	}

	ad->datafile_total_num_frames++;

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_area_detector_vctest_extended_status( MX_RECORD_FIELD *record_field,
				int direction,
				mx_bool_type *value_changed_ptr )
{
	static const char fname[] = "mx_area_detector_vctest_extended_status()";

	MX_RECORD *record;
	MX_AREA_DETECTOR *ad;
	MX_RECORD_FIELD *extended_status_field;
	MX_RECORD_FIELD *last_frame_number_field;
	MX_RECORD_FIELD *total_num_frames_field;
	MX_RECORD_FIELD *status_field;

	mx_bool_type must_check_last_frame_number;
	mx_bool_type must_check_total_num_frames;
	mx_bool_type must_check_status;

	mx_bool_type last_frame_number_changed;
	mx_bool_type total_num_frames_changed;
	mx_bool_type status_changed;

	mx_bool_type send_last_frame_number_callbacks;
	mx_bool_type send_total_num_frames_callbacks;
	mx_bool_type send_status_callbacks;
	mx_bool_type send_extended_status_callbacks;

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

	ad = record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			record->name );
	}

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s invoked for record field '%s.%s'",
		fname, record->name, record_field->name ));
#endif

	/* What we do here depends on which field this is. */

	switch( record_field->label_value ) {
	case MXLV_AD_LAST_FRAME_NUMBER:
		must_check_last_frame_number = TRUE;
		must_check_total_num_frames = FALSE;
		must_check_status = FALSE;
		break;
	case MXLV_AD_TOTAL_NUM_FRAMES:
		must_check_last_frame_number = FALSE;
		must_check_total_num_frames = TRUE;
		must_check_status = FALSE;
		break;
	case MXLV_AD_STATUS:
		must_check_last_frame_number = FALSE;
		must_check_total_num_frames = FALSE;
		must_check_status = TRUE;
		break;
	case MXLV_AD_EXTENDED_STATUS:
		must_check_last_frame_number = TRUE;
		must_check_total_num_frames = TRUE;
		must_check_status = TRUE;
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The record field '%s' passed to this function for record '%s' "
		"is not one of the allowed values of 'last_frame_number', "
		"'total_num_frames', 'status', or 'extended_status'",
			record_field->name, record->name );
	}

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: must_check_last_frame_number = %d",
			fname, (int) must_check_last_frame_number));
	MX_DEBUG(-2,("%s: must_check_total_num_frames = %d",
			fname, (int) must_check_total_num_frames));
	MX_DEBUG(-2,("%s: must_check_status = %d",
			fname, (int) must_check_status));
#endif

	/* Get pointers to all of the fields. */

	extended_status_field =
	  &(record->record_field_array[ ad->extended_status_field_number ]);

	last_frame_number_field =
	    &(record->record_field_array[ ad->last_frame_number_field_number ]);

	total_num_frames_field =
	    &(record->record_field_array[ ad->total_num_frames_field_number ]);

	status_field =
		&(record->record_field_array[ ad->status_field_number ]);

	last_frame_number_changed = FALSE;
	total_num_frames_changed = FALSE;
	status_changed = FALSE;

	/* Test the last_frame_number field. */

	if ( must_check_last_frame_number ) {
		mx_status = mx_default_test_for_value_changed(
						last_frame_number_field,
						direction,
						&last_frame_number_changed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Test the total_num_frames field. */

	if ( must_check_total_num_frames ) {
		mx_status = mx_default_test_for_value_changed(
						total_num_frames_field,
						direction,
						&total_num_frames_changed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Test the status field. */

	if ( must_check_status ) {
		mx_status = mx_default_test_for_value_changed(
						status_field,
						direction,
						&status_changed );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: last_frame_number_changed = %d",
			fname, (int) last_frame_number_changed));
	MX_DEBUG(-2,("%s: total_num_frames_changed = %d",
			fname, (int) total_num_frames_changed));
	MX_DEBUG(-2,("%s: status_changed = %d",
			fname, (int) status_changed));
#endif

	/* If the last_frame_number field, the total_num_frames field,
	 * or the status field changed, then we must update the
	 * extended_status field to match.
	 */

	mx_area_detector_update_extended_status_string( ad );

	/* We do not send out value changed callbacks for the record field
	 * that was passed to us, since our caller will decide whether
	 * or not to do that.  However, we _do_ send out value changed
	 * callbacks for any other fields.
	 */

	send_last_frame_number_callbacks = FALSE;
	send_total_num_frames_callbacks  = FALSE;
	send_status_callbacks            = FALSE;
	send_extended_status_callbacks   = FALSE;

	if ( record_field == last_frame_number_field ) {
		if ( last_frame_number_changed ) {
			send_extended_status_callbacks = TRUE;
		}
		if ( total_num_frames_changed ) {
			send_total_num_frames_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_status_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
	} else
	if ( record_field == total_num_frames_field ) {
		if ( last_frame_number_changed ) {
			send_last_frame_number_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
		if ( total_num_frames_changed ) {
			send_extended_status_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_status_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
	} else
	if ( record_field == status_field ) {
		if ( last_frame_number_changed ) {
			send_last_frame_number_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
		if ( total_num_frames_changed ) {
			send_total_num_frames_callbacks = TRUE;
			send_extended_status_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_extended_status_callbacks = TRUE;
		}
	} else
	if ( record_field == extended_status_field ) {
		if ( last_frame_number_changed ) {
			send_last_frame_number_callbacks = TRUE;
		}
		if ( total_num_frames_changed ) {
			send_total_num_frames_callbacks = TRUE;
		}
		if ( status_changed ) {
			send_status_callbacks = TRUE;
		}
	}

#if MX_AREA_DETECTOR_DEBUG_VCTEST
	MX_DEBUG(-2,("%s: send_last_frame_number_callbacks = %d",
			fname, (int) send_last_frame_number_callbacks));
	MX_DEBUG(-2,("%s: send_total_num_frames_callbacks = %d",
			fname, (int) send_total_num_frames_callbacks));
	MX_DEBUG(-2,("%s: send_status_callbacks = %d",
			fname, (int) send_status_callbacks));
	MX_DEBUG(-2,("%s: send_extended_status_callbacks = %d",
			fname, (int) send_extended_status_callbacks));

	MX_DEBUG(-2,("%s: last_frame_number_field->callback_list = %p",
		fname, last_frame_number_field->callback_list));
	MX_DEBUG(-2,("%s: total_num_frames_field->callback_list = %p",
		fname, total_num_frames_field->callback_list));
	MX_DEBUG(-2,("%s: status_field->callback_list = %p",
		fname, status_field->callback_list));
	MX_DEBUG(-2,("%s: extended_status_field->callback_list = %p",
		fname, extended_status_field->callback_list));
#endif

	/* Send out last_frame_number callbacks. */

	if ( send_last_frame_number_callbacks ) {
		if ( last_frame_number_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				last_frame_number_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Send out total_num_frames callbacks. */

	if ( send_total_num_frames_callbacks ) {
		if ( total_num_frames_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				total_num_frames_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Send out status callbacks. */

	if ( send_status_callbacks ) {
		if ( status_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				status_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Send out extended_status callbacks. */

	if ( send_extended_status_callbacks ) {
		if ( extended_status_field->callback_list != NULL ) {

			mx_status = mx_local_field_invoke_callback_list(
				extended_status_field, MXCBT_VALUE_CHANGED );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	if ( value_changed_ptr != NULL ) {
		switch( record_field->label_value ) {
		case MXLV_AD_LAST_FRAME_NUMBER:
			*value_changed_ptr = last_frame_number_changed;
			break;
		case MXLV_AD_TOTAL_NUM_FRAMES:
			*value_changed_ptr = total_num_frames_changed;
			break;
		case MXLV_AD_STATUS:
			*value_changed_ptr = status_changed;
			break;
		case MXLV_AD_EXTENDED_STATUS:
			*value_changed_ptr = last_frame_number_changed
						| total_num_frames_changed
						| status_changed;
			break;
		}

#if MX_AREA_DETECTOR_DEBUG_VCTEST
		MX_DEBUG(-2,("%s: '%s.%s' value_changed = %d.",
			fname, record->name, record_field->name,
			*value_changed_ptr));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT void
mx_area_detector_update_extended_status_string( MX_AREA_DETECTOR *ad )
{
#if MX_AREA_DETECTOR_DEBUG_STATUS
	static const char fname[] =
		"mx_area_detector_update_extended_status_string()";
#endif

	if ( ad == (MX_AREA_DETECTOR *) NULL )
		return;

	if ( ad->correction_measurement_in_progress
	   || ( ad->correction_measurement != NULL ) )
	{
		ad->status |= MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;
	}

	snprintf( ad->extended_status, sizeof(ad->extended_status),
		"%ld %ld %#lx",
			ad->last_frame_number,
			ad->total_num_frames,
			ad->status );

#if MX_AREA_DETECTOR_DEBUG_STATUS
	MX_DEBUG(-2,("%s: "
	"last_frame_number = %ld, total_num_frames = %ld, status = %#lx, "
	"correction_measurement_in_progress = %d, correction_measurement = %p",
	    fname, ad->last_frame_number, ad->total_num_frames, ad->status,
		ad->correction_measurement_in_progress,
		ad->correction_measurement));
	MX_DEBUG(-2,("%s: ad->extended_status = '%s'",
		fname, ad->extended_status));
#endif
	return;
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
				new_binsize = mx_round( original_binsize );

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

	double x_binsize, y_binsize;
#if 0
	double x_framesize, y_framesize;
#endif

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

#if 0
	x_framesize = mx_divide_safely( ad->maximum_framesize[0],
						ad->binsize[0] );

	y_framesize = mx_divide_safely( ad->maximum_framesize[1],
						ad->binsize[1] );

	ad->framesize[0] = mx_round( x_framesize );
	ad->framesize[1] = mx_round( y_framesize );
#else
	ad->framesize[0] = ad->maximum_framesize[0] / ad->binsize[0];
	ad->framesize[1] = ad->maximum_framesize[1] / ad->binsize[1];
#endif

	return MX_SUCCESSFUL_RESULT;
}

