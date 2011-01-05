/*
 * Name:    d_pleora_iport_vinput.c
 *
 * Purpose: MX video input driver for a Pleora iPORT video capture device.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_PLEORA_IPORT_VINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_array.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "i_pleora_iport.h"
#include "d_pleora_iport_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_pleora_iport_vinput_record_function_list = {
	NULL,
	mxd_pleora_iport_vinput_create_record_structures,
	mxd_pleora_iport_vinput_finish_record_initialization,
	NULL,
	NULL,
	mxd_pleora_iport_vinput_open,
	mxd_pleora_iport_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_pleora_iport_vinput_video_input_function_list = {
	mxd_pleora_iport_vinput_arm,
	mxd_pleora_iport_vinput_trigger,
	mxd_pleora_iport_vinput_stop,
	mxd_pleora_iport_vinput_abort,
	mxd_pleora_iport_vinput_asynchronous_capture,
	NULL,
	NULL,
	NULL,
	mxd_pleora_iport_vinput_get_extended_status,
	mxd_pleora_iport_vinput_get_frame,
	mxd_pleora_iport_vinput_get_parameter,
	mxd_pleora_iport_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_pleora_iport_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_PLEORA_IPORT_VINPUT_STANDARD_FIELDS
};

long mxd_pleora_iport_vinput_num_record_fields
		= sizeof( mxd_pleora_iport_vinput_record_field_defaults )
			/ sizeof( mxd_pleora_iport_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pleora_iport_vinput_rfield_def_ptr
			= &mxd_pleora_iport_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_pleora_iport_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_PLEORA_IPORT_VINPUT **pleora_iport_vinput,
			MX_PLEORA_IPORT **pleora_iport,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_pointers()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput_ptr;
	MX_RECORD *pleora_iport_record;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pleora_iport_vinput_ptr = (MX_PLEORA_IPORT_VINPUT *)
				vinput->record->record_type_struct;

	if ( pleora_iport_vinput_ptr == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT_VINPUT pointer for record '%s' "
			"passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}

	if ( pleora_iport_vinput != (MX_PLEORA_IPORT_VINPUT **) NULL ) {
		*pleora_iport_vinput = pleora_iport_vinput_ptr;
	}

	if ( pleora_iport != (MX_PLEORA_IPORT **) NULL ) {
		pleora_iport_record =
			pleora_iport_vinput_ptr->pleora_iport_record;

		if ( pleora_iport_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pleora_iport_record pointer for record '%s' "
			"is NULL.",
			vinput->record->name, calling_fname );
		}

		*pleora_iport = (MX_PLEORA_IPORT *)
					pleora_iport_record->record_type_struct;

		if ( *pleora_iport == (MX_PLEORA_IPORT *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PLEORA_IPORT pointer for record '%s' used "
			"by record '%s' is NULL.",
				vinput->record->name,
				pleora_iport_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	pleora_iport_vinput = (MX_PLEORA_IPORT_VINPUT *)
				malloc( sizeof(MX_PLEORA_IPORT_VINPUT) );

	if ( pleora_iport_vinput == (MX_PLEORA_IPORT_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PLEORA_IPORT_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = pleora_iport_vinput;
	record->class_specific_function_list = 
			&mxd_pleora_iport_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	pleora_iport_vinput->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pleora_iport_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	long pixels_per_frame;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->byte_order     = (long) mx_native_byteorder();
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	vinput->total_num_frames = 0;

	vinput->image_format = -1;
	vinput->bytes_per_pixel = 2;
	vinput->bits_per_pixel  = 16;

	pixels_per_frame = vinput->framesize[0] * vinput->framesize[1];

	vinput->bytes_per_frame = pixels_per_frame
					* mx_round( vinput->bytes_per_pixel );

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: vinput->framesize[0] = %ld, vinput->framesize[1] = %ld",
		fname, vinput->framesize[0], vinput->framesize[1] ));

	MX_DEBUG(-2,("%s: vinput->image_format_name = '%s'",
		fname, vinput->image_format_name));

	MX_DEBUG(-2,("%s: vinput->image_format = %ld",
		fname, vinput->image_format));

	MX_DEBUG(-2,("%s: vinput->byte_order = %ld",
		fname, vinput->byte_order));

	MX_DEBUG(-2,("%s: vinput->trigger_mode = %ld",
		fname, vinput->trigger_mode));

	MX_DEBUG(-2,("%s: vinput->bits_per_pixel = %ld",
		fname, vinput->bits_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_pixel = %g",
		fname, vinput->bytes_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_frame = %ld",
		fname, vinput->bytes_per_frame));
#endif

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_arm()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_SEQUENCE_PARAMETERS *seq;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	seq = &(vinput->sequence_parameters);

	switch( seq->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
	case MXT_SQ_CONTINUOUS:
		break;
	case MXT_SQ_MULTIFRAME:
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported sequence type %lu requested for video input '%s'.",
			seq->sequence_type, vinput->record->name );
		break;
	}

	switch( vinput->trigger_mode ) {
	case MXT_IMAGE_INTERNAL_TRIGGER:
		break;
	case MXT_IMAGE_EXTERNAL_TRIGGER:
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_trigger()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_stop()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_abort()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_asynchronous_capture( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_asynchronous_capture()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] =
		"mxd_pleora_iport_vinput_get_extended_status()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( vinput->status & MXSF_VIN_IS_BUSY ) {
		vinput->busy = TRUE;
	} else {
		vinput->busy = FALSE;
	}

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: last_frame_number = %ld, total_num_frames = %ld, status = %#lx",
		fname, vinput->last_frame_number, vinput->total_num_frames,
		vinput->status));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_frame()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	MX_IMAGE_FRAME *frame;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_get_parameter()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		mx_status = mx_image_get_image_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_PLEORA_IPORT_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_BYTE_ORDER:
		vinput->byte_order = (long) mx_native_byteorder();
		break;

	case MXLV_VIN_TRIGGER_MODE:
		vinput->trigger_mode = MXT_IMAGE_INTERNAL_TRIGGER;
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		vinput->bytes_per_frame = mx_round( vinput->bytes_per_pixel
			* vinput->framesize[0] * vinput->framesize[1] );
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		switch( vinput->image_format ) {
		case MXT_IMAGE_FORMAT_RGB:
			vinput->bytes_per_pixel = 3;
			vinput->bits_per_pixel  = 24;
			break;
		case MXT_IMAGE_FORMAT_GREY8:
			vinput->bytes_per_pixel = 1;
			vinput->bits_per_pixel  = 8;
			break;
		case MXT_IMAGE_FORMAT_GREY16:
			vinput->bytes_per_pixel = 2;
			vinput->bits_per_pixel  = 16;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported image format %ld for video input '%s'.",
				vinput->image_format, vinput->record->name );
		}
		break;

	case MXLV_VIN_BUSY:
		vinput->busy = 0;
		break;

	case MXLV_VIN_STATUS:
		vinput->status = 0;
		break;

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;
	default:
		mx_status =
			mx_video_input_default_get_parameter_handler( vinput );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pleora_iport_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_pleora_iport_vinput_set_parameter()";

	MX_PLEORA_IPORT_VINPUT *pleora_iport_vinput = NULL;
	mx_status_type mx_status;

	mx_status = mxd_pleora_iport_vinput_get_pointers( vinput,
					&pleora_iport_vinput, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_PLEORA_IPORT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the image format is not supported for "
			"video input '%s'.", vinput->record->name );

	case MXLV_VIN_BYTE_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the byte order for video input '%s' "
			"is not supported.", vinput->record->name );

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Directly changing the number of bytes per frame "
			"for video input '%s' is not supported.",
				vinput->record->name );

	case MXLV_VIN_SEQUENCE_TYPE:
		break;

	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:
		break;

	default:
		mx_status =
			mx_video_input_default_set_parameter_handler( vinput );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

