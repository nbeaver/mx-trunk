/*
 * Name:    d_network_vinput.c
 *
 * Purpose: MX network video input device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_VINPUT_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_image.h"
#include "mx_video_input.h"
#include "d_network_vinput.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxd_network_vinput_record_function_list = {
	NULL,
	mxd_network_vinput_create_record_structures,
	mxd_network_vinput_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_network_vinput_open,
	mxd_network_vinput_close
};

MX_VIDEO_INPUT_FUNCTION_LIST mxd_network_vinput_video_input_function_list = {
	mxd_network_vinput_arm,
	mxd_network_vinput_trigger,
	mxd_network_vinput_stop,
	mxd_network_vinput_abort,
	mxd_network_vinput_busy,
	mxd_network_vinput_get_status,
	mxd_network_vinput_get_frame,
	mxd_network_vinput_get_sequence,
	mxd_network_vinput_get_parameter,
	mxd_network_vinput_set_parameter,
};

MX_RECORD_FIELD_DEFAULTS mxd_network_vinput_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_VIDEO_INPUT_STANDARD_FIELDS,
	MXD_NETWORK_VINPUT_STANDARD_FIELDS
};

long mxd_network_vinput_num_record_fields
		= sizeof( mxd_network_vinput_record_field_defaults )
			/ sizeof( mxd_network_vinput_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_vinput_rfield_def_ptr
			= &mxd_network_vinput_record_field_defaults[0];

/*---*/

static mx_status_type
mxd_network_vinput_get_pointers( MX_VIDEO_INPUT *vinput,
			MX_NETWORK_VINPUT **network_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_vinput_get_pointers()";

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VIDEO_INPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if (network_vinput == NULL) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_NETWORK_VINPUT pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*network_vinput = (MX_NETWORK_VINPUT *)
				vinput->record->record_type_struct;

	if ( *network_vinput == (MX_NETWORK_VINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "MX_NETWORK_VINPUT pointer for record '%s' passed by '%s' is NULL.",
			vinput->record->name, calling_fname );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_network_vinput_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_vinput_create_record_structures()";

	MX_VIDEO_INPUT *vinput;
	MX_NETWORK_VINPUT *network_vinput;

	vinput = (MX_VIDEO_INPUT *) malloc( sizeof(MX_VIDEO_INPUT) );

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VIDEO_INPUT structure." );
	}

	network_vinput = (MX_NETWORK_VINPUT *)
				malloc( sizeof(MX_NETWORK_VINPUT) );

	if ( network_vinput == (MX_NETWORK_VINPUT *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_NETWORK_VINPUT structure." );
	}

	record->record_class_struct = vinput;
	record->record_type_struct = network_vinput;
	record->class_specific_function_list = 
			&mxd_network_vinput_video_input_function_list;

	memset( &(vinput->sequence_parameters),
			0, sizeof(vinput->sequence_parameters) );

	vinput->record = record;
	network_vinput->record = record;

	vinput->trigger_mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_finish_record_initialization( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	vinput = (MX_VIDEO_INPUT *) record->record_class_struct;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name));
#endif

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s complete for record '%s'.", fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_arm( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_arm()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	/* FIXME - Nothing here for now. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_trigger()";

	MX_NETWORK_VINPUT *network_vinput;
	MX_SEQUENCE_PARAMETERS *sp;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'",
		fname, vinput->record->name ));
#endif

	sp = &(vinput->sequence_parameters);

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: sp = %p", fname, sp));

	MX_DEBUG(-2,("%s: sp->sequence_type = %ld", fname, sp->sequence_type));
#endif

	switch( sp->sequence_type ) {
	case MXT_SQ_ONE_SHOT:
		break;
	case MXT_SQ_CONTINUOUS:
		break;

	case MXT_SQ_MULTI:
		if ( sp->num_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a sequence of type MXT_SQ_MULTI should be "
			"the number of frames.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sp->num_parameters );
				
		}

#if 0
		numbuf = mx_round( sp->parameter_array[0] );

		if ( numbuf > pxd_imageZdim() ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%ld) for "
			"video input '%s' is larger than the maximum value "
			"of %d.",
			    numbuf, vinput->record->name, pxd_imageZdim());
		}

		startbuf = 1;
		endbuf = startbuf + numbuf - 1;

		epix_status = pxd_goLiveSeq( network_vinput->unitmap,
					startbuf, endbuf, 1, numbuf, 1 );

		if ( epix_status != 0 ) {
			mxi_network_vinput_error_message(
				network_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start multi frame "
			"acquisition with video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
#endif
		break;

	case MXT_SQ_CONTINUOUS_MULTI:
		if ( sp->num_parameters < 1 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The first sequence parameter of video input '%s' "
			"for a sequence of type MXT_SQ_CONTINUOUS_MULTI should "
			"be the number of frames in the circular buffer.  "
			"However, the sequence says that it has %ld frames.",
			    vinput->record->name, sp->num_parameters );
		}

#if 0
		endbuf = mx_round( sp->parameter_array[0] );

		if ( endbuf > pxd_imageZdim() ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested number of sequence frames (%ld) for "
			"video input '%s' is larger than the maximum value "
			"of %d.",
			    endbuf, vinput->record->name, pxd_imageZdim());
		}

		startbuf = 1;

		epix_status = pxd_goLiveSeq( network_vinput->unitmap,
					startbuf, endbuf, 1, 0, 1 );

		if ( epix_status != 0 ) {
			mxi_network_vinput_error_message(
				network_vinput->unitmap, epix_status,
				error_message, sizeof(error_message) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"The attempt to start continuous multi frame "
			"acquisition with video input '%s' failed.  %s",
				vinput->record->name, error_message );
		}
#endif
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Image sequence type %ld is not supported by "
			"video input '%s'.",
			sp->sequence_type, vinput->record->name );
	}

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: Started taking a frame using video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_stop( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_stop()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_abort( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_abort()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_busy( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_busy()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_status( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_get_status()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	mx_status = mxd_network_vinput_busy( vinput );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_frame( MX_VIDEO_INPUT *vinput, MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mxd_network_vinput_get_frame()";

	MX_NETWORK_VINPUT *network_vinput;
	long image_format, bytes_per_image, words_to_read;
	long x_framesize, y_framesize;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( frame == (MX_IMAGE_FRAME **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_FRAME pointer passed was NULL." );
	}

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	/* Get the image pixel format. */

	mx_status = mx_video_input_get_image_format( vinput->record,
							&image_format );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: image_format = %ld", fname, image_format));
#endif

	/* Get the dimensions of the image. */

	mx_status = mx_video_input_get_framesize( vinput->record,
						&x_framesize, &y_framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the number of bytes in the image. */

	switch( image_format ) {
	case MXT_IMAGE_FORMAT_RGB:
		bytes_per_image = 3 * x_framesize * y_framesize;
		break;

	case MXT_IMAGE_FORMAT_GREY8:
		bytes_per_image = x_framesize * y_framesize;
		break;

	case MXT_IMAGE_FORMAT_GREY16:
		bytes_per_image = 2 * x_framesize * y_framesize;
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported image format %ld for video input '%s'.",
			image_format, vinput->record->name );
	}

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: bytes_per_image = %ld", fname, bytes_per_image));

	MX_DEBUG(-2,("%s: *frame = %p", fname, *frame));
#endif

	/* At this point, we either reuse an existing MX_IMAGE_FRAME
	 * or create a new one.
	 */

	if ( (*frame) == (MX_IMAGE_FRAME *) NULL ) {

#if MXD_NETWORK_VINPUT_DEBUG
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
	(*frame)->framesize[0] = x_framesize;
	(*frame)->framesize[1] = y_framesize;
	(*frame)->image_format = image_format;

	/* See if the image buffer is already big enough for the image. */

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: (*frame)->image_data = %p",
		fname, (*frame)->image_data));
	MX_DEBUG(-2,("%s: (*frame)->image_length = %lu, bytes_per_image = %lu",
	    fname, (unsigned long) (*frame)->image_length, bytes_per_image));
#endif

	if ( ( (*frame)->image_data != NULL )
	  && ( (*frame)->image_length >= bytes_per_image ) )
	{
#if MXD_NETWORK_VINPUT_DEBUG
		MX_DEBUG(-2,
		("%s: The image buffer is already big enough.", fname));
#endif
	} else {

#if MXD_NETWORK_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: Allocating a new image buffer of %lu bytes.",
			fname, bytes_per_image));
#endif
		/* If not, then allocate a new one. */

		if ( (*frame)->image_data != NULL ) {
			free( (*frame)->image_data );
		}

		(*frame)->image_data = malloc( bytes_per_image );

		if ( (*frame)->image_data == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate a %ld byte image buffer for "
			"video input '%s'.",
				bytes_per_image, vinput->record->name );
		}


#if MXD_NETWORK_VINPUT_DEBUG
		MX_DEBUG(-2,("%s: allocated new frame buffer.", fname));
#endif
	}

#if 1  /* FIXME!!! - This should not be present in the final version. */
	memset( (*frame)->image_data, 0, 50 );
#endif

	(*frame)->image_length = bytes_per_image;

	/* Now read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: reading a %lu byte image frame.",
			fname, (unsigned long) (*frame)->image_length ));
#endif

	if ( image_format == MXT_IMAGE_FORMAT_GREY16 ) {
		words_to_read = ((*frame)->image_length) / 2;
		
#if 0
		result = pxd_readushort( network_vinput->unitmap, 1,
				0, 0, -1, -1,
				(*frame)->image_data, words_to_read,
				colorspace );

		if ( result >= 0 ) {
			result = result * 2;
		}
#endif
	} else {
#if 0
		result = pxd_readuchar( network_vinput->unitmap, 1,
				0, 0, -1, -1,
				(*frame)->image_data, (*frame)->image_length,
				colorspace );
#endif
	}

	/* Was the read successful? */

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: successfully read a %lu byte image frame from video input '%s'.",
		fname, (unsigned long) (*frame)->image_length,
		vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_sequence( MX_VIDEO_INPUT *vinput,
				MX_IMAGE_SEQUENCE **sequence )
{
	static const char fname[] = "mxd_network_vinput_get_sequence()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sequence == (MX_IMAGE_SEQUENCE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_IMAGE_SEQUENCE pointer passed was NULL." );
	}

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s invoked for video input '%s'.",
		fname, vinput->record->name ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_get_parameter()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_PIXEL_ORDER:
	case MXLV_VIN_TRIGGER_MODE:
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_FORMAT:
#if 0
		bits_per_component = pxd_imageBdim();
		components_per_pixel = pxd_imageCdim();

		if ( bits_per_component <= 8 ) {
			switch( components_per_pixel ) {
			case 1:
			    vinput->image_format = MXT_IMAGE_FORMAT_GREY8;
			    break;
			case 3:
			    vinput->image_format = MXT_IMAGE_FORMAT_RGB;
			    break;
			default:
			    return mx_error( MXE_UNSUPPORTED, fname,
				"%d-bit video input '%s' reports an "
				"unsupported number of pixel components (%d).",
					bits_per_component,
					vinput->record->name,
					components_per_pixel );
			}
		} else
		if ( bits_per_component <= 16 ) {
			switch( components_per_pixel ) {
			case 1:
			    vinput->image_format = MXT_IMAGE_FORMAT_GREY16;
			    break;
			default:
			    return mx_error( MXE_UNSUPPORTED, fname,
				"%d-bit video input '%s' reports an "
				"unsupported number of pixel components (%d).",
					bits_per_component,
					vinput->record->name,
					components_per_pixel );
			}
		} else {
			return mx_error( MXE_UNSUPPORTED, fname,
				"Video input '%s' reports an unsupported "
				"number of bits per component (%d) "
				"and components per pixel (%d).",
					vinput->record->name,
					bits_per_component,
					components_per_pixel );
		}

#if MXD_NETWORK_VINPUT_DEBUG
		MX_DEBUG(-2,
		("%s: video format = %ld", fname, vinput->image_format));
#endif

#endif
		break;

	case MXLV_VIN_FRAMESIZE:
#if 0
		vinput->framesize[0] = pxd_imageXdim();
		vinput->framesize[1] = pxd_imageYdim();
#endif
		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_set_parameter( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_set_parameter()";

	MX_NETWORK_VINPUT *network_vinput;
	mx_status_type mx_status;

	mx_status = mxd_network_vinput_get_pointers( vinput,
						&network_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: record '%s', parameter type %ld",
		fname, vinput->record->name, vinput->parameter_type));
#endif

	switch( vinput->parameter_type ) {
	case MXLV_VIN_SEQUENCE_TYPE:
	case MXLV_VIN_NUM_SEQUENCE_PARAMETERS:
	case MXLV_VIN_SEQUENCE_PARAMETER_ARRAY:

		break;

	case MXLV_VIN_FORMAT:
		(void) mxd_network_vinput_get_parameter( vinput );

		return mx_error( MXE_UNSUPPORTED, fname,
		"Changing the video format for video input '%s' via "
		"MX is not supported.  In order to change video formats, "
		"you must create a new video configuration in the XCAP "
		"program from EPIX, Inc.", vinput->record->name );
		break;

	case MXLV_VIN_PIXEL_ORDER:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Changing the pixel order for video input '%s' "
			"is not supported.", vinput->record->name );
		break;

	case MXLV_VIN_FRAMESIZE:

#if 0
		/* Escape to the Structured Style Interface. */

		xc = pxd_xclibEscape(0, 0, 0);

		if ( xc == NULL ) {
			return mx_error( MXE_INITIALIZATION_ERROR, fname,
			"The XCLIB library has not yet been initialized "
			"for video input '%s' with pxd_PIXCIopen().",
				vinput->record->name );
		}

		xclib_InitVidStateStructs(vidstate);

		xc->pxlib.getState( &(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

		/* Change the necessary parameters. */

		vidstate.vidres->x.datsamples = vinput->framesize[0];
		vidstate.vidres->x.vidsamples = vinput->framesize[0];
		vidstate.vidres->x.setmidvidoffset = 0;
		vidstate.vidres->x.vidoffset = 0;
		vidstate.vidres->x.setmaxdatsamples = 0;
		vidstate.vidres->x.setmaxvidsamples = 0;

		vidstate.vidres->y.datsamples = vinput->framesize[1];
		vidstate.vidres->y.vidsamples = vinput->framesize[1];
		vidstate.vidres->y.setmidvidoffset = 0;
		vidstate.vidres->y.vidoffset = 0;
		vidstate.vidres->y.setmaxdatsamples = 0;
		vidstate.vidres->y.setmaxvidsamples = 0;

		vidstate.vidres->datfields = 1;
		vidstate.vidres->setmaxdatfields = 0;

		/* Leave the Structured Style Interface. */

		epix_status = xc->pxlib.defineState(
				&(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

		if ( epix_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error in xc->pxlib.defineState() for record '%s'.  "
			"Error code = %d",
				vinput->record->name, epix_status );
		}

		epix_status = pxd_xclibEscaped(0, 0, 0);

		if ( epix_status != 0 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Error in pxd_xclibEscaped() for record '%s'.  "
			"Error code = %d",
				vinput->record->name, epix_status );
		}

#endif
		break;

	case MXLV_VIN_TRIGGER_MODE:
#if 0
		if ( vinput->trigger_mode & MXT_IMAGE_INTERNAL_TRIGGER ) {

			mx_status = mxd_network_vinput_internal_trigger(
					vinput, network_vinput, TRUE );
		} else {
			mx_status = mxd_network_vinput_internal_trigger(
					vinput, network_vinput, FALSE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( vinput->trigger_mode & MXT_IMAGE_EXTERNAL_TRIGGER ) {

			mx_status = mxd_network_vinput_external_trigger(
					vinput, network_vinput, TRUE );
		} else {
			mx_status = mxd_network_vinput_external_trigger(
					vinput, network_vinput, FALSE );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif
		break;

	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Parameter type %ld not yet implemented for record '%s'.",
			vinput->parameter_type, vinput->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

