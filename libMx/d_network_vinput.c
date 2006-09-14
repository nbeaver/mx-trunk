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
	static const char fname[] =
		"mxd_network_vinput_finish_record_initialization()";

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
	mx_status = mx_video_input_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_vinput->abort_nf),
			network_vinput->server_record,
			"%s.abort", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->arm_nf),
			network_vinput->server_record,
			"%s.arm", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->busy_nf),
			network_vinput->server_record,
			"%s.busy", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->framesize_nf),
			network_vinput->server_record,
			"%s.framesize", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->image_format_name_nf),
			network_vinput->server_record,
		"%s.image_format_name", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->image_format_nf),
			network_vinput->server_record,
			"%s.image_format", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->pixel_order_nf),
			network_vinput->server_record,
			"%s.pixel_order", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->status_nf),
			network_vinput->server_record,
			"%s.status", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->stop_nf),
			network_vinput->server_record,
			"%s.stop", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->trigger_nf),
			network_vinput->server_record,
			"%s.trigger", network_vinput->remote_record_name );

	/*---*/

	mx_network_field_init( &(network_vinput->sequence_type_nf),
			network_vinput->server_record,
		"%s.sequence_type", network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->num_sequence_parameters_nf),
			network_vinput->server_record,
			"%s.num_sequence_parameters",
				network_vinput->remote_record_name );

	mx_network_field_init( &(network_vinput->sequence_parameter_array_nf),
			network_vinput->server_record,
			"%s.sequence_parameter_array",
				network_vinput->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_vinput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_vinput_open()";

	MX_VIDEO_INPUT *vinput;
	MX_NETWORK_VINPUT *network_vinput;
	long dimension[1];
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
	/* Get the image framesize from the server. */

	dimension[0] = 2;

	mx_status = mx_get_array( &(network_vinput->framesize_nf),
				MXFT_LONG, 1, dimension,
				vinput->framesize );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: record '%s', framesize = (%ld, %ld)",
	    fname, record->name, vinput->framesize[0], vinput->framesize[1]));

	/* Get the image format name from the server. */

	dimension[0] = MXU_IMAGE_FORMAT_NAME_LENGTH;

	mx_status = mx_get_array( &(network_vinput->image_format_name_nf),
				MXFT_STRING, 1, dimension,
				vinput->image_format_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Update the image format type. */

	mx_status = mx_get_image_format_type_from_name(
					vinput->image_format_name,
					&(vinput->image_format) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,
	("%s: record '%s', image format name = '%s', image format = %ld",
		fname, record->name, vinput->image_format_name,
		vinput->image_format));
#endif

	/* Get the pixel order. */

	mx_status = mx_get( &(network_vinput->pixel_order_nf),
				MXFT_LONG, &(vinput->pixel_order) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

	mx_status = mx_put( &(network_vinput->arm_nf),
				MXFT_BOOL, &(vinput->arm) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_trigger( MX_VIDEO_INPUT *vinput )
{
	static const char fname[] = "mxd_network_vinput_trigger()";

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

	mx_status = mx_put( &(network_vinput->trigger_nf),
				MXFT_BOOL, &(vinput->trigger) );

	return mx_status;
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

	mx_status = mx_put( &(network_vinput->stop_nf),
				MXFT_BOOL, &(vinput->stop) );

	return mx_status;
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

	mx_status = mx_put( &(network_vinput->abort_nf),
				MXFT_BOOL, &(vinput->abort) );

	return mx_status;
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

	mx_status = mx_get( &(network_vinput->busy_nf),
				MXFT_BOOL, &(vinput->busy) );

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: video input '%s', busy = %d",
		fname, vinput->record->name, (int) vinput->busy ));
#endif

	return mx_status;
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

	mx_status = mx_get( &(network_vinput->status_nf),
				MXFT_HEX, &(vinput->status) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_vinput_get_frame( MX_VIDEO_INPUT *vinput,
				MX_IMAGE_FRAME **frame )
{
	static const char fname[] = "mxd_network_vinput_get_frame()";

	MX_NETWORK_VINPUT *network_vinput;
	long words_to_read;
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

	/* Read the frame into the MX_IMAGE_FRAME structure. */

#if MXD_NETWORK_VINPUT_DEBUG
	MX_DEBUG(-2,("%s: reading a %lu byte image frame.",
			fname, (unsigned long) (*frame)->image_length ));
#endif

	if ( vinput->image_format == MXT_IMAGE_FORMAT_GREY16 ) {
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

