/*
 * Name:    mx_video_input.c
 *
 * Purpose: Functions for reading frames from a video input source.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2006-2007, 2009, 2011-2012, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_VIDEO_INPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_hrt.h"
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

#if 0
	if ( *vinput != NULL ) {
		MX_DEBUG(-2,("*** %s: bytes_per_frame = %ld",
			calling_fname, (*vinput)->bytes_per_frame));
		MX_DEBUG(-2,("*** %s: bytes_per_pixel = %g",
			calling_fname, (*vinput)->bytes_per_pixel));
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mx_video_input_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->maximum_framesize[0] = -1;
	vinput->maximum_framesize[1] = -1;
	vinput->asynchronous_capture = -1;
	vinput->maximum_frame_number = 0;
	vinput->last_frame_number = -1;
	vinput->total_num_frames = -1;
	vinput->status = 0;
	vinput->extended_status[0] = '\0';
	vinput->check_for_buffer_overrun = FALSE;
	vinput->num_capture_buffers = 1;

	vinput->frame = NULL;
	vinput->frame_buffer = NULL;

	mx_status = mx_image_get_image_format_type_from_name(
			vinput->image_format_name, &(vinput->image_format) );

	return mx_status;
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
	mx_bool_type would_exceed_limit;
	mx_status_type ( *set_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* NOTE: If maximum_framesize has a negative value, then our
	 * convention is that we ignore the value of maximum_framesize.
	 */

	would_exceed_limit = FALSE;

	if ( vinput->maximum_framesize[0] >= 0 ) {
		if ( x_framesize > vinput->maximum_framesize[0] ) {
			would_exceed_limit = TRUE;
		}
	} else
	if ( vinput->maximum_framesize[1] >= 0 ) {
		if ( y_framesize > vinput->maximum_framesize[1] ) {
			would_exceed_limit = TRUE;
		}
	}

	if ( would_exceed_limit ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested dimensions (%ld,%ld) for video input '%s' "
		"are outside the allowed range of (%ld,%ld).",
			x_framesize, y_framesize, record->name,
			vinput->maximum_framesize[0],
			vinput->maximum_framesize[1] );
	}

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

MX_EXPORT mx_status_type
mx_video_input_get_byte_order( MX_RECORD *record, long *byte_order )
{
	static const char fname[] = "mx_video_input_get_byte_order()";

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

	vinput->parameter_type = MXLV_VIN_BYTE_ORDER;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( byte_order != NULL ) {
		*byte_order = vinput->byte_order;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_get_bytes_per_frame( MX_RECORD *record, long *bytes_per_frame )
{
	static const char fname[] = "mx_video_input_get_bytes_per_frame()";

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

	vinput->parameter_type = MXLV_VIN_BYTES_PER_FRAME;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_per_frame != NULL ) {
		*bytes_per_frame = vinput->bytes_per_frame;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_bytes_per_frame( MX_RECORD *record, long bytes_per_frame )
{
	static const char fname[] = "mx_video_input_set_bytes_per_frame()";

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

	vinput->parameter_type = MXLV_VIN_BYTES_PER_FRAME;

	vinput->bytes_per_frame = bytes_per_frame;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_bytes_per_pixel( MX_RECORD *record, double *bytes_per_pixel )
{
	static const char fname[] = "mx_video_input_get_bytes_per_pixel()";

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

	vinput->parameter_type = MXLV_VIN_BYTES_PER_PIXEL;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_per_pixel != NULL ) {
		*bytes_per_pixel = vinput->bytes_per_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_bytes_per_pixel( MX_RECORD *record, double bytes_per_pixel )
{
	static const char fname[] = "mx_video_input_set_bytes_per_pixel()";

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

	vinput->parameter_type = MXLV_VIN_BYTES_PER_PIXEL;

	vinput->bytes_per_pixel = bytes_per_pixel;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_bits_per_pixel( MX_RECORD *record, long *bits_per_pixel )
{
	static const char fname[] = "mx_video_input_get_bits_per_pixel()";

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

	vinput->parameter_type = MXLV_VIN_BITS_PER_PIXEL;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bits_per_pixel != NULL ) {
		*bits_per_pixel = vinput->bits_per_pixel;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_bits_per_pixel( MX_RECORD *record, long bits_per_pixel )
{
	static const char fname[] = "mx_video_input_set_bits_per_pixel()";

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

	vinput->parameter_type = MXLV_VIN_BITS_PER_PIXEL;

	vinput->bits_per_pixel = bits_per_pixel;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_video_input_set_exposure_time( MX_RECORD *record, double exposure_time )
{
	static const char fname[] = "mx_video_input_set_exposure_time()";

	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
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

	vinput->parameter_type = MXLV_VIN_SEQUENCE_PARAMETER_ARRAY;

	/* Save the parameters, which will be used the next time that
	 * the video input "arms".
	 */

	sp = &(vinput->sequence_parameters);

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s: sp = %p", fname, sp));
#endif

	sp->sequence_type = MXT_SQ_ONE_SHOT;

	sp->num_parameters = 1;

	sp->parameter_array[0] = exposure_time;

	sp->parameter_array[1] = 0;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_set_stream_mode( MX_RECORD *record, double exposure_time )
{
	static const char fname[] = "mx_video_input_set_stream_mode()";

	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
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

	vinput->parameter_type = MXLV_VIN_SEQUENCE_PARAMETER_ARRAY;

	/* Save the parameters, which will be used the next time that
	 * the video input "arms".
	 */

	sp = &(vinput->sequence_parameters);

	sp->sequence_type = MXT_SQ_STREAM;

	sp->num_parameters = 1;

	sp->parameter_array[0] = exposure_time;

	sp->parameter_array[1] = 0;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_set_sequence_parameters( MX_RECORD *record,
			MX_SEQUENCE_PARAMETERS *sequence_parameters )
{
	static const char fname[] = "mx_video_input_set_sequence_parameters()";

	MX_VIDEO_INPUT *vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_VIDEO_INPUT * );
	long num_parameters;
	mx_status_type mx_status;

	if ( sequence_parameters == (MX_SEQUENCE_PARAMETERS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SEQUENCE_INFO pointer passed was NULL." );
	}

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_video_input_default_set_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_SEQUENCE_PARAMETER_ARRAY;

	/* Save the parameters, which will be used the next time that
	 * the video input "arms".
	 */

	sp = &(vinput->sequence_parameters);

	sp->sequence_type = sequence_parameters->sequence_type;

	num_parameters = sequence_parameters->num_parameters;

	if ( num_parameters > MXU_MAX_SEQUENCE_PARAMETERS ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The number of sequence parameters %ld requested for "
		"the sequence given to video input record '%s' "
		"is longer than the maximum allowed length of %d.",
			num_parameters, record->name,
			MXU_MAX_SEQUENCE_PARAMETERS );
	}

	sp->num_parameters = num_parameters;

	memcpy( sp->parameter_array, sequence_parameters->parameter_array,
			num_parameters * sizeof(double) );

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_trigger_mode( MX_RECORD *record, long *trigger_mode )
{
	static const char fname[] = "mx_video_input_get_trigger_mode()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_video_input_default_get_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_TRIGGER_MODE; 

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trigger_mode != (long *) NULL ) {
		*trigger_mode = vinput->trigger_mode;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_set_trigger_mode( MX_RECORD *record, long trigger_mode )
{
	static const char fname[] = "mx_video_input_set_trigger_mode()";

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

	vinput->parameter_type = MXLV_VIN_TRIGGER_MODE; 

	vinput->trigger_mode = trigger_mode;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

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

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Compute image frame parameters for later use. */

	mx_status = mx_video_input_get_image_format( vinput->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_framesize( vinput->record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_bytes_per_frame( vinput->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,
	("%s: image_format = %ld, framesize = (%ld,%ld), bytes_per_frame = %ld",
		fname, vinput->image_format,
		vinput->framesize[0],
		vinput->framesize[1],
		vinput->bytes_per_frame));
#endif

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

MX_EXPORT mx_status_type
mx_video_input_stop( MX_RECORD *record )
{
	static const char fname[] = "mx_video_input_stop()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *stop_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = flist->stop;

	if ( stop_fn != NULL ) {
		mx_status = (*stop_fn)( vinput );
	}

	vinput->asynchronous_capture = -1;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_abort( MX_RECORD *record )
{
	static const char fname[] = "mx_video_input_abort()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *abort_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	abort_fn = flist->abort;

	if ( abort_fn != NULL ) {
		mx_status = (*abort_fn)( vinput );
	}

	vinput->asynchronous_capture = -1;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_asynchronous_capture( MX_RECORD *record,
				long num_capture_frames,
				mx_bool_type circular )
{
	static const char fname[] = "mx_video_input_asynchronous_capture()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *asynchronous_capture_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	vinput->asynchronous_capture = num_capture_frames;
	vinput->asynchronous_circular = circular;

	asynchronous_capture_fn = flist->asynchronous_capture;

	if ( asynchronous_capture_fn != NULL ) {
		mx_status = (*asynchronous_capture_fn)( vinput );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_is_busy( MX_RECORD *record, mx_bool_type *busy )
{
	static const char fname[] = "mx_video_input_is_busy()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	unsigned long status_flags;
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_video_input_get_status( record, &status_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy != NULL ) {
		if ( vinput->status & MXSF_VIN_IS_BUSY ) {
			*busy = TRUE;
		} else {
			*busy = FALSE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_get_maximum_frame_number( MX_RECORD *record,
					unsigned long *maximum_frame_number )
{
	static const char fname[] = "mx_video_input_get_maximum_frame_number()";

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

	vinput->parameter_type = MXLV_VIN_MAXIMUM_FRAME_NUMBER;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( maximum_frame_number != NULL ) {
		*maximum_frame_number = vinput->maximum_frame_number;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_get_last_frame_number( MX_RECORD *record,
			long *last_frame_number )
{
	static const char fname[] = "mx_video_input_get_last_frame_number()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_last_frame_number_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type ( *get_extended_status_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_frame_number_fn = flist->get_last_frame_number;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_last_frame_number_fn != NULL ) {
		mx_status = (*get_last_frame_number_fn)( vinput );
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( vinput );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the last frame number for area detector '%s' "
		"is unsupported.", record->name );
	}

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s: last_frame_number = %ld",
		fname, vinput->last_frame_number));
#endif

	if ( last_frame_number != NULL ) {
		*last_frame_number = vinput->last_frame_number;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_total_num_frames( MX_RECORD *record,
			long *total_num_frames )
{
	static const char fname[] = "mx_video_input_get_total_num_frames()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_total_num_frames_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type ( *get_extended_status_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_total_num_frames_fn  = flist->get_total_num_frames;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_total_num_frames_fn != NULL ) {
		mx_status = (*get_total_num_frames_fn)( vinput );
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( vinput );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the total number of frames for area detector '%s' "
		"is unsupported.", record->name );
	}

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s: total_num_frames = %ld",
		fname, vinput->total_num_frames));
#endif

	if ( total_num_frames != NULL ) {
		*total_num_frames = vinput->total_num_frames;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_status( MX_RECORD *record,
			unsigned long *status_flags )
{
	static const char fname[] = "mx_video_input_get_status()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_status_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type ( *get_extended_status_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn            = flist->get_status;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_status_fn != NULL ) {
		mx_status = (*get_status_fn)( vinput );
	} else
	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( vinput );
	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Getting the detector status for area detector '%s' "
		"is unsupported.", record->name );
	}

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s: status = %#lx", fname, vinput->status));
#endif

	if ( status_flags != NULL ) {
		*status_flags = vinput->status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_extended_status( MX_RECORD *record,
			long *last_frame_number,
			long *total_num_frames,
			unsigned long *status_flags )
{
	static const char fname[] = "mx_video_input_get_extended_status()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_last_frame_number_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type ( *get_total_num_frames_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type ( *get_status_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type ( *get_extended_status_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_last_frame_number_fn = flist->get_last_frame_number;
	get_total_num_frames_fn  = flist->get_total_num_frames;
	get_status_fn            = flist->get_status;
	get_extended_status_fn   = flist->get_extended_status;

	if ( get_extended_status_fn != NULL ) {
		mx_status = (*get_extended_status_fn)( vinput );
	} else {
		if ( get_last_frame_number_fn == NULL ) {
			vinput->last_frame_number = -1;
		} else {
			mx_status = (*get_last_frame_number_fn)( vinput );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		}

		if ( get_total_num_frames_fn == NULL ) {
			vinput->total_num_frames = -1;
		} else {
			mx_status = (*get_total_num_frames_fn)( vinput );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		if ( get_status_fn == NULL ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Getting the detector status for area detector '%s' "
			"is unsupported.", record->name );
		} else {
			mx_status = (*get_status_fn)( vinput );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number,
		vinput->total_num_frames, vinput->status));
#endif

	if ( last_frame_number != NULL ) {
		*last_frame_number = vinput->last_frame_number;
	}
	if ( total_num_frames != NULL ) {
		*total_num_frames = vinput->total_num_frames;
	}
	if ( status_flags != NULL ) {
		*status_flags = vinput->status;
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_video_input_get_pixel_clock_frequency( MX_RECORD *record,
					double *pixel_clock_frequency )
{
	static const char fname[] =
			"mx_video_input_get_pixel_clock_frequency()";

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

	vinput->parameter_type = MXLV_VIN_PIXEL_CLOCK_FREQUENCY;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pixel_clock_frequency != NULL ) {
		*pixel_clock_frequency = vinput->pixel_clock_frequency;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_pixel_clock_frequency( MX_RECORD *record,
					double pixel_clock_frequency )
{
	static const char fname[] =
			"mx_video_input_set_pixel_clock_frequency()";

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

	vinput->parameter_type = MXLV_VIN_PIXEL_CLOCK_FREQUENCY;

	vinput->pixel_clock_frequency = pixel_clock_frequency;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_external_trigger_polarity( MX_RECORD *record,
					long *external_trigger_polarity )
{
	static const char fname[] =
			"mx_video_input_get_external_trigger_polarity()";

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

	vinput->parameter_type = MXLV_VIN_EXTERNAL_TRIGGER_POLARITY;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( external_trigger_polarity != NULL ) {
		*external_trigger_polarity = vinput->external_trigger_polarity;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_external_trigger_polarity( MX_RECORD *record,
					long external_trigger_polarity )
{
	static const char fname[] =
			"mx_video_input_set_external_trigger_polarity()";

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

	vinput->parameter_type = MXLV_VIN_EXTERNAL_TRIGGER_POLARITY;

	vinput->external_trigger_polarity = external_trigger_polarity;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_camera_trigger_polarity( MX_RECORD *record,
					long *camera_trigger_polarity )
{
	static const char fname[] =
			"mx_video_input_get_camera_trigger_polarity()";

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

	vinput->parameter_type = MXLV_VIN_CAMERA_TRIGGER_POLARITY;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( camera_trigger_polarity != NULL ) {
		*camera_trigger_polarity = vinput->camera_trigger_polarity;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_camera_trigger_polarity( MX_RECORD *record,
					long camera_trigger_polarity )
{
	static const char fname[] =
			"mx_video_input_set_camera_trigger_polarity()";

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

	vinput->parameter_type = MXLV_VIN_CAMERA_TRIGGER_POLARITY;

	vinput->camera_trigger_polarity = camera_trigger_polarity;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_master_clock( MX_RECORD *record, long *master_clock )
{
	static const char fname[] = "mx_video_input_get_master_clock()";

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

	vinput->parameter_type = MXLV_VIN_MASTER_CLOCK;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( master_clock != NULL ) {
		*master_clock = vinput->master_clock;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_set_master_clock( MX_RECORD *record, long master_clock )
{
	static const char fname[] = "mx_video_input_set_master_clock()";

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

	vinput->parameter_type = MXLV_VIN_MASTER_CLOCK;

	vinput->master_clock = master_clock;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_video_input_mark_frame_as_saved( MX_RECORD *record,
					long frame_number )
{
	static const char fname[] = "mx_video_input_mark_frame_as_saved()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers( record,
						&vinput, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_video_input_default_set_parameter_handler;
	}

	vinput->mark_frame_as_saved = frame_number;

	vinput->parameter_type = MXLV_VIN_MARK_FRAME_AS_SAVED;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_video_input_check_for_buffer_overrun( MX_RECORD *record,
					mx_bool_type enable_checking )
{
	static const char fname[] = "mx_video_input_check_for_buffer_overrun()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *set_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers( record,
						&vinput, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = flist->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_video_input_default_set_parameter_handler;
	}

	vinput->check_for_buffer_overrun = enable_checking;

	vinput->parameter_type = MXLV_VIN_CHECK_FOR_BUFFER_OVERRUN;

	mx_status = (*set_parameter_fn)( vinput );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_video_input_get_num_capture_buffers( MX_RECORD *record,
					unsigned long *num_capture_buffers )
{
	static const char fname[] = "mx_video_input_get_num_capture_buffers()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_parameter_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers( record,
						&vinput, &flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = flist->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_video_input_default_get_parameter_handler;
	}

	vinput->parameter_type = MXLV_VIN_NUM_CAPTURE_BUFFERS;

	mx_status = (*get_parameter_fn)( vinput );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_capture_buffers != (unsigned long *) NULL ) {
		*num_capture_buffers = vinput->num_capture_buffers;
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_video_input_get_frame( MX_RECORD *record,
			long frame_number,
			MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mx_video_input_get_frame()";

	MX_VIDEO_INPUT *vinput;
	MX_VIDEO_INPUT_FUNCTION_LIST *flist;
	mx_status_type ( *get_frame_fn ) ( MX_VIDEO_INPUT * );
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	if ( frame_number < (-1) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal frame number %ld requested for video input '%s'.",
			frame_number, record->name );
	}

	/* Does this driver implement a get_frame function? */

	get_frame_fn = flist->get_frame;

	if ( get_frame_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Getting an MX_IMAGE_FRAME structure for the most recently "
		"taken image has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

	/* Make sure that the frame is big enough. */

#if MX_VIDEO_INPUT_DEBUG
	MX_DEBUG(-2,("%s: resizing *frame = %p", fname, *frame));
#endif

	mx_status = mx_image_alloc( frame,
				vinput->framesize[0],
				vinput->framesize[1],
				vinput->image_format,
				vinput->byte_order,
				vinput->bytes_per_pixel,
				MXT_IMAGE_HEADER_LENGTH_IN_BYTES,
				vinput->bytes_per_frame,
				NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now get the actual frame. */

	vinput->frame_number = frame_number;

	vinput->frame = *frame;

	mx_status = (*get_frame_fn)( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_video_input_get_sequence( MX_RECORD *record,
				long num_frames,
				MX_IMAGE_SEQUENCE **sequence )
{
	static const char fname[] = "mx_video_input_get_sequence()";

	MX_VIDEO_INPUT *vinput;
	mx_status_type mx_status;

	mx_status = mx_video_input_get_pointers(record, &vinput, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"Not yet implemented.");
}

MX_EXPORT mx_status_type
mx_video_input_get_frame_from_sequence( MX_IMAGE_SEQUENCE *image_sequence,
					long frame_number,
					MX_IMAGE_FRAME **image_frame )
{
	static const char fname[] = "mx_video_input_get_frame_from_sequence()";

	if ( frame_number < ( - image_sequence->num_frames ) ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Since the image sequence only has %ld frames, "
		"frame %ld would be before the first frame in the sequence.",
			image_sequence->num_frames, frame_number );
	} else
	if ( frame_number < 0 ) {

		/* -1 returns the last frame, -2 returns the second to last,
		 * and so forth.
		 */

		frame_number = image_sequence->num_frames - ( - frame_number );
	} else
	if ( frame_number >= image_sequence->num_frames ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Frame %ld would be beyond the last frame (%ld), "
		"since the sequence only has %ld frames.",
			frame_number, image_sequence->num_frames - 1,
			image_sequence->num_frames ) ;
	}

	*image_frame = image_sequence->frame_array[ frame_number ];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_video_input_default_get_parameter_handler( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mx_video_input_default_get_parameter_handler()";

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
	case MXLV_VIN_MAXIMUM_FRAMESIZE:
	case MXLV_VIN_BITS_PER_PIXEL:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_BYTE_ORDER:
	case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
	case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
	case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
	case MXLV_VIN_MASTER_CLOCK:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_NUM_CAPTURE_BUFFERS:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;
	case MXLV_VIN_MAXIMUM_FRAME_NUMBER:
		vinput->maximum_frame_number = 0;
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for video input '%s'.",
			mx_get_field_label_string( vinput->record,
						vinput->parameter_type ),
			vinput->parameter_type,
			vinput->record->name );
		break;
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
	case MXLV_VIN_MAXIMUM_FRAMESIZE:
	case MXLV_VIN_FORMAT:
	case MXLV_VIN_BYTE_ORDER:
	case MXLV_VIN_PIXEL_CLOCK_FREQUENCY:
	case MXLV_VIN_EXTERNAL_TRIGGER_POLARITY:
	case MXLV_VIN_CAMERA_TRIGGER_POLARITY:
	case MXLV_VIN_MASTER_CLOCK:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_MARK_FRAME_AS_SAVED:
	case MXLV_VIN_CHECK_FOR_BUFFER_OVERRUN:

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

