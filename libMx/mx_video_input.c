/*
 * Name:    mx_video_input.c
 *
 * Purpose: Functions for reading frames from a video input source.
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

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_video_input.h"

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_video_input_get_pointers( MX_RECORD *record,
			MX_VIDEO_INPUT **vinput,
			MX_VIDEO_INPUT_FUNCTION_LIST **flist_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_video_input_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( record->mx_class != MXC_VIDEO_INPUT ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not a video_input.",
			record->name, calling_fname );
	}

	if ( vinput != (MX_VIDEO_INPUT **) NULL ) {
		*vinput = (MX_VIDEO_INPUT *) (record->record_class_struct);

		if ( *vinput == (MX_VIDEO_INPUT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_VIDEO_INPUT pointer for record '%s' passed by '%s' is NULL",
				record->name, calling_fname );
		}
	}

	if ( flist_ptr != (MX_VIDEO_INPUT_FUNCTION_LIST **) NULL ) {
		*flist_ptr = (MX_VIDEO_INPUT_FUNCTION_LIST *)
				(record->class_specific_function_list);

		if ( *flist_ptr == (MX_VIDEO_INPUT_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_VIDEO_INPUT_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_video_input_get_image_format( MX_RECORD *record, long *format )
{
	static const char fname[] = "mx_video_input_get_image_format()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_video_input_default_get_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_FORMAT;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( format != NULL ) {
		*format = vinput->image_format;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_image_format( MX_RECORD *record, long format )
{
	static const char fname[] = "mx_video_input_set_image_format()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_video_input_default_set_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_FORMAT;

	vinput->image_format = format;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_framesize( MX_RECORD *record,
				long *x_framesize,
				long *y_framesize )
{
	static const char fname[] = "mx_video_input_get_framesize()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_video_input_default_get_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_FRAMESIZE;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_framesize != NULL ) {
		*x_framesize = vinput->framesize[0];
	}
	if ( y_framesize != NULL ) {
		*y_framesize = vinput->framesize[1];
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_framesize( MX_RECORD *record,
				long x_framesize,
				long y_framesize )
{
	static const char fname[] = "mx_video_input_set_framesize()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_video_input_default_set_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_FRAMESIZE;

	vinput->framesize[0] = x_framesize;
	vinput->framesize[1] = y_framesize;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

/*---*/

#if 0

MX_EXPORT mx_status_type
mx_video_input_set_exposure_time( MX_RECORD *record, double exposure_time )
{
}

MX_EXPORT mx_status_type
mx_video_input_set_sequence( MX_RECORD *record,
			MX_SEQUENCE_INFO *sequence_info )
{
}

MX_EXPORT mx_status_type
mx_video_input_set_continuous_mode( MX_RECORD *record, double exposure_time )
{
}

MX_EXPORT mx_status_type
mx_video_input_enable_external_trigger( MX_RECORD *record )
{
}

MX_EXPORT mx_status_type
mx_video_input_disable_external_trigger( MX_RECORD *record )
{
}
#endif

MX_EXPORT mx_status_type
mx_video_input_arm( MX_RECORD *record )
{
	static const char fname[] = "mx_video_input_arm()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *arm_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arm_fn = flist->arm;

	if ( arm_fn != NULL ) {
		mx_status = (*arm_fn)( vinput );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_trigger( MX_RECORD *record )
{
	static const char fname[] = "mx_video_input_trigger()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *trigger_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger_fn = flist->trigger;

	if ( trigger_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Triggering the taking of image frames has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

	mx_status = (*trigger_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_start( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_video_input_arm( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_trigger( record );

	return mx_status;
}

#if 0
MX_EXPORT mx_status_type
mx_video_input_stop( MX_RECORD *record )
{
}

MX_EXPORT mx_status_type
mx_video_input_abort( MX_RECORD *record )
{
}

MX_EXPORT mx_status_type
mx_video_input_is_busy( MX_RECORD *record, mx_bool_type *busy )
{
}

MX_EXPORT mx_status_type
mx_video_input_get_status( MX_RECORD *record,
			long *last_frame_number,
			unsigned long *status_flags )
{
}
#endif

/*---*/

MX_EXPORT mx_status_type
mx_video_input_get_frame( MX_RECORD *record, MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mx_video_input_get_frame()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_frame_fn ) (MX_VIDEO_INPUT *, MX_IMAGE_FRAME **);
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_frame_fn = flist->get_frame;

	if ( get_frame_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Getting an MX_IMAGE_FRAME structure for the most recently "
		"taken image has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

	mx_status = (*get_frame_fn)( vinput, frame );

	return mx_status;
}

#if 0

MX_EXPORT mx_status_type
mx_video_input_get_sequence( MX_RECORD *record,
			MX_IMAGE_SEQUENCE **sequence )
{
}

MX_EXPORT mx_status_type
mx_video_input_get_frame_from_sequence( MX_RECORD *record,
					long frame_number,
					MX_IMAGE_FRAME **image_frame )
{
}

MX_EXPORT mx_status_type
mx_video_input_read_1d_pixel_array( MX_IMAGE_FRAME *frame,
				long pixel_datatype,
				void *destination_pixel_array,
				size_t max_array_bytes,
				size_t *num_bytes_copied )
{
}

MX_EXPORT mx_status_type
mx_video_input_read_1d_pixel_sequence( MX_IMAGE_SEQUENCE *sequence,
				long pixel_datatype,
				void *destination_pixel_array,
				size_t max_array_bytes,
				size_t *num_bytes_copied )
{
}

#endif

MX_EXPORT mx_status_type
mx_video_input_default_get_parameter_handler( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mx_video_input_default_get_parameter_handler()";

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for video input '%s'.",
			mx_get_field_label_string( vinput->record,
						vinput->parameter_type ),
			vinput->parameter_type,
			vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_default_set_parameter_handler( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mx_video_input_default_set_parameter_handler()";

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for video input '%s'.",
			mx_get_field_label_string( vinput->record,
						vinput->parameter_type ),
			vinput->parameter_type,
			vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

