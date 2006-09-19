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

	ad->frame = NULL;
	ad->frame_buffer = NULL;

	mx_status = mx_get_image_format_type_from_name(
			ad->image_format_name, &(ad->image_format) );

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

	ad->parameter_type = MXLV_AD_FORMAT;

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

	ad->parameter_type = MXLV_AD_FORMAT;

	ad->image_format = format;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
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

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_set_exposure_time( MX_RECORD *record, double exposure_time )
{
	static const char fname[] = "mx_area_detector_set_exposure_time()";

	MX_AREA_DETECTOR *ad;
	MX_SEQUENCE_PARAMETERS *sp;
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

	ad->parameter_type = MXLV_AD_SEQUENCE_PARAMETER_ARRAY;

	/* Save the parameters, which will be used the next time that
	 * the area detector "arms".
	 */

	sp = &(ad->sequence_parameters);

	sp->sequence_type = MXT_SQ_ONE_SHOT;

	sp->num_parameters = 1;

	sp->parameter_array[0] = exposure_time;

	sp->parameter_array[1] = 0;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_set_continuous_mode( MX_RECORD *record, double exposure_time )
{
	static const char fname[] = "mx_area_detector_set_continuous_mode()";

	MX_AREA_DETECTOR *ad;
	MX_SEQUENCE_PARAMETERS *sp;
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

	ad->parameter_type = MXLV_AD_SEQUENCE_PARAMETER_ARRAY;

	/* Save the parameters, which will be used the next time that
	 * the area detector "arms".
	 */

	sp = &(ad->sequence_parameters);

	sp->sequence_type = MXT_SQ_CONTINUOUS;

	sp->num_parameters = 1;

	sp->parameter_array[0] = exposure_time;

	sp->parameter_array[1] = 0;

	mx_status = (*set_parameter_fn)( ad );

	return mx_status;
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

	ad->parameter_type = MXLV_AD_SEQUENCE_PARAMETER_ARRAY;

	/* Save the parameters, which will be used the next time that
	 * the area detector "arms".
	 */

	sp = &(ad->sequence_parameters);

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

	mx_status = (*set_parameter_fn)( ad );

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

	mx_status = mx_area_detector_get_bytes_per_frame( ad->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,
	("%s: image_format = %ld, framesize = (%ld,%ld), bytes_per_frame = %ld",
		fname, ad->image_format,
		ad->framesize[0],
		ad->framesize[1],
		ad->bytes_per_frame));
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
	mx_status_type ( *busy_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	busy_fn = flist->busy;

	if ( busy_fn != NULL ) {
		mx_status = (*busy_fn)( ad );
	}

	if ( busy != NULL ) {
		*busy = ad->busy;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_status( MX_RECORD *record,
			long *last_frame_number,
			unsigned long *status_flags )
{
	static const char fname[] = "mx_area_detector_get_status()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_status_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn = flist->get_status;

	if ( get_status_fn != NULL ) {
		mx_status = (*get_status_fn)( ad );
	}

	if ( last_frame_number != NULL ) {
		*last_frame_number = 0;
	}
	if ( status_flags != NULL ) {
		*status_flags = ad->status;
	}

	return mx_status;
}

/*---*/

MX_EXPORT mx_status_type
mx_area_detector_get_frame( MX_RECORD *record,
			long frame_number,
			MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mx_area_detector_get_frame()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_frame_fn ) ( MX_AREA_DETECTOR * );
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Does this driver implement a get_frame function? */

	get_frame_fn = flist->get_frame;

	if ( get_frame_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Getting an MX_IMAGE_FRAME structure for the most recently "
		"taken image has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: *frame = %p", fname, *frame));
#endif

	/* We either reuse an existing MX_IMAGE_FRAME or create a new one. */

	if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new MX_IMAGE_FRAME.", fname));
#endif
		/* Allocate a new MX_IMAGE_FRAME. */

		*frame = malloc( sizeof(MX_IMAGE_FRAME) );

		if ( (*frame) == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"a new MX_IMAGE_FRAME structure." );
		}

		(*frame)->header_length = 0;
		(*frame)->header_data = NULL;

		(*frame)->image_length = 0;
		(*frame)->image_data = NULL;
	}

	/* Fill in some parameters. */

	(*frame)->image_type = MXT_IMAGE_LOCAL_1D_ARRAY;
	(*frame)->framesize[0] = ad->framesize[0];
	(*frame)->framesize[1] = ad->framesize[1];
	(*frame)->image_format = ad->image_format;
	(*frame)->pixel_order = ad->pixel_order;

	/* See if the image buffer is already big enough for the image. */

#if MX_AREA_DETECTOR_DEBUG
	MX_DEBUG(-2,("%s: (*frame)->image_data = %p",
		fname, (*frame)->image_data));
	MX_DEBUG(-2,("%s: (*frame)->image_length = %lu, bytes_per_frame = %lu",
		fname, (unsigned long) (*frame)->image_length,
		ad->bytes_per_frame));
#endif

	/* Setup the image data buffer. */

	if ( ((*frame)->image_length == 0) && (ad->bytes_per_frame == 0)) {

		/* Zero length image buffers are not allowed. */

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Area detector '%s' attempted to create a zero length image buffer.",
			record->name );

	} else
	if ( ( (*frame)->image_data != NULL )
	  && ( (*frame)->image_length >= ad->bytes_per_frame ) )
	{
#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: The image buffer is already big enough.", fname));
#endif
	} else {

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new image buffer of %lu bytes.",
			fname, ad->bytes_per_frame));
#endif
		/* If not, then allocate a new one. */

		if ( (*frame)->image_data != NULL ) {
			free( (*frame)->image_data );
		}

		(*frame)->image_data = malloc( ad->bytes_per_frame );

		if ( (*frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate a %ld byte image buffer for "
			"area detector '%s'.",
				ad->bytes_per_frame, ad->record->name );
		}

#if MX_AREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: allocated new frame buffer.", fname));
#endif
	}

#if 1  /* FIXME!!! - This should not be present in the final version. */
	memset( (*frame)->image_data, 0, 50 );
#endif

	(*frame)->image_length = ad->bytes_per_frame;

	/* Now get the actual frame. */

	ad->frame_number = frame_number;

	ad->frame = *frame;

	mx_status = (*get_frame_fn)( ad );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_sequence( MX_RECORD *record,
			MX_IMAGE_SEQUENCE **sequence )
{
	static const char fname[] = "mx_area_detector_get_sequence()";

	MX_AREA_DETECTOR *ad;
	MX_AREA_DETECTOR_FUNCTION_LIST *flist;
	mx_status_type ( *get_sequence_fn ) (MX_AREA_DETECTOR *,
						MX_IMAGE_SEQUENCE **);
	mx_status_type mx_status;

	mx_status = mx_area_detector_get_pointers(record, &ad, &flist, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_sequence_fn = flist->get_sequence;

	if ( get_sequence_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Getting an MX_IMAGE_SEQUENCE structure for the most recently "
		"taken sequence has not yet "
		"been implemented for the driver for record '%s'.",
			record->name );
	}

	mx_status = (*get_sequence_fn)( ad, sequence );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_get_frame_from_sequence( MX_IMAGE_SEQUENCE *image_sequence,
					long frame_number,
					MX_IMAGE_FRAME **image_frame )
{
	static const char fname[] =
			"mx_area_detector_get_frame_from_sequence()";

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

	*image_frame = &(image_sequence->frame_array[ frame_number ]);

	return MX_SUCCESSFUL_RESULT;
}

#if 0

MX_EXPORT mx_status_type
mx_area_detector_read_1d_pixel_array( MX_IMAGE_FRAME *frame,
				long pixel_datatype,
				void *destination_pixel_array,
				size_t max_array_bytes,
				size_t *num_bytes_copied )
{
}

MX_EXPORT mx_status_type
mx_area_detector_read_1d_pixel_sequence( MX_IMAGE_SEQUENCE *sequence,
				long pixel_datatype,
				void *destination_pixel_array,
				size_t max_array_bytes,
				size_t *num_bytes_copied )
{
}

#endif

MX_EXPORT mx_status_type
mx_area_detector_default_get_parameter_handler( MX_AREA_DETECTOR *ad )
{
	static const char fname[] =
		"mx_area_detector_default_get_parameter_handler()";

	switch( ad->parameter_type ) {
	case MXLV_AD_FRAMESIZE:
	case MXLV_AD_FORMAT:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:

		/* We just return the value that is already in the 
		 * data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for video input '%s'.",
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
	case MXLV_AD_FORMAT:
	case MXLV_AD_SEQUENCE_TYPE:
	case MXLV_AD_NUM_SEQUENCE_PARAMETERS:
	case MXLV_AD_SEQUENCE_PARAMETER_ARRAY:

		/* We do nothing but leave alone the value that is already
		 * stored in the data structure.
		 */

		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the MX driver "
		"for video input '%s'.",
			mx_get_field_label_string( ad->record,
						ad->parameter_type ),
			ad->parameter_type,
			ad->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

