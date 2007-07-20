/*
 * Name:    mx_area_detector.c
 *
 * Purpose: Functions for reading frames from an area detector.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG    FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
#include "mx_key.h"
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

	ad->correction_measurement = NULL;

	ad->maximum_frame_number = 0;
	ad->last_frame_number = -1;
	ad->total_num_frames = -1;
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

	ad->transfer_destination_frame = NULL;
	ad->dezinger_threshold = 1.0;

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

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s complete for area detector '%s'.",
		fname, ad->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

static mx_status_type
mxp_area_detector_find_long_parameter_field( MX_RECORD *record,
						char *parameter_name,
						MX_RECORD_FIELD **field )
{
	static const char fname[] =
			"mxp_area_detector_find_long_parameter_field()";

	mx_status_type mx_status;

	mx_status = mx_find_record_field( record, parameter_name, field );

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
		"with this function.", parameter_name,
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
			(*field)->num_dimensions, parameter_name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_get_long_parameter( MX_RECORD *record,
					char *parameter_name,
					long *long_parameter )
{
	static const char fname[] = "mx_area_detector_get_long_parameter()";

	MX_RECORD_FIELD *field;
	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_AREA_DETECTOR * );
	void *value_ptr;
	long long_value;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( parameter_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The parameter_name pointer passed was NULL." );
	}

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_area_detector_default_get_parameter_handler;
	}

	mx_status = mxp_area_detector_find_long_parameter_field(record,
							parameter_name, &field);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_ptr = mx_get_field_value_pointer( field );

	/* Read the parameter value from the detector. */

	ad->parameter_type = field->label_value;

	mx_status = (*get_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Convert the value returned to a long integer. */

	switch( field->datatype ){
	case MXFT_CHAR:
		long_value = *((char *) value_ptr);
		break;
	case MXFT_UCHAR:
		long_value = *((unsigned char *) value_ptr);
		break;
	case MXFT_SHORT:
		long_value = *((short *) value_ptr);
		break;
	case MXFT_USHORT:
		long_value = *((unsigned short *) value_ptr);
		break;
	case MXFT_BOOL:
		long_value = *((mx_bool_type *) value_ptr);
		break;
	case MXFT_LONG:
		long_value = *((long *) value_ptr);
		break;
	case MXFT_ULONG:
	case MXFT_HEX:
		long_value = *((unsigned long *) value_ptr);
		break;
	case MXFT_INT64:
		long_value = *((int64_t *) value_ptr);
		break;
	case MXFT_UINT64:
		long_value = *((uint64_t *) value_ptr);
		break;
	case MXFT_FLOAT:
		long_value = mx_round( *((float *) value_ptr) );
		break;
	case MXFT_DOUBLE:
		long_value = mx_round( *((double *) value_ptr) );
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Unsupported data type %ld requested for parameter '%s'.",
			field->datatype, parameter_name );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: parameter_name = '%s', parameter_value = %ld",
		fname, parameter_name, long_value ));
#endif

	if ( long_parameter != NULL ) {
		*long_parameter = long_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_area_detector_set_long_parameter( MX_RECORD *record,
					char *parameter_name,
					long *long_parameter )
{
	static const char fname[] = "mx_area_detector_set_long_parameter()";

	MX_RECORD_FIELD *field;
	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_AREA_DETECTOR * );
	void *value_ptr;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( parameter_name == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The parameter_name pointer passed was NULL." );
	}

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_area_detector_default_set_parameter_handler;
	}

	mx_status = mxp_area_detector_find_long_parameter_field(record,
							parameter_name, &field);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( long_parameter == NULL ) {
		/* If long_parameter is a NULL pointer, then we assume that the
		 * correct value for the parameter was copied to the field
		 * before the current function was called.
		 */

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: parameter_name = '%s'",
			fname, parameter_name ));
#endif
	} else {
		/* Otherwise, we must copy the parameter value to the field. */

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: parameter_name = '%s', parameter_value = %ld",
			fname, parameter_name, *long_parameter ));
#endif

		value_ptr = mx_get_field_value_pointer( field );

		switch( field->datatype ){
		case MXFT_CHAR:
			*((char *) value_ptr) = (char) *long_parameter;
			break;
		case MXFT_UCHAR:
			*((unsigned char *) value_ptr)
					= (unsigned char) *long_parameter;
			break;
		case MXFT_SHORT:
			*((short *) value_ptr) = (short) *long_parameter;
			break;
		case MXFT_USHORT:
			*((unsigned short *) value_ptr)
					= (unsigned short) *long_parameter;
			break;
		case MXFT_BOOL:
			*((mx_bool_type *) value_ptr)
					= (mx_bool_type) *long_parameter;
			break;
		case MXFT_LONG:
			*((long *) value_ptr) = *long_parameter;
			break;
		case MXFT_ULONG:
		case MXFT_HEX:
			*((unsigned long *) value_ptr)
					= (unsigned long) *long_parameter;
			break;
		case MXFT_INT64:
			*((int64_t *) value_ptr) = (int64_t) *long_parameter;
			break;
		case MXFT_UINT64:
			*((uint64_t *) value_ptr) = (uint64_t) *long_parameter;
			break;
		case MXFT_FLOAT:
			*((float *) value_ptr) = (float) *long_parameter;
			break;
		case MXFT_DOUBLE:
			*((double *) value_ptr) = (double) *long_parameter;
			break;
		default:
			return mx_error( MXE_TYPE_MISMATCH, fname,
		    "Unsupported data type %ld requested for parameter '%s'.",
				field->datatype, parameter_name );
		}
	}

	/* Send the new parameter value to the detector. */

	ad->parameter_type = field->label_value;

	mx_status = (*set_parameter_fn)( ad );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
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
		"outside the range of allowed values (0-%ld).",
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

	mx_status = (*measure_correction_fn)( ad );

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

	ad->parameter_type = MXLV_AD_USE_SCALED_DARK_CURRENT;
	ad->use_scaled_dark_current = use_scaled_dark_current;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

/*---*/

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

	if ( sequence_parameters == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_PARAMETERS pointer passed was NULL." );
	}

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
					double exposure_time_per_line )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_STREAK_CAMERA;
	seq_params.num_parameters = 2;
	seq_params.parameter_array[0] = num_lines;
	seq_params.parameter_array[1] = exposure_time_per_line;

	mx_status = mx_area_detector_set_sequence_parameters( record,
								&seq_params );
	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_subimage_mode( MX_RECORD *record,
					long num_lines_per_subimage,
					long num_subimages,
					double exposure_time,
					double subimage_time )
{
	MX_SEQUENCE_PARAMETERS seq_params;
	mx_status_type mx_status;

	seq_params.sequence_type = MXT_SQ_SUBIMAGE;
	seq_params.num_parameters = 4;
	seq_params.parameter_array[0] = num_lines_per_subimage;
	seq_params.parameter_array[1] = num_subimages;
	seq_params.parameter_array[2] = exposure_time;
	seq_params.parameter_array[3] = subimage_time;

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
	unsigned long status_flags, mask;
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_get_status( record, &status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy != NULL ) {
		mask = MXSF_AD_IS_BUSY
			| MXSF_AD_CORRECTION_IN_PROGRESS
			| MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;

		if ( ad->status & mask ) {
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

	if ( ad->correction_measurement != NULL ) {
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

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure the frame is big enough. */

	mx_status = mx_image_alloc( image_frame,
					MXT_IMAGE_LOCAL_1D_ARRAY,
					ad->framesize,
					ad->image_format,
					ad->byte_order,
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

	ad->transfer_destination_frame = destination_frame;

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

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Dark current frame '%s' exposure time = %g",
		    fname, frame_filename, ad->dark_current_exposure_time)); 
#endif
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

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Flood field frame '%s', average intensity = %g ADU.",
		    fname, frame_filename, ad->flood_field_average_intensity));
		MX_DEBUG(-2,
	("%s:   num_unmasked_pixels = %lu, unmasked_integrated_intensity = %g",
		    fname, num_unmasked_pixels, unmasked_integrated_intensity));
#endif
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
				long destination_frame_type,
				long source_frame_type )
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

	ad->copy_frame[0] = destination_frame_type & MXFT_AD_ALL;

	ad->copy_frame[1] = source_frame_type & MXFT_AD_ALL;

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
						MXFT_AD_IMAGE_FRAME, NULL );

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
	unsigned long image_bytes_per_row;
	unsigned long roi_bytes_per_frame, roi_bytes_per_row;
	unsigned long x_min, y_min, y, x_offset;
	char *roi_row_ptr, *image_row_ptr, *roi_data_ptr, *image_data_ptr;
	char *image_row_data_ptr;
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

	(*roi_frame)->image_type = MXT_IMAGE_LOCAL_1D_ARRAY;
	(*roi_frame)->framesize[0] = ad->roi[1] - ad->roi[0] + 1;
	(*roi_frame)->framesize[1] = ad->roi[3] - ad->roi[2] + 1;

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		(*roi_frame)->image_format    = ad->image_format;
		(*roi_frame)->byte_order      = ad->byte_order;
		(*roi_frame)->bytes_per_pixel = ad->bytes_per_pixel;
	} else {
		(*roi_frame)->image_format    = image_frame->image_format;
		(*roi_frame)->byte_order      = image_frame->byte_order;
		(*roi_frame)->bytes_per_pixel = image_frame->bytes_per_pixel;
	}

	roi_bytes_per_frame =
		(*roi_frame)->framesize[0] * (*roi_frame)->framesize[1]
			* (*roi_frame)->bytes_per_pixel;

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

/*-----------------------------------------------------------------------*/

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

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: record '%s', correction_flags = %#lx",
		fname, ad->record->name, flags));
#endif

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

	MX_IMAGE_FRAME *destination_frame, *source_frame;
	mx_status_type mx_status;

	if ( ad->transfer_frame == MXFT_AD_IMAGE_FRAME ) {

		/* The image frame should already be in the right place,
		 * so we do not need to do anything.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( ad->transfer_destination_frame == (MX_IMAGE_FRAME *) NULL ) {
		destination_frame = ad->image_frame;
	} else {
		destination_frame = ad->transfer_destination_frame;
	}

	switch( ad->transfer_frame ) {
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

	mx_status = mx_image_copy_frame( &destination_frame, source_frame );

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

	mx_status = mx_image_alloc( frame_ptr,
					MXT_IMAGE_LOCAL_1D_ARRAY,
					ad->framesize,
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
	}

	mx_status = mx_image_copy_frame( dest_frame_ptr, src_frame );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: frame %#lx successfully copied to frame %#lx.",
		fname, ad->copy_frame[0], ad->copy_frame[1]));
#endif

	return MX_SUCCESSFUL_RESULT;
}

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

		/* We just return the value that is already in the 
		 * data structure.
		 */

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

	case MXLV_AD_DETECTOR_READOUT_TIME:
		ad->detector_readout_time = 0;
		break;

	case MXLV_AD_TOTAL_SEQUENCE_TIME:
		mx_status = mx_area_detector_get_sequence_parameters(
							ad->record, &seq );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_area_detector_get_detector_readout_time(
							ad->record, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		switch( seq.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_CONTINUOUS:
			exposure_time = seq.parameter_array[0];

			ad->total_sequence_time =
				exposure_time + ad->detector_readout_time;
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

			ad->total_sequence_time =
				frame_time * (double) num_frames;
			break;
		case MXT_SQ_STROBE:
			num_frames = seq.parameter_array[0];
			exposure_time = seq.parameter_array[1];

			ad->total_sequence_time =
				( exposure_time + ad->detector_readout_time )
					* (double) num_frames;
			break;
		case MXT_SQ_BULB:
			num_frames = seq.parameter_array[0];

			ad->total_sequence_time = ad->detector_readout_time
							* (double) num_frames;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Area detector '%s' is configured for unsupported "
			"sequence type %ld.",
				ad->record->name,
				seq.sequence_type );
		}
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

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_BINSIZE:
	case MXLV_AD_IMAGE_FORMAT:
	case MXLV_AD_IMAGE_FORMAT_NAME:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_AD_TRIGGER_MODE:
	case MXLV_AD_ROI_NUMBER:
	case MXLV_AD_CORRECTION_FLAGS:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

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

MX_EXPORT mx_status_type
mx_area_detector_default_measure_correction( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_measure_correction()";

	MX_IMAGE_FRAME *dest_frame;
	void *void_image_data_pointer;
	unsigned long saved_correction_flags, desired_correction_flags;
	uint16_t *src_array, *dest_array;
	double *sum_array;
	double exposure_time, temp_double;
	long i, n, num_exposures, pixels_per_frame;
	size_t image_length;
	mx_bool_type busy;
	mx_status_type mx_status;

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

	/* Get a pointer to the destination array. */

	mx_status = mx_image_get_image_data_pointer( dest_frame,
						&image_length,
						&void_image_data_pointer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dest_array = void_image_data_pointer;

	/* Allocate a double precision array to store intermediate sums. */

	sum_array = calloc( pixels_per_frame, sizeof(double) );

	if ( sum_array == (double *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate "
		"a %ld element array of doubles.", pixels_per_frame );
	}

	/* Put the area detector in One-shot mode. */

	mx_status = mx_area_detector_set_one_shot_mode( ad->record,
							exposure_time );

	if ( mx_status.code != MXE_SUCCESS ) {
		free( sum_array );
		return mx_status;
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
			free( sum_array );
			return mx_status;
		}

		/* Wait for the exposure to end. */

		for(;;) {
			mx_status = mx_area_detector_is_busy(ad->record, &busy);

			if ( mx_status.code != MXE_SUCCESS ) {
				free( sum_array );
				return mx_status;
			}

#if 0
			if ( mx_kbhit() ) {
				(void) mx_getch();

				MX_DEBUG(-2,("%s: INTERRUPTED", fname));

				free( sum_array );
				return mx_area_detector_stop( ad->record );
			}
#endif

			if ( busy == FALSE ) {
				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		/* Readout the frame into ad->image_frame. */

		mx_status = mx_area_detector_readout_frame( ad->record, 0 );

		if ( mx_status.code != MXE_SUCCESS ) {
			free( sum_array );
			return mx_status;
		}

		/* Perform any necessary image corrections. */

		if ( desired_correction_flags != 0 ) {
			saved_correction_flags = ad->correction_flags;

			ad->correction_flags = desired_correction_flags;

			mx_status = mx_area_detector_correct_frame(ad->record);

			ad->correction_flags = saved_correction_flags;

			if ( mx_status.code != MXE_SUCCESS ) {
				free( sum_array );
				return mx_status;
			}
		}

		/* Get the image data pointer. */

		mx_status = mx_image_get_image_data_pointer( ad->image_frame,
						&image_length,
						&void_image_data_pointer );

		if ( mx_status.code != MXE_SUCCESS ) {
			free( sum_array );
			return mx_status;
		}

		src_array = void_image_data_pointer;

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: image_frame = %p, image_data = %p, src_array = %p",
			fname, ad->image_frame, ad->image_frame->image_data,
			src_array));
#endif

		/* Add the pixels in this image to the sum array. */

		for ( i = 0; i < pixels_per_frame; i++ ) {
			sum_array[i] += (double) src_array[i];
		}

#if 0
		MX_DEBUG(-2,("%s: n = %ld", fname, n));

		for ( i = 0; i < 10; i++ ) {
			MX_DEBUG(-2,
			(" src_array[%ld] = %ld, sum_array[%ld] = %g",
			i, (long) src_array[i], i, sum_array[i]));
		}
#endif
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: Calculating normalized pixels.", fname));
#endif

	/* Copy normalized pixels to the destination array. */

	for ( i = 0; i < pixels_per_frame; i++ ) {
		temp_double = sum_array[i] / num_exposures;

		dest_array[i] = mx_round( temp_double );
	}

	free( sum_array );

#if MX_AREA_DETECTOR_DEBUG
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
	mx_bool_type busy;
	mx_status_type mx_status;

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
			mx_status = mx_area_detector_is_busy(ad->record, &busy);

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

			if ( busy == FALSE ) {
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
			saved_correction_flags = ad->correction_flags;

			ad->correction_flags = desired_correction_flags;

			mx_status = mx_area_detector_correct_frame( ad->record);

			ad->correction_flags = saved_correction_flags;

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

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s invoked for area detector '%s'.",
		fname, record->name ));
#endif

	mx_status = mx_area_detector_get_pointers(record, &ad, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: image_frame = %p", fname, image_frame));
	MX_DEBUG(-2,("%s: mask_frame = %p", fname, mask_frame));
	MX_DEBUG(-2,("%s: bias_frame = %p", fname, bias_frame));
	MX_DEBUG(-2,("%s: dark_current_frame = %p", fname, dark_current_frame));
	MX_DEBUG(-2,("%s: flood_field_frame = %p", fname, flood_field_frame));
#endif

	if ( image_frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"No primary image frame was provided." );
	}

	if ( ad->correction_flags == 0 ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: No corrections requested.", fname));
#endif

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

