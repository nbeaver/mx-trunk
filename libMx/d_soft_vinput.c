/*
 * Name:    d_soft_vinput.c
 *
 * Purpose: MX software emulated video input device driver.  This driver
 *          generates test video frames.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_VINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_soft_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_soft_vinput_record_function_list = {
	NULL,
	mxd_soft_vinput_create_record_structures,
	mxd_soft_vinput_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_soft_vinput_open,
	mxd_soft_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_soft_vinput_video_input_function_list = {
	mxd_soft_vinput_arm,
	mxd_soft_vinput_trigger,
	mxd_soft_vinput_stop,
	mxd_soft_vinput_abort,
	NULL,
	NULL,
	NULL,
	mxd_soft_vinput_get_extended_status,
	mxd_soft_vinput_get_frame,
	mxd_soft_vinput_get_parameter,
	mxd_soft_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_SOFT_VINPUT_STANDARD_FIELDS
};

long mxd_soft_vinput_num_record_fields
		= sizeof( mxd_soft_vinput_record_field_defaults )
			/ sizeof( mxd_soft_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_vinput_rfield_def_ptr
			= &mxd_soft_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_soft_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_SOFT_VINPUT **soft_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_vinput_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (soft_vinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_SOFT_VINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*soft_vinput = (MX_SOFT_VINPUT *)
				vinput->record->record_type_struct;

	if ( *soft_vinput == (MX_SOFT_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_SOFT_VINPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_soft_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_soft_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_SOFT_VINPUT *soft_vinput;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	soft_vinput = (MX_SOFT_VINPUT *)
				malloc( sizeof(MX_SOFT_VINPUT) );

	if ( soft_vinput == (MX_SOFT_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_SOFT_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = soft_vinput;
	record->class_specific_function_list = 
			&mxd_soft_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	soft_vinput->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_soft_vinput_finish_record_initialization()";

	MX_VIDEO_INPUT *vinput;
	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_soft_vinput_get_pointers( vinput, &soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	mx_status = mx_video_input_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_SOFT_VINPUT *soft_vinput;
	long pixels_per_frame;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_soft_vinput_get_pointers( vinput, &soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif
	/* Initialize a bunch of driver parameters. */

	vinput->parameter_type = -1;
	vinput->frame_number   = -100;
	vinput->get_frame      = -100;
	vinput->frame          = NULL;
	vinput->frame_buffer   = NULL;
	vinput->pixel_order    = MXT_IMAGE_PIXEL_ORDER_STANDARD;
	vinput->trigger_mode   = MXT_IMAGE_NO_TRIGGER;

	soft_vinput->counter   = 0;

	switch( vinput->image_format ) {
	case MXT_IMAGE_FORMAT_RGB565:
	case MXT_IMAGE_FORMAT_YUYV:
		vinput->bytes_per_pixel = 2;
		break;

	case MXT_IMAGE_FORMAT_RGB:
		vinput->bytes_per_pixel = 3;
		break;
	case MXT_IMAGE_FORMAT_GREY8:
		vinput->bytes_per_pixel = 1;
		break;
	case MXT_IMAGE_FORMAT_GREY16:
		vinput->bytes_per_pixel = 2;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Image format %ld '%s' is not a supported image format "
		"for software-emulated video input '%s'.",
			vinput->image_format, vinput->image_format_name,
			record->name );
	}

	pixels_per_frame = vinput->framesize[0] * vinput->framesize[1];

	vinput->bytes_per_frame = pixels_per_frame
					* mx_round( vinput->bytes_per_pixel );

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: vinput->framesize[0] = %ld, vinput->framesize[1] = %ld",
		fname, vinput->framesize[0], vinput->framesize[1] ));

	MX_DEBUG(-2,("%s: vinput->image_format_name = '%s'",
		fname, vinput->image_format_name));

	MX_DEBUG(-2,("%s: vinput->image_format = %ld",
		fname, vinput->image_format));

	MX_DEBUG(-2,("%s: vinput->pixel_order = %ld",
		fname, vinput->pixel_order));

	MX_DEBUG(-2,("%s: vinput->trigger_mode = %ld",
		fname, vinput->trigger_mode));

	MX_DEBUG(-2,("%s: vinput->bytes_per_pixel = %g",
		fname, vinput->bytes_per_pixel));

	MX_DEBUG(-2,("%s: vinput->bytes_per_frame = %ld",
		fname, vinput->bytes_per_frame));
#endif

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_arm()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput, &soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_trigger()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput, &soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_stop()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput, &soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_abort()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput, &soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_get_extended_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_get_extended_status()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput,
						&soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif
	vinput->busy = FALSE;

	vinput->status = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_get_frame( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_get_frame()";

	MX_SOFT_VINPUT *soft_vinput;
	MX_IMAGE_FRAME *frame;
	unsigned long i, j, i_max, j_max;
	double x_max, y_max, max_value;
	double cx1, cx0, cy1, cy0;
	double cxr1, cxr0, cyr1, cyr0;
	double cxg1, cxg0, cyg1, cyg0;
	double cxb1, cxb0, cyb1, cyb0;
	double R, G, B;
	char *ptr8;
	uint16_t *ptr16;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput,
						&soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	frame = vinput->frame;

	if ( frame == (MX_IMAGE_FRAME *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

	/* Suppress GCC 4 uninitialized variables warnings. */

	cx1 = cx0 = cy1 = cy0 = 0.0;

	cxr1 = cxr0 = cyr1 = cyr0 = 0.0;
	cxg1 = cxg0 = cyg1 = cyg0 = 0.0;
	cxb1 = cxb0 = cyb1 = cyb0 = 0.0;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	/* Set the size of the image. */

	frame->bytes_per_pixel = vinput->bytes_per_pixel;
	frame->image_length    = vinput->bytes_per_frame;

	/* Generate the frame for the MX_IMAGE_FRAME structure. */

	switch( soft_vinput->image_type ) {
	case MXT_SOFT_VINPUT_DIAGONAL_GRADIENT:

		j_max = vinput->framesize[0] - 1;
		i_max = vinput->framesize[1] - 1;

		x_max = vinput->framesize[0] - 1;
		y_max = vinput->framesize[1] - 1;

		switch( vinput->image_format ) {
		case MXT_IMAGE_FORMAT_RGB:
			max_value = sqrt( 255.0 );

			switch( (soft_vinput->counter) % 4 ) {
			case 0:
			    cxr1 = max_value / x_max;     cxr0 = 0;
			    cyr1 = max_value / y_max;     cyr0 = 0;

			    cxg1 = - max_value / x_max;   cxg0 = max_value;
			    cyg1 = max_value / y_max;     cyg0 = 0;

			    cxb1 = max_value / x_max;     cxb0 = 0;
			    cyb1 = - max_value / y_max;   cyb0 = max_value;
			    break;

			case 1:
			    cxr1 = - max_value / x_max;   cxr0 = max_value;
			    cyr1 = - max_value / y_max;   cyr0 = max_value;

			    cxg1 = max_value / x_max;     cxg0 = 0;
			    cyg1 = max_value / y_max;     cyg0 = 0;

			    cxb1 = - max_value / x_max;   cxb0 = max_value;
			    cyb1 = max_value / y_max;     cyb0 = 0;
			    break;

			case 2:
			    cxr1 = max_value / x_max;     cxr0 = 0;
			    cyr1 = - max_value / y_max;   cyr0 = max_value;

			    cxg1 = - max_value / x_max;   cxg0 = max_value;
			    cyg1 = - max_value / y_max;   cyg0 = max_value;

			    cxb1 = max_value / x_max;     cxb0 = 0;
			    cyb1 = max_value / y_max;     cyb0 = 0;
			    break;

			case 3:
			    cxr1 = - max_value / x_max;   cxr0 = max_value;
			    cyr1 = max_value / y_max;     cyr0 = 0;

			    cxg1 = max_value / x_max;     cxg0 = 0;
			    cyg1 = - max_value / y_max;   cyg0 = max_value;

			    cxb1 = - max_value / x_max;   cxb0 = max_value;
			    cyb1 = - max_value / y_max;   cyb0 = max_value;
			    break;
			}

			ptr8 = frame->image_data;

			for ( i = 0; i <= i_max; i++ ) {
			    for ( j = 0; j <= j_max; j++ ) {

				R = ( cxr1 * j + cxr0 ) * ( cyr1 * i + cyr0 );
				G = ( cxg1 * j + cxg0 ) * ( cyg1 * i + cyg0 );
				B = ( cxb1 * j + cxb0 ) * ( cyb1 * i + cyb0 );

				ptr8[0] = R;   ptr8[1] = G;   ptr8[2] = B;

				ptr8 += 3;
			    }
			}
			break;

		case MXT_IMAGE_FORMAT_GREY8:
			max_value = sqrt( 255.0 );

			switch( (soft_vinput->counter) % 4 ) {
			case 0:
			    cx1 = max_value / x_max;     cx0 = 0;
			    cy1 = max_value / y_max;     cy0 = 0;
			    break;

			case 1:
			    cx1 = - max_value / x_max;   cx0 = max_value;
			    cy1 = - max_value / y_max;   cy0 = max_value;
			    break;

			case 2:
			    cx1 = max_value / x_max;     cx0 = 0;
			    cy1 = - max_value / y_max;   cy0 = max_value;
			    break;

			case 3:
			    cx1 = - max_value / x_max;   cx0 = max_value;
			    cy1 = max_value / y_max;     cy0 = 0;
			    break;
			}

			ptr8 = frame->image_data;

			for ( i = 0; i <= i_max; i++ ) {
			    for ( j = 0; j <= j_max; j++ ) {

				*ptr8 = ( cx1 * j + cx0 ) * ( cy1 * i + cy0 );

				ptr8++;
			    }
			}
			break;

		case MXT_IMAGE_FORMAT_GREY16:
			max_value = sqrt( 65535.0 );

			switch( (soft_vinput->counter) % 4 ) {
			case 0:
			    cx1 = max_value / x_max;     cx0 = 0;
			    cy1 = max_value / y_max;     cy0 = 0;
			    break;

			case 1:
			    cx1 = - max_value / x_max;   cx0 = max_value;
			    cy1 = - max_value / y_max;   cy0 = max_value;
			    break;

			case 2:
			    cx1 = max_value / x_max;     cx0 = 0;
			    cy1 = - max_value / y_max;   cy0 = max_value;
			    break;

			case 3:
			    cx1 = - max_value / x_max;   cx0 = max_value;
			    cy1 = max_value / y_max;     cy0 = 0;
			    break;
			}

#if 0
			if ( soft_vinput->counter == 0 ) {
				MX_DEBUG(-2,("%s: j_max = %ld, i_max = %ld",
					fname, j_max, i_max));
				MX_DEBUG(-2,("%s: x_max = %g, y_max = %g",
					fname, x_max, y_max));
				MX_DEBUG(-2,("%s: max_value = %g",
					fname, max_value));
				MX_DEBUG(-2,("%s: cx1 = %g, cx0 = %g",
					fname, cx1, cx0));
				MX_DEBUG(-2,("%s: cy1 = %g, cy0 = %g",
					fname, cy1, cy0));
			}
#endif

			ptr16 = frame->image_data;

			for ( i = 0; i <= i_max; i++ ) {
			    for ( j = 0; j <= j_max; j++ ) {

				*ptr16 = ( cx1 * j + cx0 ) * ( cy1 * i + cy0 );

#if 0
				MX_DEBUG(-2,("%s: ptr16[%ld][%ld] = %d",
					fname, j, i, *ptr16));
#endif

				ptr16++;
			    }
			}
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Video input record '%s' does not support image format "
			"%ld.", vinput->record->name, frame->image_format );
		}
		break;
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image type %lu requested for video input '%s'.",
			soft_vinput->image_type, vinput->record->name );
	}

	soft_vinput->counter++;
				
#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (unsigned long) frame->image_length,
		vinput->record->name ));

	{
		int n;
		unsigned char c;
		unsigned char *frame_buffer;

		frame_buffer = frame->image_data;

		MX_DEBUG(-2,("%s: frame = %p, frame_buffer = %p",
			fname, frame, frame_buffer));

		for ( n = 0; n < 10; n++ ) {
			c = frame_buffer[n];

			MX_DEBUG(-2,("%s: frame_buffer[%d] = %u", fname, n, c));
		}
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_get_parameter()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput,
						&soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_FRAMESIZE:
		break;

	case MXLV_VIN_FORMAT:
	case MXLV_VIN_FORMAT_NAME:

		mx_status = mx_image_get_format_name_from_type(
				vinput->image_format, vinput->image_format_name,
				MXU_IMAGE_FORMAT_NAME_LENGTH );
#if MXD_SOFT_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: video format = %ld, format name = '%s'",
		    fname, vinput->image_format, vinput->image_format_name));
#endif
		break;

	case MXLV_VIN_PIXEL_ORDER:
		break;

	case MXLV_VIN_TRIGGER_MODE:
		break;

	case MXLV_VIN_BYTES_PER_FRAME:
		break;

	case MXLV_VIN_BYTES_PER_PIXEL:
		break;

	case MXLV_VIN_BUSY:
		break;

	case MXLV_VIN_STATUS:
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
mxd_soft_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_soft_vinput_set_parameter()";

	MX_SOFT_VINPUT *soft_vinput;
	mx_status_type mx_status;

	mx_status = mxd_soft_vinput_get_pointers( vinput,
						&soft_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_VINPUT_DEBUG
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

	case MXLV_VIN_PIXEL_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the pixel order for video input '%s' "
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

