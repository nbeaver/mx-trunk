/*
 * Name:    mx_area_detector.c
 *
 * Purpose: Functions for reading frames from an area detector.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG    TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
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

	ad->correction_flags = MXFT_AD_ALL;

	ad->last_frame_number = -1;
	ad->status = 0;
	ad->extended_status[0] = '\0';

	ad->current_num_rois = ad->maximum_num_rois;
	ad->roi_number = 0;
	ad->roi[0] = 0;
	ad->roi[1] = 0;
	ad->roi[2] = 0;
	ad->roi[3] = 0;

	ad->roi_frame = NULL;
	ad->roi_frame_buffer = NULL;

	ad->image_frame = NULL;
	ad->image_frame_buffer = NULL;

	ad->frame_filename[0] = '\0';

	ad->mask_frame = NULL;
	ad->mask_frame_buffer = NULL;
	ad->mask_filename[0] = '\0';

	ad->bias_frame = NULL;
	ad->bias_frame_buffer = NULL;
	ad->bias_filename[0] = '\0';

	ad->use_scaled_dark_current = FALSE;
	ad->dark_current_exposure_time = 1.0;

	ad->dark_current_frame = NULL;
	ad->dark_current_frame_buffer = NULL;
	ad->dark_current_filename[0] = '\0';

	ad->flood_field_average_intensity = 1.0;

	ad->flood_field_frame = NULL;
	ad->flood_field_frame_buffer = NULL;
	ad->flood_field_filename[0] = '\0';

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

	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, ad->record->name));

	MX_DEBUG(-2,("%s: ad->image_format = %ld", fname, ad->image_format));
	MX_DEBUG(-2,("%s: ad->framesize = (%ld,%ld)",
		fname, ad->framesize[0], ad->framesize[1]));

	ad->correction_flags = 0;

	if ( strlen(ad->mask_filename) > 0 ) {
		mx_status = mx_area_detector_load_frame( record,
							MXFT_AD_MASK_FRAME,
							ad->mask_filename );

		if ( (mx_status.code != MXE_SUCCESS)
		  && (mx_status.code != MXE_NOT_FOUND) )
		{
			return mx_status;
		}

		ad->correction_flags |= MXFT_AD_MASK_FRAME;
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

		ad->correction_flags |= MXFT_AD_BIAS_FRAME;
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

		ad->correction_flags |= MXFT_AD_DARK_CURRENT_FRAME;
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

		ad->correction_flags |= MXFT_AD_FLOOD_FIELD_FRAME;
	}

	MX_DEBUG(-2,("%s complete for area detector '%s'.",
		fname, ad->record->name));

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_area_detector_get_property_value( MX_RECORD *record,
					char *property_name,
					long *property_value )
{
	static const char fname[] = "mx_area_detector_get_property_value()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( property_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The property_name pointer passed was NULL." );
	}

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Set the property name first. */

	strlcpy(ad->property_name, property_name, MXU_AD_PROPERTY_NAME_LENGTH);

	ad->parameter_type = MXLV_AD_PROPERTY_NAME;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read the property value. */

	ad->parameter_type = MXLV_AD_PROPERTY_VALUE;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( property_value != NULL ) {
		*property_value = ad->property_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_property_value( MX_RECORD *record,
					char *property_name,
					long property_value )
{
	static const char fname[] = "mx_area_detector_set_property_value()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( property_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The property_name pointer passed was NULL." );
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Set the property name first. */

	strlcpy(ad->property_name, property_name, MXU_AD_PROPERTY_NAME_LENGTH);

	ad->parameter_type = MXLV_AD_PROPERTY_NAME;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now write the property value. */

	ad->property_value = property_value;

	ad->parameter_type = MXLV_AD_PROPERTY_VALUE;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_property_string( MX_RECORD *record,
					char *property_name,
					char *property_string,
					size_t max_string_length )
{
	static const char fname[] = "mx_area_detector_get_property_string()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( property_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The property_name pointer passed was NULL." );
	}

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Set the property name first. */

	strlcpy(ad->property_name, property_name, MXU_AD_PROPERTY_NAME_LENGTH);

	ad->parameter_type = MXLV_AD_PROPERTY_NAME;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now read the property string. */

	ad->parameter_type = MXLV_AD_PROPERTY_STRING;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( property_string != NULL ) {
	    strlcpy( property_string, ad->property_string, max_string_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_property_string( MX_RECORD *record,
					char *property_name,
					char *property_string )
{
	static const char fname[] = "mx_area_detector_set_property_string()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( property_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The property_name pointer passed was NULL." );
	}
	if ( property_string == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The property_string pointer passed was NULL." );
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	/* Set the property name first. */

	strlcpy(ad->property_name, property_name, MXU_AD_PROPERTY_NAME_LENGTH);

	ad->parameter_type = MXLV_AD_PROPERTY_NAME;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now write the property string. */

	strlcpy( ad->property_string, property_string,
			MXU_AD_PROPERTY_STRING_LENGTH );

	ad->parameter_type = MXLV_AD_PROPERTY_STRING;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

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
				long roi_number,
				long *x_minimum,
				long *x_maximum,
				long *y_minimum,
				long *y_maximum )
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

	ad->roi_number = roi_number;

	ad->parameter_type = MXLV_AD_ROI;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_minimum != NULL ) {
		*x_minimum = ad->roi[0];
	}
	if ( x_maximum != NULL ) {
		*x_maximum = ad->roi[1];
	}
	if ( y_minimum != NULL ) {
		*y_minimum = ad->roi[2];
	}
	if ( y_maximum != NULL ) {
		*y_maximum = ad->roi[3];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_roi( MX_RECORD *record,
				long roi_number,
				long x_minimum,
				long x_maximum,
				long y_minimum,
				long y_maximum )
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

	ad->roi_number = roi_number;

	ad->parameter_type = MXLV_AD_ROI;

	ad->roi[0] = x_minimum;
	ad->roi[1] = x_maximum;
	ad->roi[2] = y_minimum;
	ad->roi[3] = y_maximum;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_subframe_size( MX_RECORD *record, long *num_columns )
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
mx_area_detector_set_subframe_size( MX_RECORD *record, long num_columns )
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

	ad->parameter_type   = MXLV_AD_CORRECTION_FLAGS;
	ad->correction_flags = correction_flags;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*---*/

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

	if ( sequence_parameters == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

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
	long num_parameters;
	mx_status_type mx_status;

	if ( sequence_parameters == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}

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

	sp->sequence_type = sequence_parameters->sequence_type;

	num_parameters = sequence_parameters->num_parameters;

	if ( num_parameters > MXU_MAX_SEQUENCE_PARAMETERS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of sequence parameters %ld requested for "
		"the sequence given to area detector record '%s' "
		"is longer than the maximum allowed length of %d.",
			num_parameters, record->name,
			MXU_MAX_SEQUENCE_PARAMETERS );
	}

	sp->num_parameters = num_parameters;

	memcpy( sp->parameter_array, sequence_parameters->parameter_array,
			num_parameters * sizeof(double) );

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
					double gap_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_MULTIFRAME;
	seq_params.num_parameters = 3;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = gap_time;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_circular_multiframe_mode( MX_RECORD *record,
						long num_frames,
						double exposure_time,
						double gap_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_CIRCULAR_MULTIFRAME;
	seq_params.num_parameters = 3;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = gap_time;

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

MX_EXPORT mx_status_type
mx_area_detector_set_geometrical_mode( MX_RECORD *record,
					long num_frames,
					double exposure_time,
					double gap_time,
					double exposure_multiplier,
					double gap_multiplier )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_GEOMETRICAL;
	seq_params.num_parameters = 5;
	seq_params.parameter_array[0] = num_frames;
	seq_params.parameter_array[1] = exposure_time;
	seq_params.parameter_array[2] = gap_time;
	seq_params.parameter_array[3] = exposure_multiplier;
	seq_params.parameter_array[4] = gap_multiplier;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

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

	arm_fn = flist->arm;

	if ( arm_fn != NULL ) {
		mx_status = (*arm_fn)( ad );
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

#if 0 && MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld",
		fname, ad->last_frame_number));
#endif

	if ( last_frame_number != NULL ) {
		*last_frame_number = ad->last_frame_number;
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

#if 0 && MX_AREA_DETECTOR_DEBUG
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
			unsigned long *status_flags )
{
	static const char fname[] = "mx_area_detector_get_extended_status()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_last_frame_number_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type ( *get_extended_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_frame_number_fn = flist->get_last_frame_number;
	get_status_fn            = flist->get_status;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( ad );
	} else {
		if ( get_last_frame_number_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the last frame number for area detector '%s' "
			"is unsupported.", record->name );
		}
		if ( get_status_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the detector status for area detector '%s' "
			"is unsupported.", record->name );
		}

		mx_status = (*get_last_frame_number_fn)( ad );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = (*get_status_fn)( ad );
	}

#if 0 && MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld, status = %#lx",
		fname, ad->last_frame_number, ad->status));
#endif

	if ( last_frame_number != NULL ) {
		*last_frame_number = ad->last_frame_number;
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

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the frame is big enough. */

	mx_status = mx_image_alloc( image_frame,
					MXT_IMAGE_LOCAL_1D_ARRAY,
					ad->framesize,
					ad->image_format,
					ad->pixel_order,
					ad->bytes_per_pixel,
					ad->header_length,
					ad->bytes_per_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ad->image_frame = *image_frame;

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

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does this driver implement a readout_frame function? */

	readout_frame_fn = flist->readout_frame;

	if ( readout_frame_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
    "Reading out an image frame for record '%s' has not yet been implemented.",
			record->name );
	}

	ad->frame_number  = frame_number;
	ad->readout_frame = frame_number;

	mx_status = (*readout_frame_fn)( ad );

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
				long frame_type )
{
	static const char fname[] = "mx_area_detector_transfer_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *transfer_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	transfer_frame_fn = flist->transfer_frame;

	if ( transfer_frame_fn == NULL ) {
		transfer_frame_fn = mx_area_detector_default_transfer_frame;
	}

	ad->transfer_frame = frame_type;

	mx_status = (*transfer_frame_fn)( ad );

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
	double unmasked_integrated_intensity;
	unsigned long i, num_unmasked_pixels, total_num_pixels;
	uint16_t *mask_data_array, *flood_field_data_array;
	mx_status_type ( *load_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	/* Some frame types need special things to happen after
	 * they are loaded.
	 */

	switch( frame_type ) {
	case MXFT_AD_DARK_CURRENT_FRAME:

		mx_status = mx_image_get_exposure_time(
					ad->dark_current_frame,
					&(ad->dark_current_exposure_time) );

		MX_DEBUG(-2,("%s: Dark current frame '%s' exposure time = %g",
		    fname, frame_filename, ad->dark_current_exposure_time)); 
		break;

	case MXFT_AD_FLOOD_FIELD_FRAME:
		/* Compute the average intensity of unmasked pixels
		 * in the image frame.
		 */

		flood_field_data_array = ad->flood_field_frame->image_data;

		total_num_pixels = ad->framesize[0] * ad->framesize[1];

		num_unmasked_pixels = 0;
		unmasked_integrated_intensity = 0.0;

		if ( ( ad->correction_flags & MXFT_AD_MASK_FRAME ) == 0 ) {
			mask_data_array = NULL;
		} else
		if ( ad->mask_frame == NULL ) {
			mask_data_array = NULL;
		} else {
			mask_data_array = ad->mask_frame->image_data;
		}

		for ( i = 0; i < total_num_pixels; i++ ) {

			if ( mask_data_array == NULL ) {
				num_unmasked_pixels++;

				unmasked_integrated_intensity
					+= (double) flood_field_data_array[i];
			} else
			if ( mask_data_array[i] != 0 ) {
				num_unmasked_pixels++;

				unmasked_integrated_intensity
					+= (double) flood_field_data_array[i];
			}
		}

		if ( num_unmasked_pixels == 0 ) {
			return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"Area detector '%s' is currently using a mask frame "
			"that does not contain _any_ unmasked pixels.",
				record->name );
		}

		ad->flood_field_average_intensity =
		   unmasked_integrated_intensity / (double) num_unmasked_pixels;

		MX_DEBUG(-2,
		("%s: Flood field frame '%s', average intensity = %g ADU.",
		    fname, frame_filename, ad->flood_field_average_intensity));
		MX_DEBUG(-2,
	("%s:   num_unmasked_pixels = %lu, unmasked_integrated_intensity = %g",
		    fname, num_unmasked_pixels, unmasked_integrated_intensity));
		break;
	}

	return MX_SUCCESSFUL_RESULT;
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

	save_frame_fn = flist->save_frame;

	if ( save_frame_fn == NULL ) {
		save_frame_fn = mx_area_detector_default_save_frame;
	}

	ad->save_frame = frame_type & MXFT_AD_ALL;

	strlcpy( ad->frame_filename, frame_filename, MXU_FILENAME_LENGTH );

	mx_status = (*save_frame_fn)( ad );

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
	mx_status_type ( *copy_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	copy_frame_fn = flist->copy_frame;

	if ( copy_frame_fn == NULL ) {
		copy_frame_fn = mx_area_detector_default_copy_frame;
	}

	ad->copy_frame[0] = source_frame_type & MXFT_AD_ALL;

	ad->copy_frame[1] = destination_frame_type & MXFT_AD_ALL;

	mx_status = (*copy_frame_fn)( ad );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_get_frame( MX_RECORD *record,
			long frame_number,
			MX_IMAGE_FRAME **image_frame )
{
	mx_status_type mx_status;

	mx_status = mx_area_detector_setup_frame( record, image_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_readout_frame( record, frame_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_correct_frame( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_transfer_frame( record,
						MXFT_AD_IMAGE_FRAME );

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
			long roi_number,
			MX_IMAGE_FRAME **roi_frame )
{
	static const char fname[] = "mx_area_detector_get_roi_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_roi_frame_fn ) ( MX_AREA_DETECTOR * );
	unsigned long image_bytes_per_row;
	unsigned long roi_bytes_per_frame, roi_bytes_per_row;
	unsigned long x_min, y_min, y, x_offset;
	char *roi_row_ptr, *image_row_ptr, *roi_data_ptr, *image_data_ptr;
	char *image_row_data_ptr;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If no image frame pointer is supplied, use the default one. */

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {

		if ( ad->image_frame == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"No MX_IMAGE_FRAME pointer was supplied." );
		} else {
			image_frame = ad->image_frame;
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

	(*roi_frame)->image_type = MXT_IMAGE_LOCAL_1D_ARRAY;
	(*roi_frame)->framesize[0] = ad->roi[1] - ad->roi[0] + 1;
	(*roi_frame)->framesize[1] = ad->roi[3] - ad->roi[2] + 1;
	(*roi_frame)->image_format = image_frame->image_format;
	(*roi_frame)->pixel_order  = image_frame->pixel_order;
	(*roi_frame)->bytes_per_pixel = image_frame->bytes_per_pixel;

	roi_bytes_per_frame =
		(*roi_frame)->framesize[0] * (*roi_frame)->framesize[1]
			* image_frame->bytes_per_pixel;

	/* See if the image buffer is already big enough for the image. */

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: (*roi_frame)->image_data = %p",
		fname, (*roi_frame)->image_data));
	MX_DEBUG(-2,
	("%s: (*roi_frame)->image_length = %lu, bytes_per_frame = %lu",
		fname, (unsigned long) (*roi_frame)->image_length,
		ad->bytes_per_frame));
#endif

	/* Setup the image data buffer. */

	if ( ((*roi_frame)->image_length == 0) && (roi_bytes_per_frame == 0)) {

		/* Zero length image buffers are not allowed. */

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
    "Area detector '%s' attempted to create a zero length ROI image buffer.",
			record->name );

	} else
	if ( ( (*roi_frame)->image_data != NULL )
	  && ( (*roi_frame)->image_length >= roi_bytes_per_frame ) )
	{
#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: The image buffer is already big enough.", fname));
#endif
	} else {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new image buffer of %lu bytes.",
			fname, roi_bytes_per_frame));
#endif
		/* If not, then allocate a new one. */

		if ( (*roi_frame)->image_data != NULL ) {
			free( (*roi_frame)->image_data );
		}

		(*roi_frame)->image_data = malloc( roi_bytes_per_frame );

		if ( (*roi_frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate a %ld byte ROI image buffer for "
			"area detector '%s'.",
				roi_bytes_per_frame, ad->record->name );
		}

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: allocated new frame buffer.", fname));
#endif
	}

#if 1  /* FIXME!!! - This should not be present in the final version. */
	memset( (*roi_frame)->image_data, 0, 50 );
#endif

	ad->roi_bytes_per_frame = roi_bytes_per_frame;

	(*roi_frame)->bytes_per_pixel = ad->bytes_per_pixel;
	(*roi_frame)->image_length    = ad->roi_bytes_per_frame;

	MX_DEBUG(-2,
	("%s: ROI image_length = %ld, ad->roi_bytes_per_frame = %ld",
	fname, (long) (*roi_frame)->image_length, ad->roi_bytes_per_frame));


	ad->roi_number = roi_number;

	ad->roi_frame = *roi_frame;

	/* If the driver has a get_roi_frame method, invoke it. */

	get_roi_frame_fn = flist->get_roi_frame;

	if ( get_roi_frame_fn != NULL ) {
		mx_status = (*get_roi_frame_fn)( ad );
	} else {
		/* Otherwise, we do the copy ourself. */

		image_data_ptr = image_frame->image_data;
		roi_data_ptr   = (*roi_frame)->image_data;

		x_min = ad->roi[0];
		y_min = ad->roi[2];

		x_offset = x_min * image_frame->bytes_per_pixel;

		image_bytes_per_row =
		    image_frame->framesize[0] * image_frame->bytes_per_pixel;

		roi_bytes_per_row =
		    (*roi_frame)->framesize[0] * (*roi_frame)->bytes_per_pixel;

		for ( y = 0; y < (*roi_frame)->framesize[1]; y++ ) {

			/* Construct the address of the first pixel
			 * for this row in the original image frame.
			 */

			image_row_ptr = image_data_ptr
				+ image_bytes_per_row * ( y + y_min );

			roi_row_ptr = roi_data_ptr + roi_bytes_per_row * y;

			/* Next construct a pointer to the start of the
			 * image data to be copied.
			 */

			image_row_data_ptr = image_row_ptr + x_offset;

			/* Now we may copy the data for this row. */

			memcpy( roi_row_ptr, image_row_data_ptr,
						roi_bytes_per_row );
		}
	}

	return mx_status;
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

	/* Determine which corrections are to be applied. */

	flags = ad->correction_flags;

	MX_DEBUG(-2,("%s: record '%s', correction_flags = %#lx",
		fname, ad->record->name, flags));

	if ( (flags & MXFT_AD_MASK_FRAME) == 0 ) {
		mask_frame = NULL;
	} else {
		mask_frame = ad->mask_frame;

		if ( mask_frame == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_NOT_READY, fname,
			"No mask frame has been loaded for area detector '%s'.",
				ad->record->name );
		}
	}

	if ( (flags & MXFT_AD_BIAS_FRAME) == 0 ) {
		bias_frame = NULL;
	} else {
		bias_frame = ad->bias_frame;

		if ( bias_frame == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_NOT_READY, fname,
			"No bias frame has been loaded for area detector '%s'.",
				ad->record->name );
		}
	}

	if ( (flags & MXFT_AD_DARK_CURRENT_FRAME) == 0 ) {
		dark_current_frame = NULL;
	} else {
		dark_current_frame = ad->dark_current_frame;

		if ( dark_current_frame == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_NOT_READY, fname,
		"No dark current frame has been loaded for area detector '%s'.",
				ad->record->name );
		}
	}

	if ( (flags & MXFT_AD_BIAS_FRAME) == 0 ) {
		flood_field_frame = NULL;
	} else {
		flood_field_frame = ad->flood_field_frame;

		if ( flood_field_frame == (MX_IMAGE_FRAME *) NULL ) {
			return mx_error( MXE_NOT_READY, fname,
		"No flood field frame has been loaded for area detector '%s'.",
				ad->record->name );
		}
	}

	mx_status = mx_area_detector_frame_correction( ad->record,
				image_frame, mask_frame, bias_frame,
				dark_current_frame, flood_field_frame );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_default_transfer_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_transfer_frame()";

	if ( ad->transfer_frame == MXFT_AD_IMAGE_FRAME ) {

		/* The image frame should already be in the right place,
		 * so we do not need to do anything.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Frame operation %#lx is not yet implemented.",
			ad->transfer_frame );
}

MX_EXPORT mx_status_type
mx_area_detector_default_load_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_load_frame()";

	MX_IMAGE_FRAME **frame_ptr;
	mx_status_type mx_status;

	MX_DEBUG(-2,
	("%s invoked for area detector '%s'.", fname, ad->record->name ));

	MX_DEBUG(-2,("%s: load_frame = %#lx, frame_filename = '%s'",
		fname, ad->load_frame, ad->frame_filename));

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
		break;
	}

	mx_status = mx_image_alloc( frame_ptr,
					MXT_IMAGE_LOCAL_1D_ARRAY,
					ad->framesize,
					ad->image_format,
					ad->pixel_order,
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

	if ( (*frame_ptr)->image_format != ad->image_format ) {
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The image format %ld for file '%s' does not match "
		"the expected image format %ld for area detector '%s'.",
			(*frame_ptr)->image_format, ad->frame_filename,
			ad->image_format, ad->record->name );
	}

	/* Does the frame we just loaded have the expected image dimensions? */

	if ( ( (*frame_ptr)->framesize[0] != ad->framesize[0] )
	  || ( (*frame_ptr)->framesize[1] != ad->framesize[1] ) )
	{
		return mx_error( MXE_CONFIGURATION_CONFLICT, fname,
		"The dimensions (%ld,%ld) of image file '%s' do not match the "
		"expected image dimensions (%ld,%ld) for area detector '%s.'",
			(*frame_ptr)->framesize[0], (*frame_ptr)->framesize[1],
			ad->frame_filename,
			ad->framesize[0], ad->framesize[1],
			ad->record->name );
	}


	MX_DEBUG(-2,("%s: frame file '%s' successfully loaded to %#lx",
		fname, ad->frame_filename, ad->load_frame));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_save_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_save_frame()";

	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	MX_DEBUG(-2,
	("%s invoked for area detector '%s'.", fname, ad->record->name ));

	MX_DEBUG(-2,("%s: save_frame = %ld, frame_filename = '%s'",
		fname, ad->save_frame, ad->frame_filename));

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
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported frame type %ld requested for area detector '%s'.",
			ad->save_frame, ad->record->name );
		break;
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

	MX_DEBUG(-2,("%s: frame %#lx successfully saved to frame file '%s'.",
		fname, ad->save_frame, ad->frame_filename));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_copy_frame( MX_AREA_DETECTOR *ad )
{
	static const char fname[] = "mx_area_detector_default_copy_frame()";

	MX_IMAGE_FRAME *src_frame;
	MX_IMAGE_FRAME** dest_frame_ptr;
	mx_status_type mx_status;

	MX_DEBUG(-2,
	("%s invoked for area detector '%s'.", fname, ad->record->name ));

	MX_DEBUG(-2,("%s: copy_frame = (%ld,%ld), frame_filename = '%s'",
	    fname, ad->copy_frame[0], ad->copy_frame[1], ad->frame_filename));

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
		break;
	}

	if ( src_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NOT_READY, fname,
		"Source image frame type %ld has not yet been initialized "
		"for area detector '%s'.",ad->copy_frame[1], ad->record->name );
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
		break;
	}

	mx_status = mx_image_copy_frame( dest_frame_ptr, src_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: frame %#lx successfully copied to frame %#lx.",
		fname, ad->copy_frame[0], ad->copy_frame[1]));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_default_get_parameter_handler( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_get_parameter_handler()";

	double double_value;

	switch( ad->parameter_type ) {
	case MXLV_AD_MAXIMUM_FRAMESIZE:
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_TRIGGER_MODE:
	case MXLV_AD_BYTES_PER_PIXEL:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;

	case MXLV_AD_BYTES_PER_FRAME:
		double_value = ad->bytes_per_pixel
		    * ((double) ad->framesize[0]) * ((double) ad->framesize[0]);

		ad->bytes_per_frame = mx_round( ceil(double_value) );
		break;

	case MXLV_AD_ROI_NUMBER:
		if ( (ad->roi_number < 0)
		  || (ad->roi_number >= ad->maximum_num_rois) )
		{
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The ROI number %ld for area detector '%s' is "
			"outside the range of allowed values (0-%ld).",
				ad->roi_number, ad->record->name,
				ad->maximum_num_rois - 1 );
		}
		break;

	case MXLV_AD_ROI:
		if ( (ad->roi_number < 0)
		  || (ad->roi_number >= ad->maximum_num_rois) )
		{
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The ROI number %ld for area detector '%s' is "
			"outside the range of allowed values (0-%ld).",
				ad->roi_number, ad->record->name,
				ad->maximum_num_rois - 1 );
		}
		ad->roi[0] = ad->roi_array[ ad->roi_number ][0];
		ad->roi[1] = ad->roi_array[ ad->roi_number ][1];
		ad->roi[2] = ad->roi_array[ ad->roi_number ][2];
		ad->roi[3] = ad->roi_array[ ad->roi_number ][3];
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

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_TRIGGER_MODE:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;

	case MXLV_AD_ROI_NUMBER:
		if ( (ad->roi_number < 0)
		  || (ad->roi_number >= ad->maximum_num_rois) )
		{
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The ROI number %ld for area detector '%s' is "
			"outside the range of allowed values (0-%ld).",
				ad->roi_number, ad->record->name,
				ad->maximum_num_rois - 1 );
		}
		break;

	case MXLV_AD_ROI:
		if ( (ad->roi_number < 0)
		  || (ad->roi_number >= ad->maximum_num_rois) )
		{
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The ROI number %ld for area detector '%s' is "
			"outside the range of allowed values (0-%ld).",
				ad->roi_number, ad->record->name,
				ad->maximum_num_rois - 1 );
		}
		ad->roi_array[ ad->roi_number ][0] = ad->roi[0];
		ad->roi_array[ ad->roi_number ][1] = ad->roi[1];
		ad->roi_array[ ad->roi_number ][2] = ad->roi[2];
		ad->roi_array[ ad->roi_number ][3] = ad->roi[3];
		break;
	case MXLV_AD_BYTES_PER_FRAME:
	case MXLV_AD_BYTES_PER_PIXEL:
	case MXLV_AD_MAXIMUM_FRAMESIZE:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Changing the parameter '%s' for record '%s' is not supported.",
			mx_get_field_label_string( ad->record,
				ad->parameter_type ), ad->record->name );
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

/*---*/

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
	uint16_t *image_data_array, *mask_data_array, *bias_data_array;
	uint16_t *dark_current_data_array, *flood_field_data_array;
	long i;
	uint16_t image_pixel, total_pixel_offset;
	unsigned long flags, big_image_pixel;
	double image_exposure_time, exposure_time_ratio;
	double actual_dark_current, flood_field_scale_factor;
	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: image_frame = %p", fname, image_frame));
	MX_DEBUG(-2,("%s: mask_frame = %p", fname, mask_frame));
	MX_DEBUG(-2,("%s: bias_frame = %p", fname, bias_frame));
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: flood_field_frame = %p", fname, flood_field_frame));

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No primary image frame was provided." );
	}

	if ( ad->correction_flags == 0 ) {
		MX_DEBUG(-2,("%s: No corrections requested.", fname));

		return MX_SUCCESSFUL_RESULT;
	}

	/* Area detector image correction is currently only supported
	 * for 16-bit greyscale images (MXT_IMAGE_FORMAT_GREY16).
	 */

	if ( image_frame->image_format != MXT_IMAGE_FORMAT_GREY16 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The primary image frame is not a 16-bit greyscale image." );
	}

	image_data_array = image_frame->image_data;

	flags = ad->correction_flags;

	if ( ( flags & MXFT_AD_MASK_FRAME ) == 0 ) {
		mask_data_array = NULL;
	} else
	if ( mask_frame == NULL ) {
		mask_data_array = NULL;
	} else {
		mask_data_array = mask_frame->image_data;
	}

	if ( ( flags & MXFT_AD_BIAS_FRAME ) == 0 ) {
		bias_data_array = NULL;
	} else
	if ( bias_frame == NULL ) {
		bias_data_array = NULL;
	} else {
		bias_data_array = bias_frame->image_data;
	}

	if ( ( flags & MXFT_AD_DARK_CURRENT_FRAME ) == 0 ) {
		dark_current_data_array = NULL;
	} else
	if ( dark_current_frame == NULL ) {
		dark_current_data_array = NULL;
	} else {
		dark_current_data_array = dark_current_frame->image_data;
	}

	if ( ( flags & MXFT_AD_FLOOD_FIELD_FRAME ) == 0 ) {
		flood_field_data_array = NULL;
	} else
	if ( flood_field_frame == NULL ) {
		flood_field_data_array = NULL;
	} else {
		flood_field_data_array = flood_field_frame->image_data;
	}

	mx_status = mx_image_get_exposure_time( ad->image_frame,
						&image_exposure_time );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ad->use_scaled_dark_current ) {
		exposure_time_ratio = mx_divide_safely( image_exposure_time,
					ad->dark_current_exposure_time );
	} else {
		exposure_time_ratio = 1.0;
	}

	for ( i = 0; i < image_frame->image_length; i++ ) {

		image_pixel = image_data_array[i];

		if ( mask_data_array != NULL ) {
			if ( mask_data_array[i] == 0 ) {

				/* If the pixel is masked off, skip this
				 * pixel and return to the top of the loop
				 * for the next pixel.
				 */

				continue;
			}
		}

		/* Compute the total offset to be subtracted from
		 * the raw pixel value.
		 */

		total_pixel_offset = 0;

		/* Add the detector bias. */

		if ( bias_data_array != NULL ) {
			total_pixel_offset += bias_data_array[i];
		}

		/* Add the rescaled dark current. */

		if ( dark_current_data_array != NULL ) {
			actual_dark_current = exposure_time_ratio
					* (double) dark_current_data_array[i];

			total_pixel_offset += mx_round( actual_dark_current );
		}

		if ( image_pixel < total_pixel_offset ) {
			image_pixel = 0;
		} else {
			image_pixel -= total_pixel_offset;
		}

		if ( flood_field_data_array != NULL ) {
			flood_field_scale_factor = mx_divide_safely(
					ad->flood_field_average_intensity,
					(double) flood_field_data_array[i] );

			big_image_pixel = mx_round(
			    flood_field_scale_factor * (double) image_pixel );

			if ( big_image_pixel > 65535 ) {
				image_pixel = 65535;
			} else {
				image_pixel = big_image_pixel;
			}
		}

		image_data_array[i] = image_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

